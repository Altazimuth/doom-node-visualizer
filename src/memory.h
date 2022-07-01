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

u8* memoryAlloc(MemoryArena *arena, usize size);
void resetArena(MemoryArena *arena);

void reportMemoryStats();
