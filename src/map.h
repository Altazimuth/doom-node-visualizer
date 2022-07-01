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
	i16 xoffset, yoffset;
	i16 topTexture, bottomTexture, midTexture;
	i16 sector;
};

struct LineDef {
	i16 v1, v2;
	i16 flags;
	i16 special, tag;
	i16 sidenum[2];
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
	i16 v1, v2;
	f32 length, xoffset;
	i16 linedef, side;
	i16 frontsector, backsector;
};

struct SubSector {
	i16 sector;
	i16 firstseg, numsegs;
};

const i32 BoxTop = 0;
const i32 BoxBottom = 1;
const i32 BoxLeft = 2;
const i32 BoxRight = 3;

const i16 SubsectorChildFlag = 0x8000;
struct Node {
	f32 x, y, dx, dy;
	f32 bbox[2][4];
	i16 children[2];
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
