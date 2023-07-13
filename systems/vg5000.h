/*#

    # vg5000.h

    A VG5000 emulator in a C header.

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

    You need to include the following headers before including vg5000.h:

        - TODO: provide list of dependencies

#*/

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdalign.h>

#ifdef __cplusplus
extern "C" {
#endif

// bump this whenever the vg5000_t struct layout changes
#define VG5000_SNAPSHOT_VERSION (0x0001)

#define VG5000_FREQUENCY (4000000)
#define VDP_TICKS_PER_CPU_TICK (EF9345_FREQUENCY / VG5000_FREQUENCY)

// VG5000µ models
// TODO: useful or redondant with ROM images?
typedef enum {
    VG5000_TYPE_10,
    VG5000_TYPE_11,
} vg5000_type_t;

// Config parameters for vg5000_init()
typedef struct {
    vg5000_type_t type;     // VG5000 model type
    chips_debug_t debug;    // optional debugger hook
    // TODO: audio state
    // ROM images
    struct {
        chips_range_t vg5000_10;
        chips_range_t vg5000_11;
    } roms;
} vg5000_desc_t;


// VG5000 emulator state
typedef struct {
    z80_t cpu;
    ef9345_t vdp;
    beeper_t beeper;
    vg5000_type_t type;
    uint32_t tick_count;
    uint8_t blink_counter;
    kbd_t kbd;
    mem_t mem;
    uint64_t cpu_pins;
    uint64_t service_bus;
    uint64_t freq_hz;
    chips_debug_t debug;
    uint8_t ram[8][0x4000]; // TODO: verify
    uint8_t rom[0x4000];
    bool valid;
} vg5000_t;

// initialize a new VG5000µ instance
void vg5000_init(vg5000_t* sys, const vg5000_desc_t* desc);
// discard a VG5000µ instance
void vg5000_discard(vg5000_t* sys);
// reset a VG5000µ instance
void vg5000_reset(vg5000_t* sys);
// query information about display requirements, can be called with nullptr
chips_display_info_t vg5000_display_info(vg5000_t* sys);
// run the VG5000µ instance for a given number of microseconds, return number of ticks
uint32_t vg5000_exec(vg5000_t* sys, uint32_t micro_seconds);
// send a key-down event
void vg5000_key_down(vg5000_t* sys, int key_code);
// send a key-up event
void vg5000_key_up(vg5000_t* sys, int key_code);
// load a VG5000µ file into the emulator
bool vg5000_quickload(vg5000_t* sys, chips_range_t data);
// save the VG5000µ state
uint32_t vg5000_save_snapshot(vg5000_t* sys, vg5000_t* dst);
// load a VG5000µ state
bool vg5000_load_snapshot(vg5000_t* sys, uint32_t version, vg5000_t* src);

#ifdef __cplusplus
} // extern "C"
#endif

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef CHIPS_IMPL

#ifndef CHIPS_ASSERT
    #include <assert.h>
    #define CHIPS_ASSERT(c) assert(c)
#endif

#define SERVICE_BUS_PIN_RKY 0
#define SERVICE_BUS_PIN_RK7 1
#define SERVICE_BUS_PIN_WK7 2
#define SERVICE_BUS_MASK_RKY (1 << SERVICE_BUS_PIN_RKY)
#define SERVICE_BUS_MASK_RK7 (1 << SERVICE_BUS_PIN_RK7)
#define SERVICE_BUS_MASK_WK7 (1 << SERVICE_BUS_PIN_WK7)

static void _vg5000_init_memory_map(vg5000_t* sys);
static void _vg5000_init_keyboard_matrix(vg5000_t* sys);

