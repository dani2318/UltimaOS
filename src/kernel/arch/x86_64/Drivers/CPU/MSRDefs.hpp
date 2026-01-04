#pragma once

// Architectural MSRs
#define MSR_EFER            0xC0000080
#define MSR_STAR            0xC0000081
#define MSR_LSTAR           0xC0000082
#define MSR_CSTAR           0xC0000083
#define MSR_SFMASK          0xC0000084

// APIC / Timer
#define MSR_APIC_BASE       0x0000001B
#define MSR_TSC_DEADLINE    0x000006E0
