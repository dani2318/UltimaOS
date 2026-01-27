#pragma once
#include <stdint.h>

#include <stddef.h>

extern "C" {

// Finds the first occurrence of character 'c' in string 's'
static char* strchr(const char* s, int c) {
    while (*s != (char)c) {
        if (*s++ == 0) {
            return nullptr;
        }
    }
    return (char*)s;
}

// Implementation of strtok
static char* strtok(char* str, const char* delimiters) {
    static char* lastToken = nullptr;
    
    // If str is provided, start a new scan. Otherwise, continue from lastToken.
    if (str != nullptr) {
        lastToken = str;
    } else if (lastToken == nullptr) {
        return nullptr;
    }

    // Skip leading delimiters
    while ((*lastToken != 0) && (strchr(delimiters, *lastToken) != nullptr)) {
        lastToken++;
    }

    if (*lastToken == '\0') {
        lastToken = nullptr;
        return nullptr;
    }

    // Found the start of a token
    char* tokenStart = lastToken;

    // Find the end of the token
    while ((*lastToken != 0) && (strchr(delimiters, *lastToken) == nullptr)) {
        lastToken++;
    }

    if (*lastToken != 0) {
        // Terminate the token and advance lastToken for the next call
        *lastToken = '\0';
        lastToken++;
    } else {
        lastToken = nullptr;
    }

    return tokenStart;
}

}

inline size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len] != 0) len++;
    return len;
}

inline int strcmp(const char* str1, const char* str2) {
    while ((*str1 != 0) && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

inline char* strcpy(char* dest, const char* src) {
    char* originalDest = dest;
    while (*src != 0) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return originalDest;
}

inline char* strcat(char* dest, const char* src) {
    char* originalDest = dest;
    while (*dest != 0) dest++;
    while (*src != 0) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return originalDest;
}

// Thread-safe strtok replacement
inline char* strtokR(char* str, const char* delim, char** saveptr) {
    if (str == nullptr) {
        str = *saveptr;
    }
    
    if (str == nullptr || *str == '\0') {
        *saveptr = nullptr;
        return nullptr;
    }
    
    // Skip leading delimiters
    while (*str != 0) {
        bool isDelim = false;
        for (const char* d = delim; *d != 0; d++) {
            if (*str == *d) {
                isDelim = true;
                break;
            }
        }
        if (!isDelim) break;
        str++;
    }
    
    if (*str == '\0') {
        *saveptr = nullptr;
        return nullptr;
    }
    
    // Find end of token
    char* tokenStart = str;
    while (*str != 0) {
        bool isDelim = false;
        for (const char* d = delim; *d != 0; d++) {
            if (*str == *d) {
                isDelim = true;
                break;
            }
        }
        if (isDelim) {
            *str = '\0';
            *saveptr = str + 1;
            return tokenStart;
        }
        str++;
    }
    
    *saveptr = nullptr;
    return tokenStart;
}

// Safe string copy with length limit
inline size_t strlcpy(char* dst, const char* src, size_t size) {
    size_t srcLen = 0;
    
    // Count source length
    while (src[srcLen] != 0) {
        srcLen++;
    }
    
    if (size == 0) {
        return srcLen;
    }
    
    // Copy up to size-1 characters
    size_t const copyLen = (srcLen < size - 1) ? srcLen : size - 1;
    for (size_t i = 0; i < copyLen; i++) {
        dst[i] = src[i];
    }
    dst[copyLen] = '\0';
    
    return srcLen;
}

// Safe string concatenation with length limit
inline size_t strlcat(char* dst, const char* src, size_t size) {
    size_t dstLen = 0;
    size_t srcLen = 0;
    
    // Find end of dst
    while (dstLen < size && (dst[dstLen] != 0)) {
        dstLen++;
    }
    
    // Count source length
    while (src[srcLen] != 0) {
        srcLen++;
    }
    
    if (dstLen >= size) {
        return dstLen + srcLen;
    }
    
    // Append source
    size_t copyLen = size - dstLen - 1;
    if (copyLen > srcLen) {
        copyLen = srcLen;
    }
    
    for (size_t i = 0; i < copyLen; i++) {
        dst[dstLen + i] = src[i];
    }
    dst[dstLen + copyLen] = '\0';
    
    return dstLen + srcLen;
}

// Safe string comparison with length limit
inline int strncmp(const char* s1, const char* s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i]) {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        if (s1[i] == '\0') {
            return 0;
        }
    }
    return 0;
}

// Check if character is in delimiter set
inline bool isDelimiter(char c, const char* delim) {
    for (const char* d = delim; *d != 0; d++) {
        if (c == *d) return true;
    }
    return false;
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
    char tmpChar;
    int64_t tmpValue;
    
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
    while (value != 0) {
        tmpValue = value;
        value /= base;
        *ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[tmpValue - value * base];
    }
    
    // Add negative sign for base 10
    if (negative) {
        *ptr++ = '-';
    }
    
    *ptr-- = '\0';
    
    // Reverse the string
    while (ptr1 < ptr) {
        tmpChar = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmpChar;
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
    char tmpChar;
    uint64_t tmpValue;
    
    if (value == 0) {
        *ptr++ = '0';
        *ptr = '\0';
        return str;
    }
    
    while (value != 0u) {
        tmpValue = value;
        value /= base;
        *ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[tmpValue - value * base];
    }
    
    *ptr-- = '\0';
    
    // Reverse
    while (ptr1 < ptr) {
        tmpChar = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmpChar;
    }
    
    return str;
}