void vg5000_init(vg5000_t* sys, const vg5000_desc_t* desc)
{
    CHIPS_ASSERT(sys && desc);
    if (desc->debug.callback.func) { CHIPS_ASSERT(desc->debug.stopped); }

    memset(sys, 0, sizeof(vg5000_t));
    sys->valid = true;
    sys->freq_hz = VG5000_FREQUENCY;
    sys->debug = desc->debug;

    ef9345_init(&sys->vdp);

    memcpy(sys->rom, desc->roms.vg5000_11.ptr, desc->roms.vg5000_11.size);
    _vg5000_init_memory_map(sys);
    _vg5000_init_keyboard_matrix(sys);
}

void vg5000_discard(vg5000_t* sys)
{
    CHIPS_ASSERT(sys && sys->valid);
    sys->valid = false;
}

void vg5000_reset(vg5000_t* sys)
{
    CHIPS_ASSERT(sys && sys->valid);
    sys->cpu_pins = z80_reset(&sys->cpu);
    beeper_reset(&sys->beeper);
    ef9345_reset(&sys->vdp);
    _vg5000_init_memory_map(sys);
}

chips_display_info_t vg5000_display_info(vg5000_t* sys)
{
    static const uint32_t palette[8] = {
        0xFF000000,     // black
        0xFF0000FF,     // red
        0xFF00FF00,     // green
        0xFF00FFFF,     // yellow
        0xFFFF0000,     // blue
        0xFFFF00FF,     // magenta
        0xFFFFFF00,     // cyan
        0xFFFFFFFF,     // white
    };

    const chips_display_info_t res = {
        .frame = {
            .dim = {
                .width = sys?sys->vdp.fb_width:320,
                .height = sys?sys->vdp.fb_height:250
            },
            .bytes_per_pixel = 1,   // TODO: verify
            .buffer = {
                .ptr = sys?sys->vdp.fb:0,
                .size = sys?sys->vdp.fb_size:0,
            }
        },
        .screen = {
            .x = 0,
            .y = 0,
            .width = sys?sys->vdp.fb_width:320,
            .height = sys?sys->vdp.fb_height:250,
        },
        .palette = {
            .ptr = sys?(void*)palette:0,  // TODO: get from EF9345 ?
            .size = sys?sizeof(palette):0 // TODO: get from EF9345
        }
    };

    CHIPS_ASSERT((res.frame.dim.width > 0) && (res.frame.dim.height > 0)); // Expected by gfx
    CHIPS_ASSERT(((sys == 0) && (res.frame.buffer.ptr == 0)) || ((sys != 0) && (res.frame.buffer.ptr != 0)));
    CHIPS_ASSERT(((sys == 0) && (res.palette.ptr == 0)) || ((sys != 0) && (res.palette.ptr != 0)));

    return res;
}

