#pragma once
/*#
    # ef9345.h

    Header-only emulator for the Thomson EF9345 display processor.

    Do this:
    ~~~C
    #define CHIPS_IMPL
    ~~~
    before you include this file in *one* C or C++ file to create the
    implementation.

    Optionally provide the following macros with your own implementation

    ~~~C
    CHIPS_ASSERT(c)
    ~~~
        your own assert macro (default: assert(c))

    ## Emulated Pins
    **********************************
    *           +----------+         *
    *    CS/ -->|          |         *
    *    OE/ -->|          |         *
    *    WE/ -->|          |<-> ADM0 *
    *   ASM/ -->|          |...      *
    *     AS -->|          |<-> ADM7 *
    *     DS -->|          |         *
    *   R/W  -->|          |--> AM8  *
    *  PC/VS <--|  EF9345  |...      *
    * HVS/HS <--|          |--> AM13 *
    *      B <--|          |         *
    *      G <--|          |<-- AD0  *
    *      R <--|          |...      *
    *           |          |<-- AD7  *
    *  LPSTB -->|          |         *
    *  RESET -->|          |         *
    *           |          |         *
    *           +----------+         *
    **********************************

    Not emulated:
    - SYNC IN: Synchro in, to synchronize with an external video signal.
    - I: external video signal.
    - HP: video clock, 4Mhz phased with RGBI signals.

#*/
#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>

#ifdef __cplusplus
extern "C" {
#endif

// Microprocessor interface
#define EF9345_PIN_AD0      (17)    /* multiplexed address/data bus */
#define EF9345_PIN_AD1      (18)    /* multiplexed address/data bus */
#define EF9345_PIN_AD2      (19)    /* multiplexed address/data bus */
#define EF9345_PIN_AD3      (21)    /* multiplexed address/data bus */
#define EF9345_PIN_AD4      (22)    /* multiplexed address/data bus */
#define EF9345_PIN_AD5      (23)    /* multiplexed address/data bus */
#define EF9345_PIN_AD6      (24)    /* multiplexed address/data bus */
#define EF9345_PIN_AD7      (25)    /* multiplexed address/data bus */

#define EF9345_PIN_AS       (14)    /* address strobe, falling edge */
#define EF9345_PIN_DS       (15)    /* data strobe, latched by AS */
#define EF9345_PIN_RW       (16)    /* read/write, read high */
#define EF9345_PIN_CS       (26)    /* chip select, latched by AS, active low */

// Memory interface
#define EF9345_PIN_AM13     (27)    /* memory address bus */
#define EF9345_PIN_AM12     (28)    /* memory address bus */
#define EF9345_PIN_AM11     (29)    /* memory address bus */
#define EF9345_PIN_AM10     (30)    /* memory address bus */
#define EF9345_PIN_AM9      (31)    /* memory address bus */
#define EF9345_PIN_AM8      (32)    /* memory address bus */
#define EF9345_PIN_ADM7     (33)    /* multiplexed address/data mode */
#define EF9345_PIN_ADM6     (34)    /* multiplexed address/data mode */
#define EF9345_PIN_ADM5     (35)    /* multiplexed address/data mode */
#define EF9345_PIN_ADM4     (36)    /* multiplexed address/data mode */
#define EF9345_PIN_ADM3     (37)    /* multiplexed address/data mode */
#define EF9345_PIN_ADM2     (38)    /* multiplexed address/data mode */
#define EF9345_PIN_ADM1     (39)    /* multiplexed address/data mode */
#define EF9345_PIN_ADM0     (40)    /* multiplexed address/data mode */

#define EF9345_PIN_OE       (2)     /* output enable, active low */
#define EF9345_PIN_WE       (3)     /* write enable, active low */
#define EF9345_PIN_ASM      (4)     /* memory address strobe mask, falling edge */

// Video interface
#define EF9345_PIN_HVS_HS   (5)     /* horizontal/vertical composite sync / horizontal sync, active high */
#define EF9345_PIN_PC_VS    (6)     /* phase comparator / vertical sync, active high */
#define EF9345_PIN_B        (7)     /* blue */
#define EF9345_PIN_G        (8)     /* green */
#define EF9345_PIN_R        (9)     /* red */

// pin bit masks
#define EF9345_MASK_OE      (1<<EF9345_PIN_OE)
#define EF9345_MASK_WE      (1<<EF9345_PIN_WE)
#define EF9345_MASK_ASM     (1<<EF9345_PIN_ASM)
#define EF9345_MASK_HVS_HS  (1<<EF9345_PIN_HVS_HS)
#define EF9345_MASK_PC_VS   (1<<EF9345_PIN_PC_VS)
#define EF9345_MASK_B       (1<<EF9345_PIN_B)
#define EF9345_MASK_G       (1<<EF9345_PIN_G)
#define EF9345_MASK_R       (1<<EF9345_PIN_R)
#define EF9345_MASK_AS      (1<<EF9345_PIN_AS)
#define EF9345_MASK_DS      (1<<EF9345_PIN_DS)
#define EF9345_MASK_RW      (1<<EF9345_PIN_RW)
#define EF9345_MASK_AD0     (1<<EF9345_PIN_AD0)
#define EF9345_MASK_AD1     (1<<EF9345_PIN_AD1)
#define EF9345_MASK_AD2     (1<<EF9345_PIN_AD2)
#define EF9345_MASK_AD3     (1<<EF9345_PIN_AD3)
#define EF9345_MASK_AD4     (1<<EF9345_PIN_AD4)
#define EF9345_MASK_AD5     (1<<EF9345_PIN_AD5)
#define EF9345_MASK_AD6     (1<<EF9345_PIN_AD6)
#define EF9345_MASK_AD7     (1<<EF9345_PIN_AD7)
#define EF9345_MASK_CS      (1<<EF9345_PIN_CS)
#define EF9345_MASK_AM13    (1<<EF9345_PIN_AM13)
#define EF9345_MASK_AM12    (1<<EF9345_PIN_AM12)
#define EF9345_MASK_AM11    (1<<EF9345_PIN_AM11)
#define EF9345_MASK_AM10    (1<<EF9345_PIN_AM10)
#define EF9345_MASK_AM9     (1<<EF9345_PIN_AM9)
#define EF9345_MASK_AM8     (1<<EF9345_PIN_AM8)
#define EF9345_MASK_ADM7    (1<<EF9345_PIN_ADM7)
#define EF9345_MASK_ADM6    (1<<EF9345_PIN_ADM6)
#define EF9345_MASK_ADM5    (1<<EF9345_PIN_ADM5)
#define EF9345_MASK_ADM4    (1<<EF9345_PIN_ADM4)
#define EF9345_MASK_ADM3    (1<<EF9345_PIN_ADM3)
#define EF9345_MASK_ADM2    (1<<EF9345_PIN_ADM2)
#define EF9345_MASK_ADM1    (1<<EF9345_PIN_ADM1)
#define EF9345_MASK_ADM0    (1<<EF9345_PIN_ADM0)

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

    uint64_t pins;                  /* pin state after last tick */
    uint16_t fb_width;
    uint16_t fb_height;
    uint32_t fb_size;
    alignas(64) uint8_t fb[EF9345_FRAMEBUFFER_SIZE];

} ef9345_t;

