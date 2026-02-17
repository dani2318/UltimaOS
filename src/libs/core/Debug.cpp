#include "Debug.hpp"
#include <stdarg.h>

#define MAX_OUTPUT_DEVICES 10
#define MIN_LOG_LEVEL Debug::Level::INFO

namespace{

    static const char* const g_LogSeverityColors[] =
    {
        "\033[2;37m",
        "\033[37m",
        "\033[1;33m",
        "\033[1;31m",
        "\033[1;37;41m",
    };

    static const char* const g_ColorReset = "\033[0m";

    struct 
    {
        Debug::Level logLevel;
        TextDevice* device;
        bool colored;
    }g_OutputDevices[MAX_OUTPUT_DEVICES];
    int OutputDevicesCount;
}

namespace Debug {

    void AddOutDevice(Debug::Level minLogLevel, bool colorOutput,TextDevice* device){
        g_OutputDevices[OutputDevicesCount].device = device;
        g_OutputDevices[OutputDevicesCount].logLevel = minLogLevel;
        g_OutputDevices[OutputDevicesCount].colored = colorOutput;
        OutputDevicesCount++;
    }

    static void Log(const char* module,Debug::Level logLevel, const char* fmt, va_list args){
        for(int i = 0; i < OutputDevicesCount; i++){
            if (logLevel < g_OutputDevices[i].logLevel)
                continue;

            if (g_OutputDevices[i].colored)
                g_OutputDevices[i].device->Write(g_LogSeverityColors[static_cast<int>(logLevel)]);
            
            g_OutputDevices[i].device->Format("[%s] ", module);
            g_OutputDevices[i].device->VFormat(fmt, args);
            
            if (g_OutputDevices[i].colored)
                g_OutputDevices[i].device->Write(g_ColorReset);
            
            g_OutputDevices[i].device->Write('\n');
        }
    }

    void Debug(const char* module, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        Log(module, Debug::Level::DEBUG, fmt, args);
        va_end(args);
    }

    void Info(const char* module, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        Log(module, Debug::Level::INFO, fmt, args);
        va_end(args);
    }

    void Warn(const char* module, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        Log(module, Debug::Level::WARN, fmt, args);
        va_end(args);
    }

    void Error(const char* module, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        Log(module, Debug::Level::ERROR, fmt, args);
        va_end(args);
    }

    void Critical(const char* module, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        Log(module, Debug::Level::CRITICAL, fmt, args);
        va_end(args);
    }
}