#include "types.h"
#include "memory.h"
#include "wad.h"
#include "system.h"

#include "stdio.h"

struct WadInfo {
	u8   wadId[4];
	u32  numLumps;
	u32  infoTableOffset;
};

struct LumpInfo {
	u32 position;
	u32 size;
	i8  name[8];
};

struct WadFile {
	const char* name;
	u8*         data;
	WadInfo     info;
	LumpInfo*   directory;
};


static WadFile*  wadFiles = 0;
static usize     numLoadedWads = 0;
static usize     maxWads = 0;


void initWads() {
	maxWads = 127;
	u8* memory = memoryAlloc(permanent, sizeof(WadFile) * maxWads);

	wadFiles = (WadFile *)memory;
	numLoadedWads = 0;
}


WadResult loadWadFile(const char *name) {
	if(numLoadedWads >= maxWads) return WadResult::Failure;

	FILE* f;
	auto result = fopen_s(&f, name, "rb");

	if(result == 0 ) {
		fseek(f, 0, SEEK_END);
		auto size = ftell(f);
		fseek(f, 0, SEEK_SET);

		if(size) {
			auto mem = memoryAlloc(permanent, size);

			fread(mem, 1, size, f);

			WadFile *file = wadFiles + numLoadedWads;

			file->info = *((WadInfo*)mem);
			file->name = name;
			file->data = mem;

			if(file->info.infoTableOffset + (sizeof(LumpInfo) * file->info.numLumps) <= size) {
				file->directory = (LumpInfo*)(mem + file->info.infoTableOffset);
				numLoadedWads++;
				return WadResult::Success;
			}
		}

		fclose(f);
	}

	return WadResult::Failure;
}


const i32 LUMP_MASK = 0xFFFFFF;
const i32 WAD_MASK  = 0x7F000000;
const i32 WAD_SHIFT = 24;

static inline i32 packLumpNum(i32 wadIndex, i32 lumpIndex) {
	return ((wadIndex << WAD_SHIFT) & WAD_MASK) | lumpIndex;
}

static inline i32 unpackWadIndex(i32 lumpNum) {
	return lumpNum >> WAD_SHIFT;
}

static inline i32 unpackLumpIndex(i32 lumpNum) {
	return lumpNum & LUMP_MASK;
}


LumpNum findLumpByName(const char* name) {
	LumpNum result = -1;

	const u64 id = *((const u64*)name);

	for(int i = numLoadedWads - 1; i >= 0; --i) {
		auto wad = wadFiles[i];

		for(u32 p = 0; p < wad.info.numLumps; ++p) {

			const u64 lid = *((u64*)wad.directory[p].name);

			if(lid != id) continue;

			result = packLumpNum(i, p);

			return result;
		}
	}

	return result;
}


LumpResult getLumpByNum(LumpNum lumpNum, usize offset) {
	LumpResult result = {
		WadResult::Failure
	};


	if (lumpNum != -1) {
		auto wadIndex = unpackWadIndex(lumpNum);
		auto lumpIndex = unpackLumpIndex(lumpNum) + offset;
		if (wadIndex >= numLoadedWads) fatalError("getNameOfLump given invalid lumpNum: wadIndex out of range");

		auto wad = wadFiles[wadIndex];
		if (lumpIndex < 0 || lumpIndex >= wad.info.numLumps) fatalError("getNameOfLump given invalude lumpNum: lumpIndex out of range");

		result.result = WadResult::Success;
		result.lump.data   = wad.data + wad.directory[lumpIndex].position;
		result.lump.length = wad.directory[lumpIndex].size;
		result.name        = wad.directory[lumpIndex].name;
	}

	return result;
}


LumpResult getLumpByName(const char* name) {
	auto lumpNum = findLumpByName(name);

	return getLumpByNum(lumpNum);
}


#include "string.h"

Array<LumpNum> findMapLumps() {
	Array<LumpNum> result;

	result.capacity = 50;
	result.data = (LumpNum*)memoryAlloc(permanent, sizeof(LumpNum) * result.capacity);
	result.length = 0;

	for (i32 i = 0; i < numLoadedWads; ++i) {
		auto wad = wadFiles[i];

		for (i32 p = 0; p < wad.info.numLumps; ++p) {
			if (wad.info.numLumps - p <= 11) break;

			if (strncmp(wad.directory[p + (int)MapLumps::Things].name,     "THINGS\0\0",   8) != 0) continue;
			if (strncmp(wad.directory[p + (int)MapLumps::Linedefs].name,   "LINEDEFS",     8) != 0) continue;
			if (strncmp(wad.directory[p + (int)MapLumps::Sidedefs].name,   "SIDEDEFS",     8) != 0) continue;
			if (strncmp(wad.directory[p + (int)MapLumps::Vertexes].name,   "VERTEXES",     8) != 0) continue;
			if (strncmp(wad.directory[p + (int)MapLumps::Segs].name,       "SEGS\0\0\0\0", 8) != 0) continue;
			if (strncmp(wad.directory[p + (int)MapLumps::SubSectors].name, "SSECTORS",     8) != 0) continue;
			if (strncmp(wad.directory[p + (int)MapLumps::Nodes].name,      "NODES\0\0\0",  8) != 0) continue;
			if (strncmp(wad.directory[p + (int)MapLumps::Sectors].name,    "SECTORS\0",    8) != 0) continue;
			if (strncmp(wad.directory[p + (int)MapLumps::Reject].name,     "REJECT\0\0",   8) != 0) continue;
			if (strncmp(wad.directory[p + (int)MapLumps::Blockmap].name,   "BLOCKMAP",     8) != 0) continue;

			assert(result.length < result.capacity);

			result.data[result.length] = packLumpNum(i, p);
			result.length++;
		}
	}

	return result;
}