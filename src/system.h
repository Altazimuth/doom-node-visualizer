#pragma once

#include "assert.h"

void logMessage(const char* format, ...);

void reportFatalError(const char* format, ...);

#define fatalError(fmt, ...) { reportFatalError(fmt, ##__VA_ARGS__); assert(false);}
