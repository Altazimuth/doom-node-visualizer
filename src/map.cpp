#include "map.h"
#include "wad.h"
#include "system.h"
#include "memory.h"

#include "string.h"
#include "math.h"


struct MapSector {
	i16 floorheight, ceilingheight;
	i8  floorpic[8], ceilingpic[8];

	i16 lightlevel;
	i16 special;
	i16 tag;
};

struct MapVertex {
	i16 x, y;
};

struct MapSideDef {
	i16 xoffset, yoffset;
	i8 topTexture[8], bottomTexture[8], midTexture[8];
	i16 sector;
};

struct MapLine {
	i16 v1, v2;
	i16 flags;
	i16 special, tag;
	i16 sidenum[2];
};

struct MapSubsector {
	i16 numSegs;
	i16 firstSeg;
};

struct MapSeg {
	i16 v1, v2;
	i16 angle;
	i16 linedef;
	i16 side;
	i16 xoffset;
};

struct MapNode {
	i16 x, y;
	i16 dx, dy;
	i16 bbox[2][4];
	u16 children[2];
};

struct MapThing {
	i16 x, y;
	i16 angle;
	i16 type;
	i16 options;
};

struct MapZSeg {
	u32 v1;
	union {
		u32 v2;
		u32 partner; // XGLN, XGL2
	};
	u32 linedef;
	u8 side;
};

struct MapZNode {
	union {
		struct {
			i16 x, y; // Partition line from (x,y) to x+dx,y+dy)
			i16 dx, dy;
		};
		struct {
			i32 x32, y32; // Partition line from (x,y) to x+dx,y+dy)
			i32 dx32, dy32;
		};
	};
	// Bounding box for each child, clip against view frustum.
	i16 bbox[2][4];
	// If NF_SUBSECTOR its a subsector, else it's a node of another subtree.
	i32 children[2];
};

inline static void u16ToNodeChild(Map *map, i32 *loc, u16 value)
{
	// Support for extended nodes
	if (value == 0xffff) {
		*loc = -1l;
	} else if (value & 0x8000) {
		// Convert to extended type
		*loc = value & ~0x8000;

		// Check for invalid subsector reference
		if (*loc >= map->subsectors.length) {
			logMessage("\tWarning: BSP tree references invalid subsector # % d\a\n", *loc);
			*loc = 0;
		}

		*loc |= SubsectorChildFlag;
	}
	else
		*loc = (i32)value & 0xffff;
}

