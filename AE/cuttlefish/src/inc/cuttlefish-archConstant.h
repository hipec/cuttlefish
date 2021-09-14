/* Filename: RCR_daemon.c
 * 
 * Basic Description: RCRdaemon implemention for Haswell micro-architecture
 * 
 * Synopsis: This file contains functions to update counters in RCRBlackboard
 * * Following counters are included:
 * * 1. RAPL - Power and Temperature
 * * 2. Frequency - Time Stamp Counter (TSC), Average and Busy
 * * 3. Instruction - Instruction retired at User Level and OS Level
 * * 4. Memory - TOR_OCCUPANCY ALL, RR_OCCUPANCY IRQ, LLC_VICTIMS ALL
 *
 * * Contains a node meter that stores the RAPL power unit that allows portability across architecture for power/energy calculation
 *
 * * This implementation can also detect the topology of a node as block or cyclic, and automatically update the corresponding counter values to the correct socket 
 *
 * Commented counters if any are not used in the current implementation
 * *
 * * Please check your architecture specification for supported counters and other information
 *
 * Produced at Renaissance Computing Institute
 * 
 * original version Written by  Sridutt Bhalachandra, sriduttb@renci.org
 * *             Allan Porterfield,    akp@renci.org
 * */
/* Haswell Power MSR register addresses */

/*******************EVENT VALUES*******************************/ 

#define IA32_PERF_GLOBAL_CTRL_VALUE 0x10000000F // bit {0-3} tells us the number of PMC registers in use (i'th bit implies that PMC[i] is active) NOT USING in CUTTLEFISH
#define IA32_PERFEVTSEL0_VALUE 0x4120D1 //L3MISS NOT USING in CUTTLEFISH
#define IA32_PERFEVTSEL1_VALUE 0x4110D1 //L2MISS NOT USING in CUTTLEFISH
#define IA32_FIXED_CTR_CTRL_VALUE 0x2 // Control register for Fixed Counter 

/*******************REGISTER VALUES*******************************/ 

/* RAPL defines */
#define MSR_RAPL_POWER_UNIT              0x606
#define MSR_RAPL_POWER_UNIT_POWER_SHIFT  0
#define MSR_RAPL_POWER_UNIT_POWER_MASK   0xF
#define MSR_RAPL_POWER_UNIT_ENERGY_SHIFT 8
#define MSR_RAPL_POWER_UNIT_ENERGY_MASK  0x1F
#define MSR_RAPL_POWER_UNIT_TIME_SHIFT   15
#define MSR_RAPL_POWER_UNIT_TIME_MASK    0xF

/* Package */
#define MSR_PKG_RAPL_POWER_LIMIT        0x610
#define MSR_PKG_ENERGY_STATUS           0x611
#define MSR_PKG_PERF_STATUS             0x613
#define MSR_PKG_POWER_INFO              0x614

/* PP0 */
#define MSR_PP0_POWER_LIMIT             0x638
#define MSR_PP0_ENERGY_STATUS           0x639
#define MSR_PP0_POLICY                  0x63A
#define MSR_PP0_PERF_STATUS             0x63B

/* PP1 */
#define MSR_PP1_POWER_LIMIT             0x640
#define MSR_PP1_ENERGY_STATUS           0x641
#define MSR_PP1_POLICY                  0x642

/* DRAM */
#define MSR_DRAM_POWER_LIMIT            0x618
#define MSR_DRAM_ENERGY_STATUS          0x619
#define MSR_DRAM_PERF_STATUS            0x61B
#define MSR_DRAM_POWER_INFO             0x61C

/* Thermal MSRs */
#define IA32_THERM_STATUS_MSR           0x19C
#define IA32_TEMPERATURE_TARGET         0x01A2
#define IA32_PERF_STATUS                0x0198
#define MSR_PLATFORM_INFO               0xCE
#define IA32_PACKAGE_THERM_STATUS       0x1B1

