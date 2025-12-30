#pragma once

typedef void (*constructor)();
extern "C" constructor __ctors_start;
extern "C" constructor __ctors_end;