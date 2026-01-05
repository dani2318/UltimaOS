#include <globals.hpp>
// MUST include these to fix "incomplete type"
#include <memory/memory.hpp> 
#include <arch/x86_64/Drivers/HAL/HAL.hpp>
#include <arch/x86_64/Serial.hpp>
#include <arch/x86_64/ScreenWriter.hpp>
#include <arch/x86_64/Drivers/ACPI/ACPI.hpp>
#include <arch/x86_64/Interrupts/GDT/GDT.hpp>
#include <arch/x86_64/Interrupts/IDT/IDT.hpp>


// Define the variables
HAL* g_hal = nullptr;
Serial* g_serialWriter = nullptr;
ScreenWriter* g_screenwriter = nullptr;
ACPIDriver* g_acpi = nullptr;
GDT* g_gdt = nullptr;
IDT* g_idt = nullptr;
EFI_SYSTEM_TABLE* g_uefi_system_table = nullptr;

// These are full objects, not pointers. 
// Their headers must be included above!
PhysicalMemoryManager g_PMM; 
HeapAllocator g_heap;