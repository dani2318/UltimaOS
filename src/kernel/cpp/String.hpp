#pragma once
#include <stdint.h>

#include <stdint.h>
#include <stddef.h>

extern "C" {

// Finds the first occurrence of character 'c' in string 's'
static char* strchr(const char* s, int c) {
    while (*s != (char)c) {
        if (!*s++) {
            return nullptr;
        }
    }
    return (char*)s;
}

// Implementation of strtok
static char* strtok(char* str, const char* delimiters) {
    static char* lastToken = nullptr;
    
    // If str is provided, start a new scan. Otherwise, continue from lastToken.
    if (str) {
        lastToken = str;
    } else if (!lastToken) {
        return nullptr;
    }

    // Skip leading delimiters
    while (*lastToken && strchr(delimiters, *lastToken)) {
        lastToken++;
    }

    if (*lastToken == '\0') {
        lastToken = nullptr;
        return nullptr;
    }

    // Found the start of a token
    char* tokenStart = lastToken;

    // Find the end of the token
    while (*lastToken && !strchr(delimiters, *lastToken)) {
        lastToken++;
    }

    if (*lastToken) {
        // Terminate the token and advance lastToken for the next call
        *lastToken = '\0';
        lastToken++;
    } else {
        lastToken = nullptr;
    }

    return tokenStart;
}

}

// String length
inline size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

// String compare
inline int strcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

// String copy
inline char* strcpy(char* dest, const char* src) {
    char* original_dest = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return original_dest;
}

// String concatenate
inline char* strcat(char* dest, const char* src) {
    char* original_dest = dest;
    while (*dest) dest++;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return original_dest;
}


// String compare (first n characters)
inline int strncmp(const char* str1, const char* str2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (str1[i] != str2[i] || str1[i] == '\0') {
            return (unsigned char)str1[i] - (unsigned char)str2[i];
        }
    }
    return 0;
}

inline char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;

    // Copy characters from src to dest
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }

    // If we haven't reached 'n', fill the rest with null bytes
    // This is required by the C standard
    for (; i < n; i++) {
        dest[i] = '\0';
    }

    return dest;
}

static inline char* itoa(int64_t value, char* str, int base) {
    // Handle invalid base
    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }
    
    char* ptr = str;
    char* ptr1 = str;
    char tmp_char;
    int64_t tmp_value;
    
    // Handle negative numbers for base 10
    bool negative = false;
    if (value < 0 && base == 10) {
        negative = true;
        value = -value;
    }
    
    // Handle 0 explicitly
    if (value == 0) {
        *ptr++ = '0';
        *ptr = '\0';
        return str;
    }
    
    // Process individual digits
    while (value) {
        tmp_value = value;
        value /= base;
        *ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[tmp_value - value * base];
    }
    
    // Add negative sign for base 10
    if (negative) {
        *ptr++ = '-';
    }
    
    *ptr-- = '\0';
    
    // Reverse the string
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
    
    return str;
}

// Also add an unsigned version for uint64_t
static inline char* utoa(uint64_t value, char* str, int base) {
    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }
    
    char* ptr = str;
    char* ptr1 = str;
    char tmp_char;
    uint64_t tmp_value;
    
    if (value == 0) {
        *ptr++ = '0';
        *ptr = '\0';
        return str;
    }
    
    while (value) {
        tmp_value = value;
        value /= base;
        *ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[tmp_value - value * base];
    }
    
    *ptr-- = '\0';
    
    // Reverse
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
    
    return str;
}