bool loadStandardNodes(Map *map, LumpNum lumpNum)
{
	// Segs
	{
		auto segsLookup = getLumpByNum(lumpNum, (int)MapLumps::Segs);
		if (segsLookup.result != WadResult::Success || strncmp(segsLookup.name, "SEGS\0", 8) != 0) return false;

		Slice<MapSeg> mapSegs;
		mapSegs.data = (MapSeg*)segsLookup.lump.data;
		mapSegs.length = segsLookup.lump.length / sizeof(MapSeg);

		map->segs.data = arenaAlloc<Seg>(level, mapSegs.length);
		map->segs.length = mapSegs.length;

		for (int i = 0; i < mapSegs.length; ++i) {
			MapSeg *ms = mapSegs.data + i;
			Seg *s = map->segs.data + i;

			s->v1 = &map->vertexes.data[ms->v1];
			s->v2 = &map->vertexes.data[ms->v2];
			s->xoffset = ms->xoffset;
			s->linedef = &map->lines.data[ms->linedef];
			s->side = &map->sides.data[ms->side];

			if (ms->side < 0 || ms->side > 1) {
				logMessage("Seg %i side out of range (value: %i)", i, s->side);
				return false;
			}

			{
				Vertex *v1, *v2;
				v1 = map->vertexes.data + ms->v1;
				v2 = map->vertexes.data + ms->v2;

				f32 dx = (v2->x - v1->x);
				f32 dy = (v2->y - v1->y);

				s->length = sqrtf(dx * dx + dy * dy);
			}

			LineDef* line = s->linedef;

			s->frontsector = map->sides[line->sidenum[ms->side]].sector;
			if (line->flags & (i32)LineFlags::TwoSided && line->sidenum[ms->side ^ 1] != -1) {
				s->backsector = map->sides[line->sidenum[ms->side ^ 1]].sector;
			}
			else {
				s->backsector = nullptr;
			}
		}

		logMessage("\tLoaded %i segs", map->segs.length);
	}

	// Subsectors
	{
		auto ssecLookup = getLumpByNum(lumpNum, (int)MapLumps::SubSectors);
		if (ssecLookup.result != WadResult::Success || strncmp(ssecLookup.name, "SSECTORS", 8) != 0) return false;

		Slice<MapSubsector> mapSubSectors;
		mapSubSectors.data = (MapSubsector*)ssecLookup.lump.data;
		mapSubSectors.length = ssecLookup.lump.length / sizeof(MapSubsector);

		map->subsectors.data = arenaAlloc<SubSector>(level, mapSubSectors.length);
		map->subsectors.length = mapSubSectors.length;

		for (int i = 0; i < mapSubSectors.length; ++i) {
			SubSector *ssec = map->subsectors.data + i;
			MapSubsector *mssec = mapSubSectors.data + i;

			ssec->firstseg = mssec->firstSeg;
			ssec->numsegs = mssec->numSegs;

			Seg* seg = map->segs.data + ssec->firstseg;
			ssec->sector = seg->frontsector;
		}

		logMessage("\tLoaded %i subsectors", map->subsectors.length);
	}

	// Nodes
	{
		auto nodesLookup = getLumpByNum(lumpNum, (int)MapLumps::Nodes);
		if (nodesLookup.result != WadResult::Success || strncmp(nodesLookup.name, "NODES", 8) != 0) return false;

		Slice<MapNode> mapNodes;
		mapNodes.data = (MapNode*)nodesLookup.lump.data;
		mapNodes.length = nodesLookup.lump.length / sizeof(MapNode);

		map->nodes.data = arenaAlloc<Node>(level, mapNodes.length);
		map->nodes.length = mapNodes.length;

		for (int i = 0; i < mapNodes.length; ++i) {
			MapNode* mn = mapNodes.data + i;
			Node*    n = map->nodes.data + i;

			n->x = (f32)mn->x;
			n->y = (f32)mn->y;
			n->dx = (f32)mn->dx;
			n->dy = (f32)mn->dy;

			for (int j = 0; j < 2; ++j) {
				u16ToNodeChild(map, &(n->children[j]), mn->children[j]);

				for (int k = 0; k < 4; ++k) {
					n->bbox[j][k] = (f32)mn->bbox[j][k];
				}
			}
		}

		logMessage("\tLoaded %i nodes", map->nodes.length);
	}

	return true;
}

inline static i32 safeRealU32Index(u16 input, usize limit, const char *func, int index, const char *item)
{
	i32 ret = (i32)(input) & 0xffff;

	if (ret >= limit) {
		logMessage("\tError: %s #%d - bad %s #%d\a\n", func, index, item, ret);
		ret = 0;
	}

	return ret;
}

static void calcSegLength(Seg *lseg)
{
	float dx, dy;

	dx = lseg->v2->x - lseg->v1->x;
	dy = lseg->v2->x - lseg->v1->x;
	lseg->length = sqrtf(dx * dx + dy * dy);
}

static void calcSegOffset(Seg *lseg, const LineDef *line, u8 side)
{
	float dx = (side ? line->v2->x : line->v1->x) - lseg->v1->x;
	float dy = (side ? line->v2->y : line->v1->y) - lseg->v1->y;

	lseg->xoffset = sqrtf(dx * dx + dy * dy);
}

