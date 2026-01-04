#pragma once
#include <stdint.h>

struct ACPIHeader
{
    char Signature[4];
    uint32_t Length;
    uint8_t Revision;
    uint8_t Checksum;
    char OEMID[6];
    char OEMTableID[8];
    uint32_t OEMRevision;
    uint32_t CreatorID;
    uint32_t CreatorRevision;

    bool Verify()
    {
        uint8_t sum = 0;
        for (uint32_t i = 0; i < Length; i++)
        {
            sum += ((uint8_t *)this)[i];
        }
        return sum == 0;
    }
} __attribute__((packed));

struct GenericAddressStructure
{
    uint8_t AddressSpace; // 0 = Memory, 1 = I/O, 2 = PCI Config
    uint8_t BitWidth;
    uint8_t BitOffset;
    uint8_t AccessSize;
    uint64_t Address;
} __attribute__((packed));

struct FADT : public ACPIHeader
{
    uint32_t FirmwareCtrl;
    uint32_t Dsdt;    // 32-bit pointer to DSDT
    uint8_t Reserved; // Used in ACPI 1.0; no longer used
    uint8_t PreferredPowerManagementProfile;
    uint16_t SciInterrupt;
    uint32_t SMI_CommandPort;
    uint8_t AcpiEnable;
    uint8_t AcpiDisable;
    uint8_t S4BIOS_REQ;
    uint8_t PSTATE_Control;
    uint32_t PM1a_Event_Block;
    uint32_t PM1b_Event_Block;
    uint32_t PM1aControlBlock;
    uint32_t PM1bControlBlock;
    uint32_t PM2_Control_Block;
    uint32_t PM_Timer_Block;
    uint32_t GPE0_Block;
    uint32_t GPE1_Block;
    uint8_t PM1_Event_Length;
    uint8_t PM1_Control_Length;
    uint8_t PM2_Control_Length;
    uint8_t PM_Timer_Length;
    uint8_t GPE0_Length;
    uint8_t GPE1_Length;
    uint8_t GPE1_Base;
    uint8_t CState_Control;
    uint16_t WorstC2Latency;
    uint16_t WorstC3Latency;
    uint16_t FlushSize;
    uint16_t FlushStride;
    uint8_t DutyOffset;
    uint8_t DutyWidth;
    uint8_t DayAlarm;
    uint8_t MonthAlarm;
    uint8_t Century;
    uint16_t BootArchitectureFlags; // IA-PC Boot Architecture Flags
    uint8_t Reserved2;
    uint32_t Flags; // Fixed feature flags (e.g. HW_REDUCED_ACPI)

    // ACPI 2.0+ Reset Register
    GenericAddressStructure ResetReg;
    uint8_t ResetValue;
    uint8_t Reserved3[3];

    // 64-bit Extended Fields (Standard for UEFI)
    uint64_t X_FirmwareControl;
    uint64_t X_Dsdt; // 64-bit pointer to DSDT

    GenericAddressStructure X_PM1a_Event_Block;
    GenericAddressStructure X_PM1b_Event_Block;
    GenericAddressStructure X_PM1a_Control_Block;
    GenericAddressStructure X_PM1b_Control_Block;
    GenericAddressStructure X_PM2_Control_Block;
    GenericAddressStructure X_PM_Timer_Block;
    GenericAddressStructure X_GPE0_Block;
    GenericAddressStructure X_GPE1_Block;

    // ACPI 5.0+ Sleep registers
    GenericAddressStructure SleepControlReg;
    GenericAddressStructure SleepStatusReg;

    uint64_t HypervisorVendorID;
} __attribute__((packed));
