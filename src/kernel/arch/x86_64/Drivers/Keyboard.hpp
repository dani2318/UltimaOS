#pragma once
#include <stdint.h>

#define KEYBOARD_DATA_PORT    0x60
#define KEYBOARD_STATUS_PORT  0x64

class Keyboard {
public:
    static void Initialize();
    static void OnInterrupt();
    static char GetChar();          // Blocking read
    static bool HasChar();          // Check if character available
    static void ClearBuffer();
    
private:
    static void HandleScancode(uint8_t scancode);
    static char ScancodeToChar(uint8_t scancode);
    
    // Input buffer
    static const int BUFFER_SIZE = 256;
    static char buffer[BUFFER_SIZE];
    static int buffer_start;
    static int buffer_end;
    
    // Keyboard state
    static bool shift_pressed;
    static bool ctrl_pressed;
    static bool alt_pressed;
    static bool caps_lock;
};