/* Clock Speed */
#define IA32_PERF_FIXED_CTR1            0x30A
#define IA32_PERF_FIXED_CTR2            0x30B
#define IA32_CLOCK_MODULATION           0x19A

#define MSR_TSC                         0x10  // Time Stamp Counter MSR per core
#define IA32_MPERF                      0xE7 
#define IA32_APERF                      0xE8

/*These defines are from the Architectural MSRs (Should not change between models)*/

#define IA32_FIXED_CTR_CTRL             0x38D // Controls for fixed ctr0, 1, and 2 
#define IA32_PERF_GLOBAL_CTRL           0x38F // Enables for fixed ctr0,1,and2 here
#define IA32_PERF_GLOBAL_STATUS         0x38E // Overflow condition can be found here
#define IA32_PERF_GLOBAL_OVF_CTRL       0x390 // Can clear the overflow here
#define IA32_FIXED_CTR0                 0x309 // (R/W) Counts Instr_Retired.Any
#define IA32_FIXED_CTR1                 0x30A // (R/W) Counts CPU_CLK_Unhalted.Core
#define IA32_FIXED_CTR2                 0x30B // (R/W) Counts CPU_CLK_Unhalted.Ref

#define IA32_PMC0                       0xC1 //Performance Counter Register 0
#define IA32_PMC1			0xC2 //Performance Counter Register 1
#define IA32_PERFEVTSEL0                0x186 //Performance Event Selection Register 0
#define IA32_PERFEVTSEL1		0x187 //Performance Event Selection Register 1

/*UNCORE registers for read and write*/
#define MSR_UNCORE_FREQ 		0x620
#define MSR_UNCORE_READ 		0x621

/* C Box 0 MSRs */
/*Counter registers*/
#define C0_MSR_PMON_CTR3            0xE0B
#define C0_MSR_PMON_CTR2            0xE0A
#define C0_MSR_PMON_CTR1            0xE09
#define C0_MSR_PMON_CTR0            0xE08
/*Counter Filters*/
#define C0_MSR_PMON_BOX_FILTER1     0xE06
#define C0_MSR_PMON_BOX_FILTER0     0xE05
/*Counter Config Registers*/
#define C0_MSR_PMON_CTL3            0xE04
#define C0_MSR_PMON_CTL2            0xE03
#define C0_MSR_PMON_CTL1            0xE02
#define C0_MSR_PMON_CTL0            0xE01
/*Box Control*/
#define C0_MSR_PMON_BOX_STATUS      0xE07
#define C0_MSR_PMON_BOX_CTL         0xE00

/* C Box 1 MSRs */
#define C1_MSR_PMON_CTR3            0xE1B
#define C1_MSR_PMON_CTR2            0xE1A
#define C1_MSR_PMON_CTR1            0xE19
#define C1_MSR_PMON_CTR0            0xE18
#define C1_MSR_PMON_BOX_FILTER1     0xE16
#define C1_MSR_PMON_BOX_FILTER0     0xE15
#define C1_MSR_PMON_CTL3            0xE14
#define C1_MSR_PMON_CTL2            0xE13
#define C1_MSR_PMON_CTL1            0xE12
#define C1_MSR_PMON_CTL0            0xE11
#define C1_MSR_PMON_BOX_STATUS      0xE17
#define C1_MSR_PMON_BOX_CTL         0xE10

/* C Box 2 MSRs */
#define C2_MSR_PMON_CTR3            0xE2B
#define C2_MSR_PMON_CTR2            0xE2A
#define C2_MSR_PMON_CTR1            0xE29
#define C2_MSR_PMON_CTR0            0xE28
#define C2_MSR_PMON_BOX_FILTER1     0xE26
#define C2_MSR_PMON_BOX_FILTER0     0xE25
#define C2_MSR_PMON_CTL3            0xE24
#define C2_MSR_PMON_CTL2            0xE23
#define C2_MSR_PMON_CTL1            0xE22
#define C2_MSR_PMON_CTL0            0xE21
#define C2_MSR_PMON_BOX_STATUS      0xE27
#define C2_MSR_PMON_BOX_CTL         0xE20

