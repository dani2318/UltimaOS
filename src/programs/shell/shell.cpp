#include <stdint.h>

// This matches the structure we will pass from the kernel
struct KernelAPI {
    void (*Print)(const char*);
    char (*GetChar)();
    void (*Exit)();
    void (*Clear)(uint32_t);
};

struct KernelAPI* g_api;

// Standard string comparison since we don't have a library yet
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

#include <stdint.h>

// Define our syscall numbers to match the kernel
#define SYS_PRINT    0
#define SYS_EXIT     1
#define SYS_GETCHAR  2
#define SYS_CLEAR    3

static inline uint64_t do_syscall(uint64_t num, uint64_t a1=0, uint64_t a2=0, uint64_t a3=0) {
    uint64_t ret;
    asm volatile (
        "mov %1, %%rax\n"      // syscall number
        "mov %2, %%rdi\n"      // arg1
        "mov %3, %%rsi\n"      // arg2
        "mov %4, %%rdx\n"      // arg3
        "syscall\n"
        : "=a"(ret)
        : "r"(num), "r"(a1), "r"(a2), "r"(a3)
        : "rcx", "r11", "memory"
    );
    return ret;
}

// User-friendly wrappers
void print(const char* s) { do_syscall(SYS_PRINT, (uint64_t)s); }
void clear(uint32_t color) { do_syscall(SYS_CLEAR, (uint64_t)color); }
char getch() { return (char)do_syscall(SYS_GETCHAR); }
void exit() { do_syscall(SYS_EXIT); }

extern "C" void _start() {
    char command_buffer[128];
    int idx = 0;

    while (true) {
        // 1. Print Prompt
        do_syscall(0, (uint64_t)"> ");

        // 2. Read characters until Enter
        while (true) {
            char c = (char)do_syscall(2); // GET_CHAR is Syscall 2
            
            if (c == 0) continue; // No key pressed yet, keep polling

            if (c == '\n') {
                command_buffer[idx] = '\0';
                do_syscall(0, (uint64_t)"\n"); // Echo newline
                break;
            } else if (idx < 127) {
                command_buffer[idx++] = c;
                // Echo the character back to the screen
                char echo[2] = {c, '\0'};
                do_syscall(0, (uint64_t)echo);
            }
        }

        // 3. Process Command
        if (strcmp(command_buffer, "help") == 0) {
            do_syscall(0, (uint64_t)"Available commands: help, clear, exit\n");
        } else if (strcmp(command_buffer, "exit") == 0) {
            do_syscall(1); // EXIT is Syscall 1
        } else {
            do_syscall(0, (uint64_t)"Unknown command.\n");
        }

        idx = 0; // Reset buffer for next command
    }
}