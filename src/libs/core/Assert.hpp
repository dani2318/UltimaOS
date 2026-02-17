#pragma once


#define Assert(cond) ((cond) || Assert_(#cond, __FILE__, __LINE__, __FUNCTION__))

bool Assert_(const char* cond, const char* file, int line, const char* function);