/* C Box 3 MSRs */
#define C3_MSR_PMON_CTR3            0xE3B
#define C3_MSR_PMON_CTR2            0xE3A
#define C3_MSR_PMON_CTR1            0xE39
#define C3_MSR_PMON_CTR0            0xE38
#define C3_MSR_PMON_BOX_FILTER1     0xE36
#define C3_MSR_PMON_BOX_FILTER0     0xE35
#define C3_MSR_PMON_CTL3            0xE34
#define C3_MSR_PMON_CTL2            0xE33
#define C3_MSR_PMON_CTL1            0xE32
#define C3_MSR_PMON_CTL0            0xE31
#define C3_MSR_PMON_BOX_STATUS      0xE37
#define C3_MSR_PMON_BOX_CTL         0xE30

/* C Box 4 MSRs */
#define C4_MSR_PMON_CTR3            0xE4B
#define C4_MSR_PMON_CTR2            0xE4A
#define C4_MSR_PMON_CTR1            0xE49
#define C4_MSR_PMON_CTR0            0xE48
#define C4_MSR_PMON_BOX_FILTER1     0xE46
#define C4_MSR_PMON_BOX_FILTER0     0xE45
#define C4_MSR_PMON_CTL3            0xE44
#define C4_MSR_PMON_CTL2            0xE43
#define C4_MSR_PMON_CTL1            0xE42
#define C4_MSR_PMON_CTL0            0xE41
#define C4_MSR_PMON_BOX_STATUS      0xE47
#define C4_MSR_PMON_BOX_CTL         0xE40

/* C Box 5 MSRs */
#define C5_MSR_PMON_CTR3            0xE5B
#define C5_MSR_PMON_CTR2            0xE5A
#define C5_MSR_PMON_CTR1            0xE59
#define C5_MSR_PMON_CTR0            0xE58
#define C5_MSR_PMON_BOX_FILTER1     0xE56
#define C5_MSR_PMON_BOX_FILTER0     0xE55
#define C5_MSR_PMON_CTL3            0xE54
#define C5_MSR_PMON_CTL2            0xE53
#define C5_MSR_PMON_CTL1            0xE52
#define C5_MSR_PMON_CTL0            0xE51
#define C5_MSR_PMON_BOX_STATUS      0xE57
#define C5_MSR_PMON_BOX_CTL         0xE50

/* C Box 6 MSRs */
#define C6_MSR_PMON_CTR3            0xE6B
#define C6_MSR_PMON_CTR2            0xE6A
#define C6_MSR_PMON_CTR1            0xE69
#define C6_MSR_PMON_CTR0            0xE68
#define C6_MSR_PMON_BOX_FILTER1     0xE66
#define C6_MSR_PMON_BOX_FILTER0     0xE65
#define C6_MSR_PMON_CTL3            0xE64
#define C6_MSR_PMON_CTL2            0xE63
#define C6_MSR_PMON_CTL1            0xE62
#define C6_MSR_PMON_CTL0            0xE61
#define C6_MSR_PMON_BOX_STATUS      0xE67
#define C6_MSR_PMON_BOX_CTL         0xE60

/* C Box 7 MSRs */
#define C7_MSR_PMON_CTR3            0xE7B
#define C7_MSR_PMON_CTR2            0xE7A
#define C7_MSR_PMON_CTR1            0xE79
#define C7_MSR_PMON_CTR0            0xE78
#define C7_MSR_PMON_BOX_FILTER1     0xE76
#define C7_MSR_PMON_BOX_FILTER0     0xE75
#define C7_MSR_PMON_CTL3            0xE74
#define C7_MSR_PMON_CTL2            0xE73
#define C7_MSR_PMON_CTL1            0xE72
#define C7_MSR_PMON_CTL0            0xE71
#define C7_MSR_PMON_BOX_STATUS      0xE77
#define C7_MSR_PMON_BOX_CTL         0xE70

