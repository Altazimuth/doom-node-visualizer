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
	i16 children[2];
};

struct MapThing {
	i16 x, y;
	i16 angle;
	i16 type;
	i16 options;
};


MapLoad loadMap(LumpNum lumpNum) {
	MapLoad result = {};

	resetArena(level);

	LumpResult mapMarker = getLumpByNum(lumpNum, 0);
	if(mapMarker.result != WadResult::Success) {
		result.result = MapResult::NotFound;
		return result;
	}

	result.result = MapResult::InvalidMap;

	auto map = (Map*)memoryAlloc(level, sizeof(Map));

	logMessage("Loading map %.8s...", mapMarker.name);

	// Sectors
	{
		auto sectors = getLumpByNum(lumpNum, (int)MapLumps::Sectors);
		if (sectors.result == WadResult::Failure || strncmp(sectors.name, "SECTORS", 8) != 0) return result;

		Slice<MapSector> mapSectors;
		mapSectors.data = (MapSector*)sectors.lump.data;
		mapSectors.length = sectors.lump.length / sizeof(MapSector);

		map->sectors.data = (Sector*)memoryAlloc(level, sizeof(Sector) * mapSectors.length);
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

		logMessage("\tLoaded %i sectors", sectors.lump.length);
	}

	// Vertexes
	{
		auto vertexesLookup = getLumpByNum(lumpNum, (int)MapLumps::Vertexes);
		if (vertexesLookup.result == WadResult::Failure || strncmp(vertexesLookup.name, "VERTEXES", 8) != 0) return result;

		Slice<MapVertex> mapVertexes;
		mapVertexes.data = (MapVertex*)vertexesLookup.lump.data;
		mapVertexes.length = vertexesLookup.lump.length / sizeof(MapVertex);

		map->vertexes.data = (Vertex*)memoryAlloc(level, sizeof(Vertex) * mapVertexes.length);
		map->vertexes.length = mapVertexes.length;

		for (int i = 0; i < mapVertexes.length; ++i) {
			Vertex*    v  = map->vertexes.data + i;
			MapVertex* mv = mapVertexes.data + i;

			v->x = mv->x;
			v->y = mv->y;
		}

		logMessage("\tLoaded %i vertexes", vertexesLookup.lump.length);
	}

	// Sides
	{
		auto sidesLookup = getLumpByNum(lumpNum, (int)MapLumps::Sidedefs);
		if (sidesLookup.result == WadResult::Failure || strncmp(sidesLookup.name, "SIDEDEFS", 8) != 0) return result;

		Slice<MapSideDef> mapSides;
		mapSides.data = (MapSideDef*)sidesLookup.lump.data;
		mapSides.length = sidesLookup.lump.length / sizeof(MapSideDef);

		map->sides.data = (SideDef*)memoryAlloc(level, sizeof(SideDef) * mapSides.length);
		map->sides.length = mapSides.length;

		for (int i = 0; i < mapSides.length; ++i) {
			MapSideDef* mapside = mapSides.data + i;
			SideDef*    side = map->sides.data + i;

			side->xoffset = mapside->xoffset;
			side->yoffset = mapside->yoffset;
			side->sector = mapside->sector;
			side->topTexture = -1;
			side->bottomTexture = -1;
			side->midTexture = -1;
		}

		logMessage("\tLoaded %i sides", sidesLookup.lump.length);
	}

	// Lines
	{
		auto linesLookup = getLumpByNum(lumpNum, (int)MapLumps::Linedefs);
		if (linesLookup.result == WadResult::Failure || strncmp(linesLookup.name, "LINEDEFS", 8) != 0) return result;

		Slice<MapLine> mapLines;
		mapLines.data = (MapLine*)linesLookup.lump.data;
		mapLines.length = linesLookup.lump.length / sizeof(MapLine);

		map->lines.data = (LineDef*)memoryAlloc(level, sizeof(LineDef) * mapLines.length);
		map->lines.length = mapLines.length;

		for (int i = 0; i < mapLines.length; ++i) {
			MapLine* ml = mapLines.data + i;
			LineDef* l  = map->lines.data + i;

			l->v1 = ml->v1;
			l->v2 = ml->v2;
			l->special = ml->special;
			l->flags = ml->flags;
			l->tag = ml->tag;
			l->sidenum[0] = ml->sidenum[0];
			l->sidenum[1] = ml->sidenum[1];
		}

		logMessage("\tLoaded %i lines", linesLookup.lump.length);
	}

	// Segs
	{
		auto segsLookup = getLumpByNum(lumpNum, (int)MapLumps::Segs);
		if (segsLookup.result != WadResult::Success || strncmp(segsLookup.name, "SEGS\0", 8) != 0) return result;

		Slice<MapSeg> mapSegs;
		mapSegs.data = (MapSeg*)segsLookup.lump.data;
		mapSegs.length = segsLookup.lump.length / sizeof(MapSeg);

		map->segs.data = (Seg*)memoryAlloc(level, sizeof(Seg) * mapSegs.length);
		map->segs.length = mapSegs.length;

		for (int i = 0; i < mapSegs.length; ++i) {
			MapSeg *ms = mapSegs.data + i;
			Seg *s = map->segs.data + i;

			s->v1 = ms->v1;
			s->v2 = ms->v2;
			s->xoffset = ms->xoffset;
			s->linedef = ms->linedef;
			s->side = ms->side;

			if (s->side < 0 || s->side > 1) {
				logMessage("Seg %i side out of range (value: %i)", i, s->side);
				return result;
			}

			{
				Vertex *v1, *v2;
				v1 = map->vertexes.data + s->v1;
				v2 = map->vertexes.data + s->v2;

				f32 dx = (v2->x - v1->x);
				f32 dy = (v2->y - v1->y);

				s->length = sqrtf(dx * dx + dy * dy);
			}

			LineDef* line = map->lines.data + s->linedef;

			s->fronsector = map->sides[line->sidenum[s->side]].sector;
			if (line->flags & (i32)LineFlags::TwoSided && line->sidenum[s->side ^ 1] != -1) {
				s->backsector = map->sides[line->sidenum[s->side ^ 1]].sector;
			}
			else {
				s->backsector = -1;
			}
		}

		logMessage("\tLoaded %i segs", segsLookup.lump.length);
	}

	// Subsectors
	{
		auto ssecLookup = getLumpByNum(lumpNum, (int)MapLumps::SubSectors);
		if (ssecLookup.result != WadResult::Success || strncmp(ssecLookup.name, "SSECTORS", 8) != 0) return result;

		Slice<MapSubsector> mapSubSectors;
		mapSubSectors.data = (MapSubsector*)ssecLookup.lump.data;
		mapSubSectors.length = ssecLookup.lump.length / sizeof(MapSubsector);

		map->subsectors.data = (SubSector*)memoryAlloc(level, sizeof(SubSector) * mapSubSectors.length);
		map->subsectors.length = mapSubSectors.length;

		for (int i = 0; i < mapSubSectors.length; ++i) {
			SubSector *ssec = map->subsectors.data + i;
			MapSubsector *mssec = mapSubSectors.data + i;

			ssec->firstseg = mssec->firstSeg;
			ssec->numsegs = mssec->numSegs;

			Seg* seg = map->segs.data + ssec->firstseg;
			ssec->sector = seg->fronsector;
		}

		logMessage("\tLoaded %i subsectors", ssecLookup.lump.length);
	}

	// Nodes
	{
		auto nodesLookup = getLumpByNum(lumpNum, (int)MapLumps::Nodes);
		if (nodesLookup.result != WadResult::Success || strncmp(nodesLookup.name, "NODES", 8) != 0) return result;

		Slice<MapNode> mapNodes;
		mapNodes.data = (MapNode*)nodesLookup.lump.data;
		mapNodes.length = nodesLookup.lump.length / sizeof(MapNode);

		map->nodes.data = (Node*)memoryAlloc(level, sizeof(Node) * mapNodes.length);
		map->nodes.length = mapNodes.length;

		for (int i = 0; i < mapNodes.length; ++i) {
			MapNode* mn = mapNodes.data + i;
			Node*    n = map->nodes.data + i;

			n->x = (f32)mn->x;
			n->y = (f32)mn->y;
			n->dx = (f32)mn->dx;
			n->dy = (f32)mn->dy;

			for (int j = 0; j < 2; ++j) {
				n->children[j] = mn->children[j];

				for (int k = 0; k < 4; ++k) {
					n->bbox[j][k] = (f32)mn->bbox[j][k];
				}
			}
		}

		logMessage("\tLoaded %i nodes", nodesLookup.lump.length);
	}

	result.result = MapResult::Success;
	result.map = map;

	return result;
}
