#include <globals.hpp>
#include <memory/memory.hpp> 
#include <Drivers/HAL/HAL.hpp>
#include <arch/x86_64/Serial.hpp>
#include <arch/x86_64/ScreenWriter.hpp>
#include <Drivers/firmware/ACPI/ACPI.hpp>
#include <arch/x86_64/Interrupts/GDT/GDT.hpp>
#include <arch/x86_64/Interrupts/IDT/IDT.hpp>
#include <arch/x86_64/Multitasking/Scheduler.hpp>

// Define the variables
HAL* g_hal = nullptr;
ScreenWriter* g_screenwriter = nullptr;
ACPIDriver* g_acpi = nullptr;
GDT* g_gdt = nullptr;
IDT* g_idt = nullptr;
EFI_SYSTEM_TABLE* g_uefi_system_table = nullptr;
Scheduler* g_scheduler = nullptr;

PhysicalMemoryManager g_PMM; 
HeapAllocator g_heap;