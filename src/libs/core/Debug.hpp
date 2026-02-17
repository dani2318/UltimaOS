#pragma once
#include <core/dev/TextDevice.hpp>

namespace Debug {

    enum class Level {
        DEBUG = 0,
        INFO  = 1,
        WARN = 2,
        ERROR = 3,
        CRITICAL = 4
    };

    void AddOutDevice(Debug::Level minLogLevel,  bool colorOutput, TextDevice* device);
    void Debug(const char* module, const char* fmt, ...);
    void Info(const char* module, const char* fmt, ...);
    void Warn(const char* module, const char* fmt, ...);
    void Error(const char* module, const char* fmt, ...);
    void Critical(const char* module, const char* fmt, ...);

}