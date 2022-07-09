#pragma once

// Always include this header before the line #define WIN32_LEAN_AND_MEAN
#include <dsound.h>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define IN
#define OUT
#define OPTIONAL

#include <cinttypes>
#include <cassert>

enum Error {
	ERROR_NONE = 0,
	ERROR_FAILURE,
	ERROR_EOF,
	ERROR_READ
};

using byte = uint8_t;

template <class T>
inline void SafeDelete(T **ptr)
{
	if (ptr && *ptr) {
		delete *ptr;
		*ptr = nullptr;
	}
}

template <class T>
inline void SafeRelease(T **ptr)
{
	if (ptr && *ptr) {
		(*ptr)->Release();
		*ptr = nullptr;
	}
}

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);   \
  void operator=(const TypeName&)


#include <cstdio>// _vsnprintf_s

inline void DebugPrintfA(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	const size_t kCapacity = 512;
	char buf[kCapacity];
	auto n = _vsnprintf_s((char *)buf, kCapacity, kCapacity - 1, format, args);

	OutputDebugStringA(buf);

	va_end(args);
}