#include "debug.h"
static const char* const g_LogSeverityColors[] =
{
    [LVL_DEBUG]        = "\033[2;37m",
    [LVL_INFO]         = "\033[37m",
    [LVL_WARN]         = "\033[1;33m",
    [LVL_ERROR]        = "\033[1;31m",
    [LVL_CRITICAL]     = "\033[1;37;41m",
};

static const char* const g_ColorReset = "\033[0m";

void logf(const char* module, DebugLevel level, const char* fmt, ...){
    va_list args;
    if (level < MIN_LOG_LEVEL)
        return;

    fputs(g_LogSeverityColors[level], VFS_FD_DEBUG);
    fprintf(VFS_FD_DEBUG, "[%s] ", module);
    vfprintf(VFS_FD_DEBUG, fmt, args);
    fputs(g_ColorReset, VFS_FD_DEBUG);
    fputc('\n', VFS_FD_DEBUG);

    va_end(args);
}
