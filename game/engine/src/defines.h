#pragma once

#include "core/Logger/Logger.h"


// Unsigned int types.
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

// Signed int types.
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

// Floating point types
typedef float f32;
typedef double f64;

// Boolean types
typedef int b32;
typedef char b8;

// Properly define static assertions.
#if defined(__clang__) || defined(__gcc__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif

// Ensure all types are of the correct size.
STATIC_ASSERT(sizeof(u8) == 1, "Expected u8 to be 1 byte.");
STATIC_ASSERT(sizeof(u16) == 2, "Expected u16 to be 2 bytes.");
STATIC_ASSERT(sizeof(u32) == 4, "Expected u32 to be 4 bytes.");
STATIC_ASSERT(sizeof(u64) == 8, "Expected u64 to be 8 bytes.");

STATIC_ASSERT(sizeof(i8) == 1, "Expected i8 to be 1 byte.");
STATIC_ASSERT(sizeof(i16) == 2, "Expected i16 to be 2 bytes.");
STATIC_ASSERT(sizeof(i32) == 4, "Expected i32 to be 4 bytes.");
STATIC_ASSERT(sizeof(i64) == 8, "Expected i64 to be 8 bytes.");

STATIC_ASSERT(sizeof(f32) == 4, "Expected f32 to be 4 bytes.");
STATIC_ASSERT(sizeof(f64) == 8, "Expected f64 to be 8 bytes.");

#define TRUE 1
#define FALSE 0

// Platform detection
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) 
#define EM_PLATFORM_WINDOWS 1
#ifndef _WIN64
#error "64-bit is required on Windows!"
#endif
#endif

#ifdef EM_EXPORT
// Exports
#ifdef _MSC_VER
#define EM_API __declspec(dllexport)
#else
#define EM_API __attribute__((visibility("default")))
#endif
#else
// Imports
#ifdef _MSC_VER
#define EM_API __declspec(dllimport)
#else
#define EM_API
#endif
#endif

//logging

#define LOG_WARN_ENABLED 1
#define LOG_INFO_ENABLED 1
#define LOG_DEBUG_ENABLED 1
#define LOG_TRACE_ENABLED 1

#if EM_RELEASE == 1
#define LOG_DEBUG_ENABLED 0
#define LOG_TRACE_ENABLED 0
#endif

#define EM_FATAL(message, ...) {\
    Logger::Log(LogLevel::LOG_LEVEL_FATAL , message, ##__VA_ARGS__);\
}

#ifndef EM_ERROR
#define EM_ERROR(message, ...)  {\
    Logger::Log(LogLevel::LOG_LEVEL_ERROR, message, ##__VA_ARGS__);\
}
#endif

#if LOG_WARN_ENABLED == 1
#define EM_WARN(message, ...)  \
{\
     Logger::Log(LogLevel::LOG_LEVEL_WARN, message, ##__VA_ARGS__);\
}
#else
#define EM_WARN(message, ...)
#endif

#if LOG_INFO_ENABLED == 1
#define EM_INFO(message, ...) \
{\
     Logger::Log(LogLevel::LOG_LEVEL_INFO, message, ##__VA_ARGS__);\
}
#else
#define EM_INFO(message, ...)
#endif

#if LOG_DEBUG_ENABLED == 1
#define EM_DEBUG(message, ...) {\
     Logger::Log(LogLevel::LOG_LEVEL_DEBUG, message, ##__VA_ARGS__);\
}
#else
#define EM_DEBUG(message, ...)
#endif

#if LOG_TRACE_ENABLED == 1
#define EM_TRACE(message, ...) {\
     Logger::Log(LogLevel::LOG_LEVEL_TRACE, message, ##__VA_ARGS__);\
}
#else
#define EM_TRACE(message, ...)
#endif