bool loadZSegs(Map *map, u8* data)
{
	SubSector *ss = map->subsectors.data; // For GL znodes
	int actualSegIndex = 0;
	Seg *prevSegToSet = nullptr;
	Vertex *firstV1 = nullptr; // First vertex of first seg

	for(u32 i = 0; i < map->segs.length; i++, ++actualSegIndex)
	{
		LineDef *ldef;
		u32 v1, v2 = 0;
		u32 linedef;
		u8 side;
		Seg *li = map->segs.data + actualSegIndex;
		MapZSeg mzdeg{};

		// Increment current subsector if applicable
		//if (type != ZNodeType_Normal) {
		//	if (actualSegIndex >= ss->firstseg + ss->numsegs) {
		//		++ss;
		//		ss->firstseg = actualSegIndex;
		//		firstV1 = nullptr;
		//	}
		//}

		// FIXME - see no verification of vertex indices
		v1 = mzdeg.v1 = *(u32*)data; data += sizeof(u32);
		//if (type == ZNodeType_Normal) {   // Only set directly for nonGL
			v2 = mzdeg.v2 = *(u32*)data; data += sizeof(u32);
		//} else {
		//	if (actualSegIndex == ss->firstline && !firstV1) { // Only set it once
		//		firstV1 = ::vertexes + v1;
		//	} else if (prevSegToSet) {
		//		// set the second vertex of previous
		//		prevSegToSet->v2 = ::vertexes + v1;
		//		P_CalcSegLength(prevSegToSet);
		//		prevSegToSet = nullptr; // consume it
		//	}
		//
		//	mzdeg.partner = *(u32*)data; data += sizeof(u32); // Not used in EE and probably not used here
		//}

		//if (type == ZNodeType_Normal || type == ZNodeType_GL) {
			mzdeg.linedef = *(u16*)data; data += sizeof(u16);
		//} else {
		//	mzdeg.linedef = *(u32*)data; data += sizeof(u32);
		//}
		mzdeg.side = *data++;

		//if ((type == ZNodeType_GL && mzdeg.linedef == 0xffff) || ((type == ZNodeType_GL2 || type == ZNodeType_GL3)
		//	&& mzdeg.linedef == 0xffffffff)) {
		//	--actualSegIndex;
		//	--ss->numsegs;
		//	continue; // Skip strictly GL nodes
		//}

		linedef = safeRealU32Index(mzdeg.linedef, map->lines.length, "seg", actualSegIndex, "line");

		ldef = &map->lines.data[linedef];
		li->linedef = ldef;
		side = mzdeg.side;

		// Fix wrong side index
		if (side != 0 && side != 1)
			side = 1;

		li->side = &map->sides.data[ldef->sidenum[side]];
		li->frontsector = map->sides.data[ldef->sidenum[side]].sector;

		// Ignore 2s flag if second sidedef missing:
		if (((i32)ldef->flags & (i32)LineFlags::TwoSided) && ldef->sidenum[side ^ 1] != -1) {
			li->backsector = map->sides.data[ldef->sidenum[side ^ 1]].sector;
		} else {
			li->backsector = nullptr;
		}

		li->v1 = &map->vertexes.data[v1];
		//if (type == ZNodeType_Normal) {
			li->v2 = &map->vertexes.data[v2];
		//} else {
		//	if (actualSegIndex + 1 == ss->firstseg + ss->numsegs) {
		//		li->v2 = firstV1;
		//		if (firstV1) { // firstV1 can be null because of malformed subsectors
		//			calcSegLength(li);
		//			firstV1 = nullptr;
		//		} else {
		//			level_error = "Bad ZDBSP nodes; can't start level.";
		//		}
		//	} else {
		//		prevSegToSet = li;
		//	}
		//}

		if (li->v2) { // Only count if v2 is available.
			calcSegLength(li);
		}
		calcSegOffset(li, ldef, side);
	}

	// Update the seg count
	map->segs.length = actualSegIndex;

	return true;
}

//
// For safe reading of ZDoom nodes lump
//
static bool checkZNodesOverflowFN(i64 &size, usize count)
{
	size -= count;

	if (size < 0) {
		return false;
	}

	return true;
}

#define checkZNodesOverflow(size, count) \
	if (!checkZNodesOverflowFN(size, count)) { \
		return false; \
	}


