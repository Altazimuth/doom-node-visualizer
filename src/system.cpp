#include "stdio.h"
#include "stdarg.h"
#include "assert.h"

static const int size = 1024;
static char fmtBuffer[size];


void logMessage(const char* format, ...) {
	va_list aptr;

	va_start(aptr, format);
	auto ret = vsnprintf(fmtBuffer, size, format, aptr);
	va_end(aptr);

	if(ret > 0) {
		printf("%s\n", fmtBuffer);
	}
}


void reportFatalError(const char* format, ...) {
	va_list aptr;

	va_start(aptr, format);
	auto ret = vsnprintf(fmtBuffer, size, format, aptr);
	va_end(aptr);

	if(ret > 0) {
		printf("FATAL: %s\n", fmtBuffer);
	}

	assert(false);
}