/* C Box 8 MSRs */
#define C8_MSR_PMON_CTR3            0xE8B
#define C8_MSR_PMON_CTR2            0xE8A
#define C8_MSR_PMON_CTR1            0xE89
#define C8_MSR_PMON_CTR0            0xE88
#define C8_MSR_PMON_BOX_FILTER1     0xE86
#define C8_MSR_PMON_BOX_FILTER0     0xE85
#define C8_MSR_PMON_CTL3            0xE84
#define C8_MSR_PMON_CTL2            0xE83
#define C8_MSR_PMON_CTL1            0xE82
#define C8_MSR_PMON_CTL0            0xE81
#define C8_MSR_PMON_BOX_STATUS      0xE87
#define C8_MSR_PMON_BOX_CTL         0xE80

/* C Box 9 MSRs */
#define C9_MSR_PMON_CTR3            0xE9B
#define C9_MSR_PMON_CTR2            0xE9A
#define C9_MSR_PMON_CTR1            0xE99
#define C9_MSR_PMON_CTR0            0xE98
#define C9_MSR_PMON_BOX_FILTER1     0xE96
#define C9_MSR_PMON_BOX_FILTER0     0xE95
#define C9_MSR_PMON_CTL3            0xE94
#define C9_MSR_PMON_CTL2            0xE93
#define C9_MSR_PMON_CTL1            0xE92
#define C9_MSR_PMON_CTL0            0xE91
#define C9_MSR_PMON_BOX_STATUS      0xE97
#define C9_MSR_PMON_BOX_CTL         0xE90

/* C Box 10 MSRs */
#define C10_MSR_PMON_CTR3           0xEAB
#define C10_MSR_PMON_CTR2           0xEAA
#define C10_MSR_PMON_CTR1           0xEA9
#define C10_MSR_PMON_CTR0           0xEA8
#define C10_MSR_PMON_BOX_FILTER1    0xEA6
#define C10_MSR_PMON_BOX_FILTER0    0xEA5
#define C10_MSR_PMON_CTL3           0xEA4
#define C10_MSR_PMON_CTL2           0xEA3
#define C10_MSR_PMON_CTL1           0xEA2
#define C10_MSR_PMON_CTL0           0xEA1
#define C10_MSR_PMON_BOX_STATUS     0xEA7
#define C10_MSR_PMON_BOX_CTL        0xEA0

/* C Box 11 MSRs */
#define C11_MSR_PMON_CTR3           0xEBB
#define C11_MSR_PMON_CTR2           0xEBA
#define C11_MSR_PMON_CTR1           0xEB9
#define C11_MSR_PMON_CTR0           0xEB8
#define C11_MSR_PMON_BOX_FILTER1    0xEB6
#define C11_MSR_PMON_BOX_FILTER0    0xEB5
#define C11_MSR_PMON_CTL3           0xEB4
#define C11_MSR_PMON_CTL2           0xEB3
#define C11_MSR_PMON_CTL1           0xEB2
#define C11_MSR_PMON_CTL0           0xEB1
#define C11_MSR_PMON_BOX_STATUS     0xEB7
#define C11_MSR_PMON_BOX_CTL        0xEB0

/* C Box 12 MSRs */
#define C12_MSR_PMON_CTR3           0xECB
#define C12_MSR_PMON_CTR2           0xECA
#define C12_MSR_PMON_CTR1           0xEC9
#define C12_MSR_PMON_CTR0           0xEC8
#define C12_MSR_PMON_BOX_FILTER1    0xEC6
#define C12_MSR_PMON_BOX_FILTER0    0xEC5
#define C12_MSR_PMON_CTL3           0xEC4
#define C12_MSR_PMON_CTL2           0xEC3
#define C12_MSR_PMON_CTL1           0xEC2
#define C12_MSR_PMON_CTL0           0xEC1
#define C12_MSR_PMON_BOX_STATUS     0xEC7
#define C12_MSR_PMON_BOX_CTL        0xEC0