/* helper macros to extract address and data values from pin mask */

#define EF9345_AD0_AD7_MASK (EF9345_MASK_AD0 | EF9345_MASK_AD1 | EF9345_MASK_AD2 | EF9345_MASK_AD3 | EF9345_MASK_AD4 | EF9345_MASK_AD5 | EF9345_MASK_AD6 | EF9345_MASK_AD7)
#define EF9345_ADM0_ADM7_MASK (EF9345_MASK_ADM0 | EF9345_MASK_ADM1 | EF9345_MASK_ADM2 | EF9345_MASK_ADM3 | EF9345_MASK_ADM4 | EF9345_MASK_ADM5 | EF9345_MASK_ADM6 | EF9345_MASK_ADM7)
#define EF9345_AM8_AM13_MASK (EF9345_MASK_AM8 | EF9345_MASK_AM9 | EF9345_MASK_AM10 | EF9345_MASK_AM11 | EF9345_MASK_AM12 | EF9345_MASK_AM13)
#define EF9345_ADM0_AM13_MASK (EF9345_ADM0_ADM7_MASK | EF9345_AM8_AM13_MASK)

/* extract multiplexed data/bus from AD0-AD7 pins */
#define EF9345_GET_MUX_DATA_ADDR(p) ((uint8_t)(((p)&EF9345_AD0_AD7_MASK)>>EF9345_PIN_AD0))
/* set multiplexed data/bus to AD0-AD7 pins */
#define EF9345_SET_MUX_DATA_ADDR(p, d) {((p) = ((p)&~EF9345_AD0_AD7_MASK)|(((d)&0xff)<<EF9345_PIN_AD0));}
/* extract multiplexed address from ADM0-ADM7 and AM8-AM13 pins */
#define EF9345_GET_MUX_ADDR(p) ((uint16_t)(((p)&EF9345_ADM0_AM13_MASK)>>EF9345_PIN_ADM0))
/* set multiplexed address to ADM0-ADM7 and AM8-AM13 pins */
#define EF9345_SET_MUX_ADDR(p, a) {((p) = ((p)&~EF9345_ADM0_AM13_MASK)|(((a)&0x3fff)<<EF9345_PIN_ADM0));}
/* extract multiplexed data from ADM0-ADM7 pins */
#define EF9345_GET_MUX_DATA(p) ((uint8_t)(((p)&EF9345_ADM0_ADM7_MASK)>>EF9345_PIN_ADM0))
/* set multiplexed data to ADM0-ADM7 pins */
#define EF9345_SET_MUX_DATA(p, d) {((p) = ((p)&~EF9345_ADM0_ADM7_MASK)|(((d)&0xff)<<EF9345_PIN_ADM0));}

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

void ef9345_init(ef9345_t* ef9345) {
    CHIPS_ASSERT(ef9345);
    memset(ef9345, 0, sizeof(*ef9345));

    ef9345->fb_width = EF9345_FRAMEBUFFER_WIDTH;
    ef9345->fb_height = EF9345_FRAMEBUFFER_HEIGHT;
    ef9345->fb_size = EF9345_FRAMEBUFFER_SIZE;

}

void ef9345_reset(ef9345_t* ef9345) {
    CHIPS_ASSERT(ef9345);
    ef9345_init(ef9345);
}

uint64_t ef9345_tick(ef9345_t* ef9345, uint64_t vdp_pins) {
    CHIPS_ASSERT(ef9345);

    // update scanline

    static uint16_t fake_counter = 0;
    ef9345->fb[fake_counter & 0x1FFF] = (fake_counter / 4 ) & 7;
    fake_counter++;

    return vdp_pins;
}

#endif // CHIPS_IMPL
