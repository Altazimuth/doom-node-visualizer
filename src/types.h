#pragma once

typedef char          i8;
typedef short         i16;
typedef int           i32;
typedef long long int i64;

typedef unsigned char          u8;
typedef unsigned short         u16;
typedef unsigned int           u32;
typedef unsigned long long int u64;

typedef u64 usize;

typedef i32 fixed32;
const i32 fracBits = 16;
const i32 fixedUnit = 1 << fracBits;

typedef float  f32;
typedef double f64;
const f64 fpFracUnit = 65536.0;

typedef i32 LumpNum;

struct Color {
	u8 r, g, b, a;
};

struct PaletteColor {
	u8 r, g, b;
};

#ifdef DEBUG
#include <assert.h>
#endif

template<typename T>
struct Array {
	usize capacity;
	usize length;
	T*    data;

	const T& operator[](usize index) {
		#ifdef DEBUG
		assert(index < length);
		#endif
		return data[index];
	}
};

template<typename T>
struct Slice {
	usize length;
	T*    data;

	const T& operator[](usize index) {
		#ifdef DEBUG
		assert(index < length);
		#endif
		return data[index];
	}
};

template<typename T>
struct ConstSlice {
	usize    length;
	const T* data;

	const T& operator[](usize index) {
		#ifdef DEBUG
		assert(index < length);
		#endif
		return data[index];
	}
};