#pragma once

#include "types.h"

enum class WadResult {
	Success,
	Failure
};

enum class MapLumps {
	Label,
	Things,
	Linedefs,
	Sidedefs,
	Vertexes,
	Segs,
	SubSectors,
	Nodes,
	Sectors,
	Reject,
	Blockmap
};

void initWads();
WadResult loadWadFile(const char* name);

struct LumpResult {
	WadResult result;
	const i8* name;
	Slice<u8> lump;
};

LumpNum findLumpByName(const char* name);

LumpResult getLumpByName(const char* name);
LumpResult getLumpByNum(LumpNum num, usize offset = 0);

Array<LumpNum> findMapLumps();


