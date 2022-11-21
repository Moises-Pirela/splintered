#include "Logger.h"
#include <cstring>
#include <stdarg.h> 
#include <stdio.h>

#if EM_PLATFORM_WINDOWS

#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>

#endif

void Logger::Log(LogLevel level, const char* message, ...)
{
    const char* level_strings[6] = {"[FATAL]: ", "[ERROR]: ", "[WARN]: ", "[INFO]: ", "[DEBUG]: ", "[TRACE]: "};

    b8 isError = level < LOG_LEVEL_WARN;
    
    const i32 msgLength = 32000;
    char output[msgLength];

    memset(output,0,sizeof(output));

    va_list arg_ptr;
    va_start(arg_ptr, message);
    vsnprintf(output, 32000, message, arg_ptr);
    va_end(arg_ptr);

    char output2[32000];
    sprintf(output2, "%s%s\n", level_strings[level], output);

    if (!isError){
        Write(output2, level);
    }
    else{
        WriteError(output2, level);
    }
}

void Logger::Write(const char* message, u8 color)
{
    HANDLE hcon = GetStdHandle(STD_ERROR_HANDLE);

    static u8 logLevels[6] = {64, 4, 6, 2, 1, 8};

    SetConsoleTextAttribute(hcon, logLevels[color]);

    OutputDebugStringA(message);
    u64 length = strlen(message);
    LPDWORD number_written = 0;

    WriteConsoleA(hcon, message, (DWORD)length, number_written, 0);
}

void Logger::WriteError(const char* message, u8 color)
{
    HANDLE hcon = GetStdHandle(STD_ERROR_HANDLE);

    static u8 logLevels[6] = {64, 4, 6, 2, 1, 8};

    SetConsoleTextAttribute(hcon, logLevels[color]);

    OutputDebugStringA(message);
    u64 length = strlen(message);
    LPDWORD number_written = 0;

    WriteConsoleA(hcon, message, (DWORD)length, number_written, 0);
}

