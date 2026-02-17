#include "IO.hpp"

#define UNUSED_PORT 0x80

void iowait(){
    arch::i686::Outb(UNUSED_PORT, 0);
}