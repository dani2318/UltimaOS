#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <memory/memory.h>  // for memset
#include "globals.h"

/* =========================
   BITMAP
   ========================= */

typedef struct {
    size_t   Size;     // Size in bytes
    uint8_t* Buffer;
} Bitmap;

static inline bool Bitmap_Get(Bitmap* bmp, uint64_t index) {
    return (bmp->Buffer[index / 8] & (1 << (index % 8))) != 0;
}

static inline void Bitmap_Set(Bitmap* bmp, uint64_t index, bool value) {
    if (value)
        bmp->Buffer[index / 8] |=  (1 << (index % 8));
    else
        bmp->Buffer[index / 8] &= ~(1 << (index % 8));
}

/* =========================
   PHYSICAL MEMORY MANAGER
   ========================= */

typedef struct {
    Bitmap   PageBitmap;
    uint64_t TotalMemory;
    uint64_t FreeMemory;
    uint64_t UsedMemory;
} PhysicalMemoryManager;

#define PAGE_SIZE 4096

static inline void PMM_FreePage(PhysicalMemoryManager* pmm, void* address) {
    uint64_t index = (uint64_t)address / PAGE_SIZE;
    if (!Bitmap_Get(&pmm->PageBitmap, index)) return;

    Bitmap_Set(&pmm->PageBitmap, index, false);
    pmm->FreeMemory += PAGE_SIZE;
    pmm->UsedMemory -= PAGE_SIZE;
}

static inline void PMM_LockPage(PhysicalMemoryManager* pmm, void* address) {
    uint64_t index = (uint64_t)address / PAGE_SIZE;
    if (Bitmap_Get(&pmm->PageBitmap, index)) return;

    Bitmap_Set(&pmm->PageBitmap, index, true);
    pmm->FreeMemory -= PAGE_SIZE;
    pmm->UsedMemory += PAGE_SIZE;
}

static inline void PMM_FreePages(PhysicalMemoryManager* pmm, void* address, uint64_t count) {
    for (uint64_t i = 0; i < count; i++) {
        PMM_FreePage(pmm, (void*)((uint64_t)address + i * PAGE_SIZE));
    }
}

static inline void PMM_LockPages(PhysicalMemoryManager* pmm, void* address, uint64_t count) {
    for (uint64_t i = 0; i < count; i++) {
        PMM_LockPage(pmm, (void*)((uint64_t)address + i * PAGE_SIZE));
    }
}

static inline void* PMM_RequestPage(PhysicalMemoryManager* pmm) {
    for (uint64_t i = 0; i < pmm->PageBitmap.Size * 8; i++) {
        if (!Bitmap_Get(&pmm->PageBitmap, i)) {
            void* addr = (void*)(i * PAGE_SIZE);
            PMM_LockPage(pmm, addr);
            return addr;
        }
    }
    return NULL;
}

/* =========================
   PAGING STRUCTURES
   ========================= */

typedef struct {
    uint64_t value;
} PageTableEntry;

static inline void PTE_SetFlag(PageTableEntry* e, uint64_t flag, bool enable) {
    if (enable)
        e->value |= flag;
    else
        e->value &= ~flag;
}

static inline bool PTE_GetFlag(PageTableEntry* e, uint64_t flag) {
    return (e->value & flag) != 0;
}

static inline void PTE_SetAddress(PageTableEntry* e, uint64_t address) {
    e->value &= 0xFFFULL;
    e->value |= address & 0x000FFFFFFFFFF000ULL;
}

static inline uint64_t PTE_GetAddress(PageTableEntry* e) {
    return e->value & 0x000FFFFFFFFFF000ULL;
}

typedef struct {
    PageTableEntry entries[512];
} PageTable;

/* =========================
   PAGE TABLE MANAGER
   ========================= */
extern PhysicalMemoryManager g_PMM;
typedef struct {
    PageTable* PML4;
} PageTableManager;

static inline void PTM_Init(PageTableManager* ptm, PageTable* pml4) {
    ptm->PML4 = pml4;
}

static inline void PTM_MapMemory(PageTableManager* ptm, void* virtualAddr, void* physicalAddr) {
    uint64_t vAddr = (uint64_t)virtualAddr;

    uint64_t i4 = (vAddr >> 39) & 0x1FF;
    uint64_t i3 = (vAddr >> 30) & 0x1FF;
    uint64_t i2 = (vAddr >> 21) & 0x1FF;
    uint64_t i1 = (vAddr >> 12) & 0x1FF;

    PageTableEntry* entry;
    PageTable* PDP;
    PageTable* PD;
    PageTable* PT;

    /* PML4 -> PDP */
    entry = &ptm->PML4->entries[i4];
    if (!PTE_GetFlag(entry, 1ULL << 0)) {
        PDP = (PageTable*)PMM_RequestPage(&g_PMM);
        memset(PDP, 0, PAGE_SIZE);
        PTE_SetAddress(entry, (uint64_t)PDP);
        PTE_SetFlag(entry, 1ULL << 0, true); // Always Present
        PTE_SetFlag(entry, 1ULL << 1, true); // Always R/W for table structures
    } else {
        PDP = (PageTable*)PTE_GetAddress(entry);
    }

    /* PDP -> PD */
    entry = &PDP->entries[i3];
    if (!PTE_GetFlag(entry, 1ULL << 0)) {
        PD = (PageTable*)PMM_RequestPage(&g_PMM);
        memset(PD, 0, PAGE_SIZE);
        PTE_SetAddress(entry, (uint64_t)PD);
        PTE_SetFlag(entry, 1ULL << 0, true);
        PTE_SetFlag(entry, 1ULL << 1, true);
    } else {
        PD = (PageTable*)PTE_GetAddress(entry);
    }

    /* PD -> PT */
    entry = &PD->entries[i2];
    if (!PTE_GetFlag(entry, 1ULL << 0)) {
        PT = (PageTable*)PMM_RequestPage(&g_PMM);
        memset(PT, 0, PAGE_SIZE);
        PTE_SetAddress(entry, (uint64_t)PT);
        PTE_SetFlag(entry, 1ULL << 0, true);
        PTE_SetFlag(entry, 1ULL << 1, true);
    } else {
        PT = (PageTable*)PTE_GetAddress(entry);
    }

    /* PT -> page */
    entry = &PT->entries[i1];
    PTE_SetAddress(entry, (uint64_t)physicalAddr);
    PTE_SetFlag(entry, 1ULL << 0, true);
    PTE_SetFlag(entry, 1ULL << 1, true);
}