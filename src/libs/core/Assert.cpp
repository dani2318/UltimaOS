#include "Assert.hpp"
#include <arch/i686/IO.hpp>
#include <Debug.hpp>

bool Assert_(const char* cond, const char* file, int line, const char* function) {
    //TODO: Print debug message!
    Debug::Critical("Assert", "Assertion failed: %s", cond);
    Debug::Critical("Assert", "In file: %s line %d function %s", file, line, function);
    arch::i686::Panic();
    return false;
}