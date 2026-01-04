#include "Keyboard.hpp"
#include <arch/x86_64/Serial.hpp>

// Scancode Set 1 to ASCII mapping (US keyboard layout)
static const char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, // Control
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, // Left shift
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0, // Right shift
    '*',
    0, // Alt
    ' ', // Space
    0, // Caps lock
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // F1-F10
};

static const char scancode_to_ascii_shift[] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, // Control
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, // Left shift
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
    0, // Right shift
    '*',
    0, // Alt
    ' ', // Space
    0, // Caps lock
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // F1-F10
};

// Static member initialization
char Keyboard::buffer[BUFFER_SIZE];
int Keyboard::buffer_start = 0;
int Keyboard::buffer_end = 0;
bool Keyboard::shift_pressed = false;
bool Keyboard::ctrl_pressed = false;
bool Keyboard::alt_pressed = false;
bool Keyboard::caps_lock = false;

void Keyboard::Initialize() {
    buffer_start = 0;
    buffer_end = 0;
    shift_pressed = false;
    ctrl_pressed = false;
    alt_pressed = false;
    caps_lock = false;
}

void Keyboard::OnInterrupt() {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    HandleScancode(scancode);
}

void Keyboard::HandleScancode(uint8_t scancode) {
    // Check if this is a key release (bit 7 set)
    if (scancode & 0x80) {
        // Key release
        scancode &= 0x7F; // Remove release bit
        
        switch(scancode) {
            case 0x2A: // Left shift
            case 0x36: // Right shift
                shift_pressed = false;
                break;
            case 0x1D: // Control
                ctrl_pressed = false;
                break;
            case 0x38: // Alt
                alt_pressed = false;
                break;
        }
    } else {
        // Key press
        switch(scancode) {
            case 0x2A: // Left shift
            case 0x36: // Right shift
                shift_pressed = true;
                break;
            case 0x1D: // Control
                ctrl_pressed = true;
                break;
            case 0x38: // Alt
                alt_pressed = true;
                break;
            case 0x3A: // Caps lock
                caps_lock = !caps_lock;
                break;
            default: {
                // Convert to character
                char c = ScancodeToChar(scancode);
                if (c != 0) {
                    // Add to buffer
                    int next = (buffer_end + 1) % BUFFER_SIZE;
                    if (next != buffer_start) { // Buffer not full
                        buffer[buffer_end] = c;
                        buffer_end = next;
                    }
                }
                break;
            }
        }
    }
}

char Keyboard::ScancodeToChar(uint8_t scancode) {
    if (scancode >= sizeof(scancode_to_ascii)) {
        return 0;
    }
    
    char c;
    if (shift_pressed) {
        c = scancode_to_ascii_shift[scancode];
    } else {
        c = scancode_to_ascii[scancode];
    }
    
    // Apply caps lock for letters
    if (caps_lock && c >= 'a' && c <= 'z') {
        c = c - 'a' + 'A';
    } else if (caps_lock && c >= 'A' && c <= 'Z') {
        c = c - 'A' + 'a';
    }
    
    return c;
}

char Keyboard::GetChar() {
    // Wait until character available
    while (!HasChar()) {
        asm volatile("hlt");
    }
    
    // Get character from buffer
    char c = buffer[buffer_start];
    buffer_start = (buffer_start + 1) % BUFFER_SIZE;
    return c;
}

bool Keyboard::HasChar() {
    return buffer_start != buffer_end;
}

void Keyboard::ClearBuffer() {
    buffer_start = 0;
    buffer_end = 0;
}