/* C Box 13 MSRs */
#define C13_MSR_PMON_CTR3           0xEDB
#define C13_MSR_PMON_CTR2           0xEDA
#define C13_MSR_PMON_CTR1           0xED9
#define C13_MSR_PMON_CTR0           0xED8
#define C13_MSR_PMON_BOX_FILTER1    0xED6
#define C13_MSR_PMON_BOX_FILTER0    0xED5
#define C13_MSR_PMON_CTL3           0xED4
#define C13_MSR_PMON_CTL2           0xED3
#define C13_MSR_PMON_CTL1           0xED2
#define C13_MSR_PMON_CTL0           0xED1
#define C13_MSR_PMON_BOX_STATUS     0xED7
#define C13_MSR_PMON_BOX_CTL        0xED0

/* C Box 14 MSRs */
#define C14_MSR_PMON_CTR3           0xEEB
#define C14_MSR_PMON_CTR2           0xEEA
#define C14_MSR_PMON_CTR1           0xEE9
#define C14_MSR_PMON_CTR0           0xEE8
#define C14_MSR_PMON_BOX_FILTER1    0xEE6
#define C14_MSR_PMON_BOX_FILTER0    0xEE5
#define C14_MSR_PMON_CTL3           0xEE4
#define C14_MSR_PMON_CTL2           0xEE3
#define C14_MSR_PMON_CTL1           0xEE2
#define C14_MSR_PMON_CTL0           0xEE1
#define C14_MSR_PMON_BOX_STATUS     0xEE7
#define C14_MSR_PMON_BOX_CTL        0xEE0

/* C Box 15 MSRs */
#define C15_MSR_PMON_CTR3           0xEFB
#define C15_MSR_PMON_CTR2           0xEFA
#define C15_MSR_PMON_CTR1           0xEF9
#define C15_MSR_PMON_CTR0           0xEF8
#define C15_MSR_PMON_BOX_FILTER1    0xEF6
#define C15_MSR_PMON_BOX_FILTER0    0xEF5
#define C15_MSR_PMON_CTL3           0xEF4
#define C15_MSR_PMON_CTL2           0xEF3
#define C15_MSR_PMON_CTL1           0xEF2
#define C15_MSR_PMON_CTL0           0xEF1
#define C15_MSR_PMON_BOX_STATUS     0xEF7
#define C15_MSR_PMON_BOX_CTL        0xEF0

/* C Box 16 MSRs */
#define C16_MSR_PMON_CTR3           0xF0B
#define C16_MSR_PMON_CTR2           0xF0A
#define C16_MSR_PMON_CTR1           0xF09
#define C16_MSR_PMON_CTR0           0xF08
#define C16_MSR_PMON_BOX_FILTER1    0xF06
#define C16_MSR_PMON_BOX_FILTER0    0xF05
#define C16_MSR_PMON_CTL3           0xF04
#define C16_MSR_PMON_CTL2           0xF03
#define C16_MSR_PMON_CTL1           0xF02
#define C16_MSR_PMON_CTL0           0xF01
#define C16_MSR_PMON_BOX_STATUS     0xF07
#define C16_MSR_PMON_BOX_CTL        0xF00

/* C Box 17 MSRs */
#define C17_MSR_PMON_CTR3           0xF1B
#define C17_MSR_PMON_CTR2           0xF1A
#define C17_MSR_PMON_CTR1           0xF19
#define C17_MSR_PMON_CTR0           0xF18
#define C17_MSR_PMON_BOX_FILTER1    0xF16
#define C17_MSR_PMON_BOX_FILTER0    0xF15
#define C17_MSR_PMON_CTL3           0xF14
#define C17_MSR_PMON_CTL2           0xF13
#define C17_MSR_PMON_CTL1           0xF12
#define C17_MSR_PMON_CTL0           0xF11
#define C17_MSR_PMON_BOX_STATUS     0xF17
#define C17_MSR_PMON_BOX_CTL        0xF10

