#include <arch/x86_64/Drivers/HAL/HAL.hpp>
#include <arch/x86_64/Drivers/ACPI/ACPI.hpp>
#include <arch/x86_64/Paging.hpp>
#include <arch/x86_64/Drivers/CPU/MSR.hpp>
#include <arch/x86_64/Drivers/CPU/MSRDefs.hpp>

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
    asm volatile("mov %0, %%cr3" ::"r"(pml4));
}

void HAL::InitializePlatform(BootInfo *bootInfo,
                             PageTableManager &ptManager)
{
    // ACPI RSDT/XSDT
    uint64_t rsdtPhys = (uint64_t)bootInfo->rsdt_address;

    // Identity map first two pages (RSDT/XSDT may span pages)
    ptManager.MapMemory((void *)rsdtPhys, (void *)rsdtPhys);
    ptManager.MapMemory((void *)(rsdtPhys + 4096),
                        (void *)(rsdtPhys + 4096));

    // Initialize ACPI
    g_acpi = new ACPIDriver((void *)rsdtPhys, ptManager);

    // Disable legacy PIC
    g_acpi->DisablePIC();

    // Enable LAPIC
    uint64_t lapic = g_acpi->GetLAPICAddress();
    ptManager.MapMemory((void *)lapic, (void *)lapic);

    // Spurious Interrupt Vector Register
    *(volatile uint32_t *)(lapic + 0xF0) = 0x1FF;

    g_acpi->EnableLocalAPIC();
}
