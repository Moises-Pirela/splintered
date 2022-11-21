#pragma once

#include "defines.h"

enum LogLevel
{
    LOG_LEVEL_FATAL = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4,
    LOG_LEVEL_TRACE = 5,
};

class Logger
{
public:

    static void Log(LogLevel level, const char* message, ...);

    private:
    
    static void Write(const char* message, u8 color);
    static void WriteError(const char* message, u8 color);
};