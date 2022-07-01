#include "memory.h"
#include "types.h"
#include "system.h"

#include <stdlib.h>

struct MemoryBook {
	u8*   data;
	usize size;
};

struct MemoryArena {
	u8*   data;
	usize size;
	u8*   freePtr;
};


const usize MAIN_BLOCK_SIZE = MEGABYTES(128);
static MemoryBook mainMemoryBlock = {};

static MemoryArena permanentStorage = {};
static MemoryArena levelStorage = {};
static MemoryArena temporaryStorage = {};

MemoryArena* permanent = 0;
MemoryArena* level = 0;
MemoryArena* temporary = 0;

static i64 mostTemporaryStorageUsed = 0;


void initMemory() {
	auto block = (u8*)malloc(MAIN_BLOCK_SIZE);

	if (!block) {
		fatalError("Failed to allocate main block of memory.");
	}

	mainMemoryBlock.data = block;
	mainMemoryBlock.size = MAIN_BLOCK_SIZE;

	usize offset = 0;

	permanentStorage.data = permanentStorage.freePtr = block + offset;
	permanentStorage.size = MEGABYTES(32);

	offset += permanentStorage.size;

	levelStorage.data = levelStorage.freePtr = block + offset;
	levelStorage.size = MEGABYTES(32);

	offset += levelStorage.size;

	temporaryStorage.data = temporaryStorage.freePtr = block + offset;
	temporaryStorage.size = mainMemoryBlock.size - offset;

	permanent = &permanentStorage;
	temporary = &temporaryStorage;
	level = &levelStorage;
}


void resetArena(MemoryArena *arena) {
	if (arena == temporary && arena->freePtr - arena->data > mostTemporaryStorageUsed) {
		mostTemporaryStorageUsed = arena->freePtr - arena->data;
	}

	arena->freePtr = arena->data;
}


u8* memoryAlloc(MemoryArena *arena, usize size) {
	if (!arena || (arena->freePtr - arena->data) + size >= arena->size) {
		fatalError("Failed to allocate from arena");
	}

	u8* result = arena->freePtr;
	arena->freePtr += size;

	return result;
}


void reportMemoryStats() {
	if (temporary->freePtr - temporary->data > mostTemporaryStorageUsed) {
		mostTemporaryStorageUsed = temporary->freePtr - temporary->data;
	}

	logMessage("Main Memory Block: %i kb", mainMemoryBlock.size / 1024);
	logMessage("Permanent storage: %i of %i kb used", (permanentStorage.freePtr - permanentStorage.data) / 1024, permanentStorage.size / 1024);
	logMessage("Level storage: %i of %i kb used", (levelStorage.freePtr - levelStorage.data) / 1024, levelStorage.size / 1024);
	logMessage("Most temporary storage used: %i of %i kb", mostTemporaryStorageUsed / 1024, temporaryStorage.size / 1024);
}