#pragma once

#include "types.h"

struct v2f {
	f32 x, y;

	inline v2f& operator*=(f32 s) {
		this->x *= s;
		this->y *= s;
		return *this;
	}

	inline v2f& operator/=(f32 s) {
		this->x /= s;
		this->y /= s;
		return *this;
	}

	inline v2f& operator+=(f32 s) {
		this->x += s;
		this->y += s;
		return *this;
	}

	inline v2f& operator+=(v2f rhs) {
		this->x += rhs.x;
		this->y += rhs.y;
		return *this;
	}

	inline v2f& operator-=(v2f rhs) {
		this->x -= rhs.x;
		this->y -= rhs.y;
		return *this;
	}
};

inline v2f operator *(f32 s, const v2f vec) {
	return {
		vec.x * s,
		vec.y * s
	};
}

inline v2f operator *(const v2f vec, const f32 s) {
	return {
		vec.x * s,
		vec.y * s
	};
}

inline v2f operator /(const v2f vec, const f32 s) {
	return {
		vec.x / s,
		vec.y / s
	};
}

inline v2f operator +(const v2f lhs, const v2f rhs) {
	return {
		lhs.x + rhs.x,
		lhs.y + rhs.y
	};
}

inline v2f operator -(const v2f lhs, const v2f rhs) {
	return {
		lhs.x - rhs.x,
		lhs.y - rhs.y
	};
}

inline v2f operator -(const v2f rhs) {
	return {
		-rhs.x,
		-rhs.y
	};
}


struct v3f {
	f32 x, y, z;
};



inline i32 max(i32 a, i32 b) {
	return a > b ? a : b;
}

inline i32 min(i32 a, i32 b) {
	return a < b ? a : b;
}

inline f32 square(f32 a) {
	return a * a;
}