bool loadZNodes(Map *map, LumpNum lumpNum)
{
	auto nodesLookup = getLumpByNum(lumpNum, (int)MapLumps::Nodes);
	i64 len = nodesLookup.lump.length;
	u8* data = nodesLookup.lump.data;

	u32 orgVerts, newVerts;
	u32 numSubs, currSeg;
	u32 numSegs;
	u32 numNodes;
	Vertex *newVertArray = nullptr;

	// Skip header
	checkZNodesOverflow(len, 4);
	data += 4;

	// Vertexes (extra added during nodebuilding)
	{
		checkZNodesOverflow(len, sizeof(orgVerts));
		orgVerts = *(u32*)data; data += sizeof(u32);

		checkZNodesOverflow(len, sizeof(newVerts));
		newVerts = *(u32*)data; data += sizeof(u32);

		checkZNodesOverflow(len, newVerts * 2 * sizeof(i32));
		if (orgVerts + newVerts == (u32)map->vertexes.length) {
			newVertArray = map->vertexes.data;
		} else {
			// Use tag (here and elsewhere)
			newVertArray = arenaAlloc<Vertex>(level, orgVerts + newVerts);
			memcpy(newVertArray, map->vertexes.data, orgVerts * sizeof(Vertex));
		}

		for (u32 i = 0; i < newVerts; i++) {
			const u32 vIndex = i + orgVerts;

			newVertArray[vIndex].x = (f32)(*(fixed32*)data / fpFracUnit); data += sizeof(fixed32);
			newVertArray[vIndex].y = (f32)(*(fixed32*)data / fpFracUnit); data += sizeof(fixed32);
		}

		if (map->vertexes.data != newVertArray) {
			// Fixup linedef vertex pointers
			for (u32 i = 0; i < (u32)map->lines.length; i++) {
				map->lines.data[i].v1 = &newVertArray[map->lines[i].v1 - map->vertexes.data];
				map->lines.data[i].v2 = &newVertArray[map->lines[i].v2 - map->vertexes.data];
			}
			//free(map->vertexes.data);
			map->vertexes.data = newVertArray;
			map->vertexes.length = (usize)(orgVerts + newVerts);
		}

		logMessage("\tLoaded %i extra vertexes", newVerts);
	}

	// Subsectors
	{
		checkZNodesOverflow(len, sizeof(numSubs));
		numSubs = *(u32*)data; data += sizeof(u32);
		map->subsectors.length = (usize)numSubs;
		if (map->subsectors.length <= 0) {
			logMessage("\tNo subsectors in level");
			//Z_Free(lumpptr);
			return false;
		}

		checkZNodesOverflow(len, numSubs * sizeof(u32));
		map->subsectors.data = arenaAlloc<SubSector>(level, map->subsectors.length);
		for(u32 i = currSeg = 0; i < numSubs; i++)
		{
			map->subsectors.data[i].firstseg = (i32)currSeg;
			map->subsectors.data[i].numsegs = *(i32*)data; data += sizeof(i32);
			currSeg += map->subsectors.data[i].numsegs;
		}

		logMessage("\tLoaded %i subsectors", map->subsectors.length);
	}

	// Segs
	{
		checkZNodesOverflow(len, sizeof(numSegs));
		numSegs = *(u32*)data; data += sizeof(u32);

		// The number of segs stored should match the number of
		// segs used by subsectors.
		if (numSegs != currSeg) {
			logMessage("\tIncorrect number of segs in nodes");
			//Z_Free(lumpptr);
			return false;
		}

		map->segs.length = (usize)numSegs;
		usize totalSegSize;
		//if (signature.type == ZNodeType_Normal || signature.type == ZNodeType_GL) {
			totalSegSize = map->segs.length * 11; // Hardcoded original structure size
		// } else {
		//	totalSegSize = numsegs * 13; // DWORD linedef
		// }

		checkZNodesOverflow(len, totalSegSize);
		map->segs.data = arenaAlloc<Seg>(level, map->segs.length);
		loadZSegs(map, data); // FIXME

		data += totalSegSize;

		logMessage("\tLoaded %i segs", map->segs.length);
	}

	// Nodes
	{
		checkZNodesOverflow(len, sizeof(numNodes));
		numNodes = *(u32*)data; data += sizeof(u32);

		map->nodes.length = (usize)numNodes;
		checkZNodesOverflow(len, numNodes * 32);
		map->nodes.data = arenaAlloc<Node>(level, map->nodes.length);

		for (u32 i = 0; i < numNodes; i++) {
			Node* node = map->nodes.data + i;
			MapZNode mznode{};

			//if (signature.type == ZNodeType_GL3) {
			//	mznode.x32 = *(i32*)data; data += sizeof(i32);
			//	mznode.y32 = *(i32*)data; data += sizeof(i32);
			//	mznode.dx32 = *(i32*)data; data += sizeof(i32);
			//	mznode.dy32 = *(i32*)data; data += sizeof(i32);
			//} else {
				mznode.x = *(i16*)data; data += sizeof(i16);
				mznode.y = *(i16*)data; data += sizeof(i16);
				mznode.dx = *(i16*)data; data += sizeof(i16);
				mznode.dy = *(i16*)data; data += sizeof(i16);
			//}

			for (u32 j = 0; j < 2; j++) {
				for(u32 k = 0; k < 4; k++) {
					mznode.bbox[j][k] = *(i16*)data; data += sizeof(i16);
				}
			}

			for (u32 j = 0; j < 2; j++) {
				mznode.children[j] = *(i32*)data; data += sizeof(i32);
			}

			//if (signature.type == ZNodeType_GL3) {
			//	node->x = (f32)mznode.x32;
			//	node->y = (f32)mznode.y32;
			//	node->dx = (f32)mznode.dx32;
			//	node->dy = (f32)mznode.dy32;
			//
			//	// In Eternity node P_CalcNodeCoefficients2 is called here
			//} else {
				node->x = (f32)mznode.x;
				node->y = (f32)mznode.y;
				node->dx = (f32)mznode.dx;
				node->dy = (f32)mznode.dy;
			//}

			for (u32 j = 0; j < 2; j++) {
				node->children[j] = (u32)(mznode.children[j]);

				for(u32 k = 0; k < 4; k++) {
					node->bbox[j][k] = (f32)mznode.bbox[j][k];
				}
			}
		}

		logMessage("\tLoaded %i nodes", map->nodes.length);
	}

	return true;
}