void _vg5000_7807_decoder(const uint64_t * cpu_pins, uint64_t * vdp_pins, uint64_t * service_bus) {
    // remember active is always high in this emulator for z80 signals
    // signals generated from 7807 are in their physical state

    // G2/ is high if either RD/ or WR/ are asserted while IORQ/ is asserted
    uint8_t input_G2 = (((*cpu_pins & Z80_RD) >> Z80_PIN_RD) |
                        ((*cpu_pins & Z80_WR) >> Z80_PIN_WR)) & 
                       ((*cpu_pins & Z80_IORQ) >> Z80_PIN_IORQ);
    input_G2 = !input_G2; // turn to physical state -> active low

    // G1/ is A7
    uint8_t input_G1 = (*cpu_pins & Z80_A7) >> Z80_PIN_A7;

    // To have an output line selected, G1 must be HIGH and G2 must be LOW
    uint8_t output;
    if (input_G1 && !input_G2) {
        uint8_t input_a = (*cpu_pins & Z80_A5) >> Z80_PIN_A5;
        uint8_t input_b = (*cpu_pins & Z80_A6) >> Z80_PIN_A6;
        uint8_t input_c = (*cpu_pins & Z80_WR) >> Z80_PIN_WR;
        input_c = (~input_c)&1; // turn to physical state -> active low

        uint8_t shift = (input_c << 2) | (input_b << 1) | input_a;
        output = 1 << shift;    // Selected line, logic
        output = ~output;       // Selected line, physical
    } else {
        output = 0xFF;
    }

    uint64_t locale_vdp_pins = *vdp_pins;

    locale_vdp_pins &= ~EF9345_MASK_AS;
    locale_vdp_pins |= (~output & 0x01) << EF9345_PIN_AS;

    locale_vdp_pins &= ~EF9345_MASK_DS;
    locale_vdp_pins |= ((output & 0x40) >> 6) << EF9345_PIN_DS;

    locale_vdp_pins &= ~EF9345_MASK_RW;
    locale_vdp_pins |= ((output & 0x04) >> 2) << EF9345_PIN_RW;

    *vdp_pins = locale_vdp_pins;

    uint64_t local_service_bus = *service_bus;
    local_service_bus &= ~SERVICE_BUS_MASK_RKY;
    local_service_bus |= ((output & 0x10) >> 4) << SERVICE_BUS_PIN_RKY;

    local_service_bus &= ~SERVICE_BUS_MASK_RK7;
    local_service_bus |= ((output & 0x20) >> 5) << SERVICE_BUS_PIN_RK7;

    local_service_bus &= ~SERVICE_BUS_MASK_WK7;
    local_service_bus |= ((output & 0x02) >> 1) << SERVICE_BUS_PIN_WK7;

    *service_bus = local_service_bus;
}

uint64_t _vg5000_tick(vg5000_t* sys, uint64_t cpu_pins)
{
    // tick the CPU
    cpu_pins = z80_tick(&sys->cpu, cpu_pins);

    // TODO: update beeper
    
    if (cpu_pins & Z80_MREQ) {
        // TODO: check memory mapping depending on the configuration
        const uint16_t addr = Z80_GET_ADDR(cpu_pins);
        if (cpu_pins & Z80_RD) {
            Z80_SET_DATA(cpu_pins, mem_rd(&sys->mem, addr));
        }
        else if (cpu_pins & Z80_WR) {
            // write to memory
            mem_wr(&sys->mem, addr, Z80_GET_DATA(cpu_pins));
        }
    }
    
    // Decode EF9347 and K7 control signals
    uint64_t vdp_pins = sys->vdp.pins;
    _vg5000_7807_decoder(&cpu_pins, &vdp_pins, &sys->service_bus);

    uint8_t z80_data = Z80_GET_DATA(cpu_pins);
    EF9345_SET_MUX_DATA_ADDR(vdp_pins, z80_data);

    // This is a shortcut, as the VDP is updating in parallel to the CPU
    for (int vdp_update = 0; vdp_update < VDP_TICKS_PER_CPU_TICK; vdp_update++) {
        vdp_pins = ef9345_tick(&sys->vdp, vdp_pins);
    }

    // If read phase from the EF9345, apply the data to the Z80 data bus
    uint8_t ds = (vdp_pins & EF9345_MASK_DS) >> EF9345_PIN_DS;
    if (!ds)
    {
        Z80_SET_DATA(cpu_pins, EF9345_GET_MUX_DATA_ADDR(vdp_pins));
    }

    return cpu_pins;
}

uint32_t vg5000_exec(vg5000_t* sys, uint32_t micro_seconds)
{
    CHIPS_ASSERT(sys && sys->valid);
    uint32_t num_ticks = clk_us_to_ticks(VG5000_FREQUENCY, micro_seconds);
    uint64_t pins = sys->cpu_pins;
    if (0 == sys->debug.callback.func) {
        // run without debug hook
        for (uint32_t ticks = 0; ticks < num_ticks; ticks++) {
            pins = _vg5000_tick(sys, pins);
        }
    }
    else {
        // run with debug hook]
        for (uint32_t ticks = 0; (ticks < num_ticks) && !(*sys->debug.stopped); ticks++) {
            pins = _vg5000_tick(sys, pins);
            sys->debug.callback.func(sys->debug.callback.user_data, pins);
        }
    }
    sys->cpu_pins = pins;
    kbd_update(&sys->kbd, micro_seconds);
    return num_ticks;

}

