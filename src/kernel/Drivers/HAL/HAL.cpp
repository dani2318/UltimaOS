#include "Drivers/HAL/HAL.hpp"
#include "Drivers/firmware/ACPI/ACPI.hpp"
#include "arch/x86_64/Paging/Paging.hpp"
#include <arch/x86_64/CPU/MSR.hpp>
#include <arch/x86_64/CPU/MSRDefs.hpp>
#include <cpp/String.hpp>
#include <globals.hpp>

extern ACPIDriver *g_acpi;

void HAL::DisableInterrupts()
{
    asm volatile("cli");
}

void HAL::EnableInterrupts()
{
    asm volatile("sti");
}

void HAL::Halt()
{
    asm volatile("hlt");
}


uint64_t HAL::ReadTSC()
{
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

void HAL::InitializeCPU()
{
    uint64_t cr0, cr4;

    // Enable SSE
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 2); // EM
    cr0 |=  (1 << 1); // MP
    asm volatile("mov %0, %%cr0" :: "r"(cr0));

    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 9);   // OSFXSR
    cr4 |= (1 << 10);  // OSXMMEXCPT
    asm volatile("mov %0, %%cr4" :: "r"(cr4));

    // Enable NX bit
    uint64_t efer = MSR::Read(MSR_EFER);
    efer |= (1 << 11); // NXE
    MSR::Write(MSR_EFER, efer);
}

void HAL::SwitchPageTable(void *pml4)
{
    uint64_t addr = (uint64_t)pml4;
    if (!pml4) gScreenwriter->print("PML4 is null", true);
    if (addr & 0xFFF) gScreenwriter->print("PML4 not aligned", true);

    if (addr > 0x40000000) {
        // Might be virtual - this is suspicious
        gScreenwriter->print("Warning: PML4 looks like virtual address\n",true);
    }

    uint64_t current_cr3;
    asm volatile("mov %%cr3, %0" : "=r"(current_cr3));
    gScreenwriter->print("Switching CR3: 0x", true);

    char str[32];   // Allocate buffers
    char str2[32];

    itoa(current_cr3, str, 16);
    itoa(addr, str2, 16);

    gScreenwriter->print(str, true);
    gScreenwriter->print(" -> ",true);
    gScreenwriter->print("0x", true);
    gScreenwriter->print(str2, true);
    gScreenwriter->print("\n", true);

    asm volatile("cli");
    asm volatile("mov %0, %%cr3" ::"r"(addr));
    asm volatile("sti");

}

void HAL::InitializePlatform(BootInfo* bootInfo, PageTableManager& ptManager)
{
    // ACPI RSDT/XSDT physical address
    uint64_t rsdtPhys = (uint64_t)bootInfo->rsdt_address;

    // Align to page boundaries
    uint64_t page0 = rsdtPhys & ~0xFFFULL;
    uint64_t page1 = page0 + 0x1000; // next 4KB page

    // Identity map the first two pages (RSDT/XSDT may span pages)
    ptManager.mapPage(page0, page0, false, false, true); // writable=false, kernel-only, executable=true
    ptManager.mapPage(page1, page1, false, false, true);

    // Initialize ACPI driver
    g_acpi = new ACPIDriver((void*)rsdtPhys, ptManager);

    // Disable legacy PIC
    g_acpi->DisablePIC();

    // Enable Local APIC
    uint64_t lapicPhys = g_acpi->GetLAPICAddress();
    ptManager.mapPage(lapicPhys & ~0xFFFULL, lapicPhys & ~0xFFFULL, true, false, true);

    // Spurious Interrupt Vector Register
    *(volatile uint32_t*)(PHYS_TO_VIRT(lapicPhys) + 0xF0) = 0x1FF;

    g_acpi->EnableLocalAPIC();
}