//
// Inline routine to convert a short value to a long, preserving the value
// -1 but treating any other negative value as unsigned.
//
inline static i32 i16ToI32(i16 value)
{
	return (value == -1) ? -1l : (i32)value & 0xffff;
}

MapLoad loadMap(LumpNum lumpNum) {
	MapLoad result = {};

	resetArena(level);

	LumpResult mapMarker = getLumpByNum(lumpNum, 0);
	if (mapMarker.result != WadResult::Success) {
		result.result = MapResult::NotFound;
		return result;
	}

	result.result = MapResult::InvalidMap;

	auto map = arenaAlloc<Map>(level);

	logMessage("Loading map %.8s...", mapMarker.name);

	// Sectors
	{
		auto sectors = getLumpByNum(lumpNum, (int)MapLumps::Sectors);
		if (sectors.result == WadResult::Failure || strncmp(sectors.name, "SECTORS", 8) != 0) return result;

		Slice<MapSector> mapSectors;
		mapSectors.data = (MapSector*)sectors.lump.data;
		mapSectors.length = sectors.lump.length / sizeof(MapSector);

		map->sectors.data = arenaAlloc<Sector>(level, mapSectors.length);
		map->sectors.length = mapSectors.length;

		for (int i = 0; i < mapSectors.length; ++i) {
			Sector*    sec    = map->sectors.data + i;
			MapSector* mapsec = mapSectors.data + i;

			sec->ceilingheight = mapsec->ceilingheight;
			sec->floorheight = mapsec->floorheight;
			sec->floortex = -1;
			sec->ceilingtex = -1;
			sec->special = mapsec->special;
			sec->tag = mapsec->tag;
		}

		logMessage("\tLoaded %i sectors", map->sectors.length);
	}

	// Vertexes
	{
		auto vertexesLookup = getLumpByNum(lumpNum, (int)MapLumps::Vertexes);
		if (vertexesLookup.result == WadResult::Failure || strncmp(vertexesLookup.name, "VERTEXES", 8) != 0) return result;

		Slice<MapVertex> mapVertexes;
		mapVertexes.data = (MapVertex*)vertexesLookup.lump.data;
		mapVertexes.length = vertexesLookup.lump.length / sizeof(MapVertex);

		map->vertexes.data = arenaAlloc<Vertex>(level, mapVertexes.length);
		map->vertexes.length = mapVertexes.length;

		for (int i = 0; i < mapVertexes.length; ++i) {
			Vertex*    v  = map->vertexes.data + i;
			MapVertex* mv = mapVertexes.data + i;

			v->x = mv->x;
			v->y = mv->y;
		}

		logMessage("\tLoaded %i vertexes", map->vertexes.length);
	}

	// Sides
	{
		auto sidesLookup = getLumpByNum(lumpNum, (int)MapLumps::Sidedefs);
		if (sidesLookup.result == WadResult::Failure || strncmp(sidesLookup.name, "SIDEDEFS", 8) != 0) return result;

		Slice<MapSideDef> mapSides;
		mapSides.data = (MapSideDef*)sidesLookup.lump.data;
		mapSides.length = sidesLookup.lump.length / sizeof(MapSideDef);

		map->sides.data = arenaAlloc<SideDef>(level, mapSides.length);
		map->sides.length = mapSides.length;

		for (int i = 0; i < mapSides.length; ++i) {
			MapSideDef* mapside = mapSides.data + i;
			SideDef*    side = map->sides.data + i;

			side->xoffset = mapside->xoffset;
			side->yoffset = mapside->yoffset;
			side->sector = &map->sectors.data[mapside->sector];
			u64 foo = (u64)side->sector;
			side->topTexture = -1;
			side->bottomTexture = -1;
			side->midTexture = -1;
		}

		logMessage("\tLoaded %i sides", map->sides.length);
	}

	// Lines
	{
		auto linesLookup = getLumpByNum(lumpNum, (int)MapLumps::Linedefs);
		if (linesLookup.result == WadResult::Failure || strncmp(linesLookup.name, "LINEDEFS", 8) != 0) return result;

		Slice<MapLine> mapLines;
		mapLines.data = (MapLine*)linesLookup.lump.data;
		mapLines.length = linesLookup.lump.length / sizeof(MapLine);

		map->lines.data = arenaAlloc<LineDef>(level, mapLines.length);
		map->lines.length = mapLines.length;

		for (int i = 0; i < mapLines.length; ++i) {
			MapLine* ml = mapLines.data + i;
			LineDef* l  = map->lines.data + i;

			l->v1 = map->vertexes.data + ml->v1;
			l->v2 = map->vertexes.data + ml->v2;
			l->special = ml->special;
			l->flags = ml->flags;
			l->tag = ml->tag;
			l->sidenum[0] = i16ToI32(ml->sidenum[0]);
			l->sidenum[1] = i16ToI32(ml->sidenum[1]);
		}

		logMessage("\tLoaded %i lines", map->lines.length);
	}

	auto nodesLookup = getLumpByNum(lumpNum, (int)MapLumps::Nodes);
	if (nodesLookup.result != WadResult::Success || strncmp(nodesLookup.name, "NODES", 8) != 0) return result;
	if (!memcmp(nodesLookup.lump.data, "XNOD", 4)) {
		if (!loadZNodes(map, lumpNum)) return result;
	} else {
		if (!loadStandardNodes(map, lumpNum)) return result;
	}

	result.result = MapResult::Success;
	result.map = map;

	return result;
}
