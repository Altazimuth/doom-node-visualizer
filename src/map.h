#pragma once

#include "types.h"

enum class MapResult {
	Success,
	NotFound,
	InvalidMap
};

struct Sector {
	f32   floorheight, ceilingheight;
	i16 floortex, ceilingtex;
	i16 special;
	i16 tag;
	u8 lightlevel;
};

struct Vertex {
	f32 x, y;
};

struct SideDef {
	f32 xoffset, yoffset;
	i16 topTexture, bottomTexture, midTexture;
	Sector *sector;
};

struct LineDef {
	Vertex *v1, *v2;
	i16 flags;
	i16 special, tag;
	i32 sidenum[2];
};
enum class LineFlags {
	Blocking = 0x1,
	BlockMosnters = 0x2,
	TwoSided = 0x4,
	UpperUnpegged = 0x8,
	LowerUnpegged = 0x10,
	Secret = 0x20,
	SoundBlock = 0x40,
	DontDraw = 0x80,
	Mapped = 0x100
};

struct Seg {
	Vertex *v1, *v2;
	f32 length, xoffset;
	LineDef *linedef;
	SideDef *side;
	Sector *frontsector, *backsector;
};

struct SubSector {
	Sector *sector;
	i32 firstseg, numsegs;
};

const i32 BoxTop = 0;
const i32 BoxBottom = 1;
const i32 BoxLeft = 2;
const i32 BoxRight = 3;

const i32 SubsectorChildFlag = 0x80000000;
struct Node {
	f32 x, y, dx, dy;
	f32 bbox[2][4];
	i32 children[2];
};

struct Map {
	Slice<Sector> sectors;
	Slice<Vertex> vertexes;
	Slice<SideDef> sides;
	Slice<LineDef> lines;
	Slice<Seg> segs;
	Slice<Node> nodes;
	Slice<SubSector> subsectors;
};

struct MapLoad {
	MapResult result;
	Map* map;
};

MapLoad loadMap(LumpNum lumpNum);