void vg5000_key_down(vg5000_t* sys, int key_code)
{}

void vg5000_key_up(vg5000_t* sys, int key_code)
{}

bool vg5000_quickload(vg5000_t* sys, chips_range_t data)
{
    return false;
}

uint32_t vg5000_save_snapshot(vg5000_t* sys, vg5000_t* dst)
{
    return 0;
}

bool vg5000_load_snapshot(vg5000_t* sys, uint32_t version, vg5000_t* src)
{
    return false;
}

static void _vg5000_init_memory_map(vg5000_t* sys) {
    mem_init(&sys->mem);
    // TODO: check memory mapping depending on the configuration
    mem_map_rom(&sys->mem, 0, 0x0000, 0x4000, sys->rom);
    mem_map_ram(&sys->mem, 0, 0x4000, 0x4000, sys->ram[0]);
    mem_map_ram(&sys->mem, 0, 0x8000, 0x4000, sys->ram[1]);
    mem_map_ram(&sys->mem, 0, 0xC000, 0x4000, sys->ram[2]);
}

static void _vg5000_init_keyboard_matrix(vg5000_t* sys)
{
    kbd_init(&sys->kbd, 1);
    kbd_register_modifier(&sys->kbd, 0, 2, 0);
    // kbd_register_modifier(&sys->kbd, 0, 2, 0);


    const char* keymap =
        /* no shift */
       /* no shift */
        "        "
        "A     Q "
        "Z:1BVCXW"
        ";26543ES"
        "POIUGF*/" // * -> ×
        "987, ]0 "
        "D <YTR+-"
        "MLKHJN ="

        // shift
        "        "
        "a     q "
        "z*#bvcxw"
        "@!%%$£\"es"
        "poiugfx/" // / -> ÷
        "(/&/ [) " // twice / ?
        "d >ytr+?" // shift+ -> ?
        "mlkhjn ^";

    for (int layer = 0; layer < 2; layer++) {
        for (int column = 0; column < 8; column++) {
            for (int line = 0; line < 8; line++) {
                const uint8_t c = keymap[layer*64 + column*8 + line];
                if (c != 0x20) {
                    kbd_register_key(&sys->kbd, c, column, line, (layer>0) ? (1<<(layer-1)) : 0);
                }
            }
        }
    }

    // special keys
    // TODO: understand and fix the key codes
    kbd_register_key(&sys->kbd, 0x0F, 7, 0, 0); // INS
    kbd_register_key(&sys->kbd, 0x08, 6, 0, 0); // CTRL
    kbd_register_key(&sys->kbd, 0x0A, 5, 0, 0); // Cursor Down
    kbd_register_key(&sys->kbd, 0x0B, 4, 0, 0); // Cursor Right
    kbd_register_key(&sys->kbd, 0x0B, 3, 0, 0); // Cursor Left
    kbd_register_key(&sys->kbd, 0x0B, 2, 0, 0); // ???
    kbd_register_key(&sys->kbd, 0x09, 0, 0, 0); // LIST
    kbd_register_key(&sys->kbd, 0x09, 6, 1, 0); // Cursor Up
    kbd_register_key(&sys->kbd, 0x0C, 5, 1, 0); // RET
    kbd_register_key(&sys->kbd, 0x0D, 3, 1, 0); // CAPS LOCK
    kbd_register_key(&sys->kbd, ' ', 2, 1, 0); // ESP
    kbd_register_key(&sys->kbd, 0x0E, 6, 6, 0); // PRT
    kbd_register_key(&sys->kbd, 0x0E, 7, 1, 0); // EFF
    // TODO: Add the Triangle Key

}


#endif // CHIPS_IMPL
