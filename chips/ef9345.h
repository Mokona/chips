#pragma once
/*#
    # ef9345.h

    Header-only emulator for the Thomson EF9345 display processor.

    ## Emulated Pins
    **********************************
    *           +----------+         *
    *    OE/ <--|          |         *
    *    WE/ <--|          |<-> ADM0 *
    *   ASM/ <--|          |...      *
    *     AS -->|          |<-> ADM7 *
    *     DS -->|  EF9345  |         *
    *    CS/ -->|          |         *
    *    R/W -->|          |--> AM8  *
    *  PC/VS <--|          |...      *
    * HVS/HS <--|          |--> AM13 *
    *      B <--|          |         *
    *      G <--|          |<-> AD0  *
    *      R <--|          |...      *
    *           |          |<-> AD7  *
    *           |          |         *
    *           +----------+         *
    **********************************

    Not emulated:
    - SYNC IN: Synchro in. Always low on the VG5000µ.
    - I: external video signal.
    - HP: video clock, 4Mhz phased with RGBI signals. It normally drives the Z80.
    - CS/: is always low on the VG5000µ.
    - R,G,B: do we really need to emulate this? Probably too fast

    Copyright (c) 2023 Sylvain Glaize
    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the
    use of this software.
    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:
        1. The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software in a
        product, an acknowledgment in the product documentation would be
        appreciated but is not required.
        2. Altered source versions must be plainly marked as such, and must not
        be misrepresented as being the original software.
        3. This notice may not be removed or altered from any source
        distribution. 
#*/
#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>

