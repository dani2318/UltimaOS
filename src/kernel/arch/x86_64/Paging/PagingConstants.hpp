#pragma once
#include <stdint.h>

// Page size
#define PAGE_SIZE 4096

// Higher-half kernel base - IMPORTANT: Use identity mapping for now
// Setting this to 0 means PHYS_TO_VIRT will just return the physical address
#define KERNEL_BASE 0x0ULL

// Address conversion macros
#define PHYS_TO_VIRT(addr) ((void*)((uint64_t)(addr) + KERNEL_BASE))
#define VIRT_TO_PHYS(addr) ((uint64_t)(addr) - KERNEL_BASE)

// Check if address is in kernel space
#define IS_KERNEL_ADDR(addr) ((uint64_t)(addr) >= KERNEL_BASE)

// Align address down to page boundary
#define PAGE_ALIGN_DOWN(addr) ((uint64_t)(addr) & ~0xFFFULL)

// Align address up to page boundary
#define PAGE_ALIGN_UP(addr) (((uint64_t)(addr) + 0xFFF) & ~0xFFFULL)

// Get page number from address
#define ADDR_TO_PAGE(addr) ((uint64_t)(addr) / PAGE_SIZE)

// Get address from page number
#define PAGE_TO_ADDR(page) ((uint64_t)(page) * PAGE_SIZE)