/* C Box 18 MSRs */
#define C18_MSR_PMON_CTR3           0xF2B
#define C18_MSR_PMON_CTR2           0xF2A
#define C18_MSR_PMON_CTR1           0xF29
#define C18_MSR_PMON_CTR0           0xF28
#define C18_MSR_PMON_BOX_FILTER1    0xF26
#define C18_MSR_PMON_BOX_FILTER0    0xF25
#define C18_MSR_PMON_CTL3           0xF24
#define C18_MSR_PMON_CTL2           0xF23
#define C18_MSR_PMON_CTL1           0xF22
#define C18_MSR_PMON_CTL0           0xF21
#define C18_MSR_PMON_BOX_STATUS     0xF27
#define C18_MSR_PMON_BOX_CTL        0xF20

/* C Box 19 MSRs */
#define C19_MSR_PMON_CTR3           0xF3B
#define C19_MSR_PMON_CTR2           0xF3A
#define C19_MSR_PMON_CTR1           0xF39
#define C19_MSR_PMON_CTR0           0xF38
#define C19_MSR_PMON_BOX_FILTER1    0xF36
#define C19_MSR_PMON_BOX_FILTER0    0xF35
#define C19_MSR_PMON_CTL3           0xF34
#define C19_MSR_PMON_CTL2           0xF33
#define C19_MSR_PMON_CTL1           0xF32
#define C19_MSR_PMON_CTL0           0xF31
#define C19_MSR_PMON_BOX_STATUS     0xF37
#define C19_MSR_PMON_BOX_CTL        0xF30

/* C Box 20 MSRs */
#define C20_MSR_PMON_CTR3           0xF4B
#define C20_MSR_PMON_CTR2           0xF4A
#define C20_MSR_PMON_CTR1           0xF49
#define C20_MSR_PMON_CTR0           0xF48
#define C20_MSR_PMON_BOX_FILTER1    0xF46
#define C20_MSR_PMON_BOX_FILTER0    0xF45
#define C20_MSR_PMON_CTL3           0xF44
#define C20_MSR_PMON_CTL2           0xF43
#define C20_MSR_PMON_CTL1           0xF42
#define C20_MSR_PMON_CTL0           0xF41
#define C20_MSR_PMON_BOX_STATUS     0xF47
#define C20_MSR_PMON_BOX_CTL        0xF40

/* C Box 21 MSRs */
#define C21_MSR_PMON_CTR3           0xF5B
#define C21_MSR_PMON_CTR2           0xF5A
#define C21_MSR_PMON_CTR1           0xF59
#define C21_MSR_PMON_CTR0           0xF58
#define C21_MSR_PMON_BOX_FILTER1    0xF56
#define C21_MSR_PMON_BOX_FILTER0    0xF55
#define C21_MSR_PMON_CTL3           0xF54
#define C21_MSR_PMON_CTL2           0xF53
#define C21_MSR_PMON_CTL1           0xF52
#define C21_MSR_PMON_CTL0           0xF51
#define C21_MSR_PMON_BOX_STATUS     0xF57
#define C21_MSR_PMON_BOX_CTL        0xF50

/* C Box 22 MSRs */
#define C22_MSR_PMON_CTR3           0xF6B
#define C22_MSR_PMON_CTR2           0xF6A
#define C22_MSR_PMON_CTR1           0xF69
#define C22_MSR_PMON_CTR0           0xF68
#define C22_MSR_PMON_BOX_FILTER1    0xF66
#define C22_MSR_PMON_BOX_FILTER0    0xF65
#define C22_MSR_PMON_CTL3           0xF64
#define C22_MSR_PMON_CTL2           0xF63
#define C22_MSR_PMON_CTL1           0xF62
#define C22_MSR_PMON_CTL0           0xF61
#define C22_MSR_PMON_BOX_STATUS     0xF67
#define C22_MSR_PMON_BOX_CTL        0xF60

