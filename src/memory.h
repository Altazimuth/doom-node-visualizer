#pragma once

#include "types.h"

#define KILOBYTES(num) (num * 1024)
#define MEGABYTES(num) (KILOBYTES(num) * 1024)
#define GIGABYTES(num) (MEGABYTES(num) * 1024)



void initMemory();

struct MemoryArena;

extern MemoryArena *permanent;
extern MemoryArena *level;
extern MemoryArena *temporary;

u8* rawArenaAlloc(MemoryArena* arena, usize size);
void resetArena(MemoryArena* arena);

template<typename T>
T* arenaAlloc(MemoryArena* arena, usize capacity = 1) {
	return (T*)rawArenaAlloc(arena, sizeof(T) * capacity);
}

void reportMemoryStats();