#ifdef __cplusplus
extern "C" {
#endif

// Microprocessor interface
#define EF9345_PIN_AD0      (0)    /* multiplexed address/data bus */
#define EF9345_PIN_AD1      (1)    /* multiplexed address/data bus */
#define EF9345_PIN_AD2      (2)    /* multiplexed address/data bus */
#define EF9345_PIN_AD3      (3)    /* multiplexed address/data bus */
#define EF9345_PIN_AD4      (4)    /* multiplexed address/data bus */
#define EF9345_PIN_AD5      (5)    /* multiplexed address/data bus */
#define EF9345_PIN_AD6      (6)    /* multiplexed address/data bus */
#define EF9345_PIN_AD7      (7)    /* multiplexed address/data bus */

#define EF9345_PIN_AS       (8)    /* address strobe, falling edge */
#define EF9345_PIN_DS       (9)    /* data strobe, latched by AS */
#define EF9345_PIN_RW       (10)    /* read/write, read high */
#define EF9345_PIN_CS       (11)    /* chip select, latched by AS, active low */

// Memory interface
#define EF9345_PIN_AM13     (29)    /* memory address bus */
#define EF9345_PIN_AM12     (28)    /* memory address bus */
#define EF9345_PIN_AM11     (27)    /* memory address bus */
#define EF9345_PIN_AM10     (26)    /* memory address bus */
#define EF9345_PIN_AM9      (25)    /* memory address bus */
#define EF9345_PIN_AM8      (24)    /* memory address bus */
#define EF9345_PIN_ADM7     (23)    /* multiplexed address/data mode */
#define EF9345_PIN_ADM6     (22)    /* multiplexed address/data mode */
#define EF9345_PIN_ADM5     (21)    /* multiplexed address/data mode */
#define EF9345_PIN_ADM4     (20)    /* multiplexed address/data mode */
#define EF9345_PIN_ADM3     (19)    /* multiplexed address/data mode */
#define EF9345_PIN_ADM2     (18)    /* multiplexed address/data mode */
#define EF9345_PIN_ADM1     (17)    /* multiplexed address/data mode */
#define EF9345_PIN_ADM0     (16)    /* multiplexed address/data mode */

#define EF9345_PIN_OE       (12)     /* output enable, active low */
#define EF9345_PIN_WE       (13)     /* write enable, active low */
#define EF9345_PIN_ASM      (14)     /* memory address strobe mask, falling edge */

// Video interface
#define EF9345_PIN_HVS_HS   (15)     /* horizontal/vertical composite sync / horizontal sync, active high */
#define EF9345_PIN_PC_VS    (30)     /* phase comparator / vertical sync, active high */
#define EF9345_PIN_B        (31)     /* blue */
#define EF9345_PIN_G        (32)     /* green */
#define EF9345_PIN_R        (33)     /* red */

// pin bit masks
#define EF9345_MASK_OE      (1ULL<<EF9345_PIN_OE)
#define EF9345_MASK_WE      (1ULL<<EF9345_PIN_WE)
#define EF9345_MASK_ASM     (1ULL<<EF9345_PIN_ASM)
#define EF9345_MASK_HVS_HS  (1ULL<<EF9345_PIN_HVS_HS)
#define EF9345_MASK_PC_VS   (1ULL<<EF9345_PIN_PC_VS)
#define EF9345_MASK_B       (1ULL<<EF9345_PIN_B)
#define EF9345_MASK_G       (1ULL<<EF9345_PIN_G)
#define EF9345_MASK_R       (1ULL<<EF9345_PIN_R)
#define EF9345_MASK_AS      (1ULL<<EF9345_PIN_AS)
#define EF9345_MASK_DS      (1ULL<<EF9345_PIN_DS)
#define EF9345_MASK_RW      (1ULL<<EF9345_PIN_RW)
#define EF9345_MASK_AD0     (1ULL<<EF9345_PIN_AD0)
#define EF9345_MASK_AD1     (1ULL<<EF9345_PIN_AD1)
#define EF9345_MASK_AD2     (1ULL<<EF9345_PIN_AD2)
#define EF9345_MASK_AD3     (1ULL<<EF9345_PIN_AD3)
#define EF9345_MASK_AD4     (1ULL<<EF9345_PIN_AD4)
#define EF9345_MASK_AD5     (1ULL<<EF9345_PIN_AD5)
#define EF9345_MASK_AD6     (1ULL<<EF9345_PIN_AD6)
#define EF9345_MASK_AD7     (1ULL<<EF9345_PIN_AD7)
#define EF9345_MASK_CS      (1ULL<<EF9345_PIN_CS)
#define EF9345_MASK_AM13    (1ULL<<EF9345_PIN_AM13)
#define EF9345_MASK_AM12    (1ULL<<EF9345_PIN_AM12)
#define EF9345_MASK_AM11    (1ULL<<EF9345_PIN_AM11)
#define EF9345_MASK_AM10    (1ULL<<EF9345_PIN_AM10)
#define EF9345_MASK_AM9     (1ULL<<EF9345_PIN_AM9)
#define EF9345_MASK_AM8     (1ULL<<EF9345_PIN_AM8)
#define EF9345_MASK_ADM7    (1ULL<<EF9345_PIN_ADM7)
#define EF9345_MASK_ADM6    (1ULL<<EF9345_PIN_ADM6)
#define EF9345_MASK_ADM5    (1ULL<<EF9345_PIN_ADM5)
#define EF9345_MASK_ADM4    (1ULL<<EF9345_PIN_ADM4)
#define EF9345_MASK_ADM3    (1ULL<<EF9345_PIN_ADM3)
#define EF9345_MASK_ADM2    (1ULL<<EF9345_PIN_ADM2)
#define EF9345_MASK_ADM1    (1ULL<<EF9345_PIN_ADM1)
#define EF9345_MASK_ADM0    (1ULL<<EF9345_PIN_ADM0)

/* register names */
#define EF9345_REG_DIRECT_R0    (0)
#define EF9345_REG_DIRECT_R1    (1)
#define EF9345_REG_DIRECT_R2    (2)
#define EF9345_REG_DIRECT_R3    (3)
#define EF9345_REG_DIRECT_R4    (4)
#define EF9345_REG_DIRECT_R5    (5)
#define EF9345_REG_DIRECT_R6    (6)
#define EF9345_REG_DIRECT_R7    (7)

#define EF9345_REG_INDIRECT_ROM (0)
#define EF9345_REG_INDIRECT_TGS (1)
#define EF9345_REG_INDIRECT_MAT (2)
#define EF9345_REG_INDIRECT_PAT (3)
#define EF9345_REG_INDIRECT_DOR (4)
#define EF9345_REG_INDIRECT_ROR (7)

#define EF9345_FRAMEBUFFER_WIDTH (320)
#define EF9345_FRAMEBUFFER_HEIGHT (250)
#define EF9345_FRAMEBUFFER_SIZE (EF9345_FRAMEBUFFER_WIDTH*EF9345_FRAMEBUFFER_HEIGHT)

#define EF9345_FREQUENCY (12000000)    /* 12 MHz */

typedef struct {
    union {
        uint8_t direct_regs[8];
        struct {
            uint8_t direct_r0;
            uint8_t direct_r1;
            uint8_t direct_r2;
            uint8_t direct_r3;
            uint8_t direct_r4;
            uint8_t direct_r5;
            uint8_t direct_r6;
            uint8_t direct_r7;
        };
    };
    union {
        uint8_t indirect_regs[8];
        struct {
            uint8_t indirect_rom;
            uint8_t indirect_tgs;
            uint8_t indirect_mat;
            uint8_t indirect_pat;
            uint8_t indirect_dor;
            uint8_t indirect_xxx_5;    /* unused */
            uint8_t indirect_xxx_6;    /* unused */
            uint8_t indirect_ror;
        };
    };

    uint64_t pins; /* pin state after last tick */

    /* Latched data by the latest AS falling edge */
    uint8_t l_address; // called A in the EF9345 datasheet, with a supplementary 8th bit being CS/ (which is always 0 for the VG5000µ)
    uint8_t l_ds;
    
    bool execution_flag; // true when execution was requested by XQR

    mem_t mem;  // Access to Video RAM
    uint8_t ram[0x2000]; // Video RAM

    uint16_t line_tick;
    uint16_t current_line;

    uint16_t fb_width;
    uint16_t fb_height;
    uint32_t fb_size;
    alignas(64) uint8_t fb[EF9345_FRAMEBUFFER_SIZE]; // TODO: extract from here and implement a "TV" surface on the vg5000. Keep pointer only here.

} ef9345_t;

/* helper macros to extract address and data values from pin mask */
#define EF9345_AD0_AD7_MASK (EF9345_MASK_AD0 | EF9345_MASK_AD1 | EF9345_MASK_AD2 | EF9345_MASK_AD3 | EF9345_MASK_AD4 | EF9345_MASK_AD5 | EF9345_MASK_AD6 | EF9345_MASK_AD7)
#define EF9345_ADM0_ADM7_MASK (EF9345_MASK_ADM0 | EF9345_MASK_ADM1 | EF9345_MASK_ADM2 | EF9345_MASK_ADM3 | EF9345_MASK_ADM4 | EF9345_MASK_ADM5 | EF9345_MASK_ADM6 | EF9345_MASK_ADM7)
#define EF9345_AM8_AM13_MASK (EF9345_MASK_AM8 | EF9345_MASK_AM9 | EF9345_MASK_AM10 | EF9345_MASK_AM11 | EF9345_MASK_AM12 | EF9345_MASK_AM13)
#define EF9345_ADM0_AM13_MASK (EF9345_ADM0_ADM7_MASK | EF9345_AM8_AM13_MASK)

/* processor interface */
/* extract multiplexed data/bus from AD0-AD7 pins */
#define EF9345_GET_MUX_DATA_ADDR(p) ((uint8_t)(((p)&EF9345_AD0_AD7_MASK)>>EF9345_PIN_AD0))
/* set multiplexed data/bus to AD0-AD7 pins */
#define EF9345_SET_MUX_DATA_ADDR(p, d) {((p) = ((p)&~EF9345_AD0_AD7_MASK)|(((d)&0xff)<<EF9345_PIN_AD0));}

/* internal memory interface */
/* extract multiplexed address from ADM0-ADM7 and AM8-AM13 pins */
#define EF9345_GET_MUX_ADDR(p) ((uint16_t)(((p)&EF9345_ADM0_AM13_MASK)>>EF9345_PIN_ADM0))
/* set multiplexed address to ADM0-ADM7 and AM8-AM13 pins */
#define EF9345_SET_MUX_ADDR(p, a) {((p) = ((p)&~EF9345_ADM0_AM13_MASK)|(((a)&0x3fff)<<EF9345_PIN_ADM0));}
/* extract multiplexed data from ADM0-ADM7 pins */
#define EF9345_GET_MUX_DATA(p) ((uint8_t)(((p)&EF9345_ADM0_ADM7_MASK)>>EF9345_PIN_ADM0))
/* set multiplexed data to ADM0-ADM7 pins */
#define EF9345_SET_MUX_DATA(p, d) {((p) = ((p)&~EF9345_ADM0_ADM7_MASK)|(((d)&0xff)<<EF9345_PIN_ADM0));}

/* Memory pointer macros */
//

/* initialize a new ef9345 instance */
void ef9345_init(ef9345_t *ef9345);
/* reset an existing ef9345 instance */
void ef9345_reset(ef9345_t *ef9345);
/* tick the ef9345 instance, returns the pins of the simulated ef9345 */
uint64_t ef9345_tick(ef9345_t *ef9345, uint64_t vdp_pins);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef CHIPS_IMPL
#include <string.h>
#ifndef CHIPS_ASSERT
    #include <assert.h>
    #define CHIPS_ASSERT(c) assert(c)
#endif

static void _ef9345_init_memory_map(ef9345_t* ef9345);
static uint64_t _ef9345_external_bus_transfer(ef9345_t* ef9345, uint64_t vdp_pins);
static uint64_t _ef9345_beam_update(ef9345_t* ef9345, uint64_t vdp_pins);

void ef9345_init(ef9345_t* ef9345) {
    CHIPS_ASSERT(ef9345);
    memset(ef9345, 0, sizeof(*ef9345));

    ef9345->fb_width = EF9345_FRAMEBUFFER_WIDTH;
    ef9345->fb_height = EF9345_FRAMEBUFFER_HEIGHT;
    ef9345->fb_size = EF9345_FRAMEBUFFER_SIZE;

    _ef9345_init_memory_map(ef9345);
}

void ef9345_reset(ef9345_t* ef9345) {
    CHIPS_ASSERT(ef9345);
    ef9345_init(ef9345);
}

// The horizontal line takes 64µs to display
static const uint16_t tick_per_line = EF9345_FREQUENCY / 1000000 * 64;

// The H Blank is set during the 10µs (for 40 char/row)
// TODO: it is slighlty different with 80 char/row)
static const uint16_t tick_hblank_start = EF9345_FREQUENCY / 1000000 * 10;

// Number of ticks for 1µs
static const uint16_t tick_for_1mus = EF9345_FREQUENCY / 1000000;

uint64_t ef9345_tick(ef9345_t* ef9345, uint64_t vdp_pins) {
    CHIPS_ASSERT(ef9345);

    // TODO: here, update the R0 register

    vdp_pins = _ef9345_external_bus_transfer(ef9345, vdp_pins);
    vdp_pins = _ef9345_beam_update(ef9345, vdp_pins);

    ef9345->pins = vdp_pins;

    return vdp_pins;
}

static void _ef9345_init_memory_map(ef9345_t* ef9345) {
    mem_init(&ef9345->mem);
    mem_map_ram(&ef9345->mem, 0, 0x0000, 0x2000, ef9345->ram); // TODO: add a UI visualisation of this RAM
}

static uint16_t _ef9345_mp_to_physical_address(ef9345_t* ef9345) {
    uint16_t x = ef9345->direct_r7 & 0x3f;
    uint16_t y = ef9345->direct_r6 & 0x1f;
    uint16_t z = ((ef9345->direct_r7 & 0x80) >> 7) |
                 ((ef9345->direct_r7 & 0x40) >> 5) |
                 ((ef9345->direct_r6 & 0x20) >> 3) |
                 ((ef9345->direct_r6 & 0x80) >> 4);

    uint16_t address = x | (y << 6) | (z << 11);
    return address;
}

static uint16_t _ef9345_ap_to_physical_address(ef9345_t* ef9345) {
    uint16_t x = ef9345->direct_r5 & 0x3f;
    uint16_t y = ef9345->direct_r4 & 0x1f;
    uint16_t z = ((ef9345->direct_r5 & 0x80) >> 7) |
                 ((ef9345->direct_r5 & 0x40) >> 5) |
                 ((ef9345->direct_r4 & 0x20) >> 3) |
                 ((ef9345->direct_r6 & 0x40) >> 3);

    uint16_t address = x | (y << 6) | (z << 11);
    return address;
}

static void _ef9345_incr_mp_y(ef9345_t* ef9345) {
    uint8_t y = ef9345->direct_r6 & 0x1f;
    y+=1;
    if (y>31) { y -= 24; }
    ef9345->direct_r6 = (ef9345->direct_r6 & 0xe0) | y;
}

static void _ef9345_incr_mp_x(ef9345_t* ef9345, bool increment_y) {
    uint8_t x = ef9345->direct_r7 & 0x3f;
    x=(x+1)%40;
    ef9345->direct_r7 = (ef9345->direct_r7 & 0xc0) | x;

    if (x == 0 && increment_y) {
        _ef9345_incr_mp_y(ef9345);
    }
}

static void _ef9345_incr_ap_x(ef9345_t* ef9345) {
    uint8_t x = ef9345->direct_r5 & 0x3f;
    x=(x+1)%40;
    ef9345->direct_r7 = (ef9345->direct_r7 & 0xc0) | x;
}

static void _ef9345_start_execute_command(ef9345_t* ef9345) {
    uint8_t command_code = ef9345->direct_r0 & 0xF0;
    uint8_t command_param = ef9345->direct_r0 & 0x0F;
    switch (command_code) {
        case 0x00: // KRx / CLF / CLG
            switch (command_param & 0x06) {
                case 0x00: // KRF
                    printf("Unimplemented 0x00 command: %02X\n", command_param);
                    break;
                case 0x02: { // KRG
                    bool is_read = command_param & 0x08;
                    bool auto_incr = command_param & 0x01;

                    int16_t address = _ef9345_mp_to_physical_address(ef9345);

                    if (is_read) {
                        ef9345->direct_r1 = mem_rd(&ef9345->mem, address);
                        ef9345->direct_r2 = mem_rd(&ef9345->mem, address + 0x0800); // TODO: verify if memory should wrap
                    }
                    else {
                        mem_wr(&ef9345->mem, address, ef9345->direct_r1);
                        mem_wr(&ef9345->mem, address + 0x0800, ef9345->direct_r2); // TODO: verify if memory should wrap
                    }
                    // TODO: direct_r3 is used as a working register. It should be changed. But to what?

                    if (auto_incr) {
                        _ef9345_incr_mp_x(ef9345, false);
                    }

                    // TODO: need to set the status flags
                    // TODO: set execution time
                    break;
                }
                case 0x04: // CLF
                    printf("Unimplemented 0x00 command: %02X\n", command_param);
                    break;
                case 0x06: // CLG
                    printf("Unimplemented 0x00 command: %02X\n", command_param);
                    break;
                default:
                    CHIPS_ASSERT(0 && "EF9345: error in decoding KRx and co.");
                    break;
            }
            break;
        case 0x10: // KRE
            printf("Unimplemented command: %02X\n", command_code);
            break;
        case 0x20: // KRV
            printf("Unimplemented command: %02X\n", command_code);
            break;
        case 0x30: { // OCT
            bool is_read = command_param & 0x08;
            bool is_aux = command_param & 0x04;
            bool auto_incr = command_param & 0x01;

            int16_t address = is_aux?_ef9345_ap_to_physical_address(ef9345):
                                     _ef9345_mp_to_physical_address(ef9345);

            if (is_read) {
                ef9345->direct_r1 = mem_rd(&ef9345->mem, address);
            }
            else {
                mem_wr(&ef9345->mem, address, ef9345->direct_r1);
            }

            if (auto_incr) {
                if (is_aux ) {
                    _ef9345_incr_ap_x(ef9345);
                }
                else {
                    _ef9345_incr_mp_x(ef9345, true);
                }
            }
            // TODO: need to set the status flags
            // TODO: set execution time
            break;
        }
        case 0x40: // KRC
            printf("Unimplemented command: %02X\n", command_code);
            break;
        case 0x50: // KRL
            printf("Unimplemented command: %02X\n", command_code);
            break;
        case 0x60: // EXP
            printf("Unimplemented command: %02X\n", command_code);
            break;
        case 0x70: // CMP
            printf("Unimplemented command: %02X\n", command_code);
            break;
        case 0x80: { // IND
            uint8_t reg_num = command_param & 0x07;
            bool is_read = command_param & 0x08;
            if (is_read) {
                ef9345->direct_r1 = ef9345->indirect_regs[reg_num];
            }
            else { // Write to indirect register
                ef9345->indirect_regs[reg_num] = ef9345->direct_r1;
            }
            // TODO: need to set the status flags
            // TODO: set execution time
            break;
        }
        case 0x90: // VSM / VRM / NOP
            printf("Unimplemented command: %02X\n", command_code);
            break;
        case 0xB0: { // INY
            _ef9345_incr_mp_y(ef9345);
            // TODO: need to set the status flags
            // TODO: set execution time
            break;
        }
        case 0xD0: // MVB
            printf("Unimplemented command: %02X\n", command_code);
            break;
        case 0xE0: // MVD
            printf("Unimplemented command: %02X\n", command_code);
            break;
        case 0xF0: // MVT
            printf("Unimplemented command: %02X\n", command_code);
            break;
        case 0xA0: // ---
        case 0xC0: // ---
        default:
            CHIPS_ASSERT(0 && "EF9345: Unknown command code");
            break;
    }

}

static uint64_t _ef9345_external_bus_transfer(ef9345_t* ef9345, uint64_t vdp_pins) {
    uint64_t previous_pins = ef9345->pins;
    
    // check is AS is a falling edge
    uint8_t previous_as = (previous_pins & EF9345_MASK_AS) >> EF9345_PIN_AS;
    uint8_t current_as = (vdp_pins & EF9345_MASK_AS) >> EF9345_PIN_AS;
    uint8_t is_as_falling_edge = previous_as && !current_as;
    uint8_t is_as_rising_edge = !previous_as && current_as;
    
    if (is_as_falling_edge) {
        ef9345->l_address = EF9345_GET_MUX_DATA_ADDR(vdp_pins);
        ef9345->l_ds = (vdp_pins & EF9345_MASK_DS) >> EF9345_PIN_DS;
        ef9345->execution_flag = ef9345->l_address & 0x08;
    }
    else if (is_as_rising_edge) {
    }

    uint8_t previous_ds = (previous_pins & EF9345_MASK_DS) >> EF9345_PIN_DS;
    uint8_t current_ds = (vdp_pins & EF9345_MASK_DS) >> EF9345_PIN_DS;
    uint8_t is_ds_falling_edge = previous_ds && !current_ds;
    uint8_t is_ds_rising_edge = !previous_ds && current_ds;

    // Normally when DS is low, but to avoid process it 3 times
    // (because EF9345 is ticked 3 times per CPU cycle), I'm only
    // considering the falling edge.)
    if (is_ds_falling_edge) {
        // Read cycle
        if (ef9345->l_ds != 0) { // Only Intel mode is emulated at now
            uint8_t reg_num = ef9345->l_address & 0x0F;
            if (reg_num & 0x07) {
                uint8_t data_out = ef9345->direct_regs[reg_num];
                EF9345_SET_MUX_DATA_ADDR(vdp_pins, data_out);
            } else {
                // TODO: remove when the R0 register is implemented
                uint8_t fake_out_data = 0x00;
                EF9345_SET_MUX_DATA_ADDR(vdp_pins, fake_out_data);
            }
        }
    }
    else if(is_ds_rising_edge) {
    }

    uint8_t previous_rw = (previous_pins & EF9345_MASK_RW) >> EF9345_PIN_RW;
    uint8_t current_rw = (vdp_pins & EF9345_MASK_RW) >> EF9345_PIN_RW;
    uint8_t is_rw_falling_edge = previous_rw && !current_rw;
    uint8_t is_rw_rising_edge = !previous_rw && current_rw;

    // Normally when WR/ is low, but to avoid process it 3 times
    // (because EF9345 is ticked 3 times per CPU cycle), I'm only
    // considering the falling edge.)
    if (is_rw_falling_edge) {
        // Write cycle
        if (ef9345->l_ds != 0) {
            // Only Intel mode is emulated at now
            if ((ef9345->l_address & 0x20) == 0x20) {
                uint8_t data_in = EF9345_GET_MUX_DATA_ADDR(vdp_pins);
                uint8_t reg_num = ef9345->l_address & 0x07;
                ef9345->direct_regs[reg_num] = data_in;
            }
        } 
    }
    else if(is_rw_rising_edge) {
        if (ef9345->execution_flag) {
            // TODO: it is not really clear when the command start.
            // The datasheet says on the rising edge of DS, which implies there's
            // a read cycle after each execution. Which is not what the VG5000µ ROM
            // is doing.
            //
            // If could be at the rising edge of AS, but with the wiring of the VG5000µ,
            // and the next rising edge being issued just to trigger the next falling edge of AS,
            // the execution would be delayed to the next EF9345 command.
            // So at the moment, I'm considering the execution starts at the rising edge of RW.
            _ef9345_start_execute_command(ef9345);
        }
    }

    return vdp_pins;
}

static uint64_t _ef9345_beam_update(ef9345_t* ef9345, uint64_t vdp_pins) {
    ef9345->line_tick = (ef9345->line_tick + 1) % tick_per_line;

    if (ef9345->line_tick == 0) {
        ef9345->current_line = (ef9345->current_line + 1) % 312; // TODO: change number of lines depending on TGS
    }

    // Sets VBLANK for the two first lines
    if (ef9345->current_line < 2) {
        vdp_pins &= ~EF9345_MASK_PC_VS;
    } else {
        vdp_pins |= EF9345_MASK_PC_VS;
    }

    // Sets HBLANK
    if (ef9345->line_tick < tick_hblank_start) {
        vdp_pins &= ~EF9345_MASK_HVS_HS;
    } else {
        vdp_pins |= EF9345_MASK_HVS_HS;
    }

    uint16_t last_scan_line = 312;
    uint16_t active_scan_lines = 250;
    uint16_t first_active_line = last_scan_line - active_scan_lines;
    // TODO: RGB output at 8Mhz for 40c/row, 12Mhz for 80Mhz for c/row
    if (ef9345->current_line >= first_active_line && ef9345->current_line < last_scan_line) {
        // Active lines
        if (ef9345->line_tick < 40*tick_for_1mus) {
            // Active display time
            uint32_t fake_address = ((ef9345->current_line - first_active_line) * 320) +
                                    (ef9345->line_tick * 2);

            ef9345->fb[fake_address] = (fake_address / 4 ) & 7;
        }
    }

    return vdp_pins;
}

#endif // CHIPS_IMPL