/* C Box 23 MSRs */
#define C23_MSR_PMON_CTR3           0xF7B
#define C23_MSR_PMON_CTR2           0xF7A
#define C23_MSR_PMON_CTR1           0xF79
#define C23_MSR_PMON_CTR0           0xF78
#define C23_MSR_PMON_BOX_FILTER1    0xF76
#define C23_MSR_PMON_BOX_FILTER0    0xF75
#define C23_MSR_PMON_CTL3           0xF74
#define C23_MSR_PMON_CTL2           0xF73
#define C23_MSR_PMON_CTL1           0xF72
#define C23_MSR_PMON_CTL0           0xF71
#define C23_MSR_PMON_BOX_STATUS     0xF77
#define C23_MSR_PMON_BOX_CTL        0xF70

/* C Box 24 MSRs */
#define C24_MSR_PMON_CTR3           0xF8B
#define C24_MSR_PMON_CTR2           0xF8A
#define C24_MSR_PMON_CTR1           0xF89
#define C24_MSR_PMON_CTR0           0xF88
#define C24_MSR_PMON_BOX_FILTER1    0xF86
#define C24_MSR_PMON_BOX_FILTER0    0xF85
#define C24_MSR_PMON_CTL3           0xF84
#define C24_MSR_PMON_CTL2           0xF83
#define C24_MSR_PMON_CTL1           0xF82
#define C24_MSR_PMON_CTL0           0xF81
#define C24_MSR_PMON_BOX_STATUS     0xF87
#define C24_MSR_PMON_BOX_CTL        0xF80

/* C Box 25 MSRs */
#define C25_MSR_PMON_CTR3           0xF9B
#define C25_MSR_PMON_CTR2           0xF9A
#define C25_MSR_PMON_CTR1           0xF99
#define C25_MSR_PMON_CTR0           0xF98
#define C25_MSR_PMON_BOX_FILTER1    0xF96
#define C25_MSR_PMON_BOX_FILTER0    0xF95
#define C25_MSR_PMON_CTL3           0xF94
#define C25_MSR_PMON_CTL2           0xF93
#define C25_MSR_PMON_CTL1           0xF92
#define C25_MSR_PMON_CTL0           0xF91
#define C25_MSR_PMON_BOX_STATUS     0xF97
#define C25_MSR_PMON_BOX_CTL        0xF90

/* C Box 26 MSRs */
#define C26_MSR_PMON_CTR3           0xFAB
#define C26_MSR_PMON_CTR2           0xFAA
#define C26_MSR_PMON_CTR1           0xFA9
#define C26_MSR_PMON_CTR0           0xFA8
#define C26_MSR_PMON_BOX_FILTER1    0xFA6
#define C26_MSR_PMON_BOX_FILTER0    0xFA5
#define C26_MSR_PMON_CTL3           0xFA4
#define C26_MSR_PMON_CTL2           0xFA3
#define C26_MSR_PMON_CTL1           0xFA2
#define C26_MSR_PMON_CTL0           0xFA1
#define C26_MSR_PMON_BOX_STATUS     0xFA7
#define C26_MSR_PMON_BOX_CTL        0xFA0

/* C Box 27 MSRs */
#define C27_MSR_PMON_CTR3           0xFBB
#define C27_MSR_PMON_CTR2           0xFBA
#define C27_MSR_PMON_CTR1           0xFB9
#define C27_MSR_PMON_CTR0           0xFB8
#define C27_MSR_PMON_BOX_FILTER1    0xFB6
#define C27_MSR_PMON_BOX_FILTER0    0xFB5
#define C27_MSR_PMON_CTL3           0xFB4
#define C27_MSR_PMON_CTL2           0xFB3
#define C27_MSR_PMON_CTL1           0xFB2
#define C27_MSR_PMON_CTL0           0xFB1
#define C27_MSR_PMON_BOX_STATUS     0xFB7
#define C27_MSR_PMON_BOX_CTL        0xFB0

#include "haswell-arch.h"



