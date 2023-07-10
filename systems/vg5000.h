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

// TODO: verify how it works
#define VG5000_FRAMEBUFFER_SIZE (320*200)
#define VG5000_PALETTE_SIZE (16*3)

// VG5000 emulator state
typedef struct {
    z80_t cpu;
    beeper_t beeper;
    vg5000_type_t type;
    uint32_t tick_count;
    uint8_t blink_counter;
    kbd_t kbd;
    mem_t mem;
    uint64_t pins;
    uint64_t freq_hz;
    chips_debug_t debug;
    uint8_t ram[8][0x4000]; // TODO: verify
    uint8_t rom[2][0x4000]; // TODO: verify
    // TODO: to replace with EF9345 framebuffer?
    alignas(64) uint8_t fb[VG5000_FRAMEBUFFER_SIZE];
    uint8_t palette[VG5000_PALETTE_SIZE];
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
void vg5000_save_snapshot(vg5000_t* sys, vg5000_t* dst);
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

void vg5000_init(vg5000_t* sys, const vg5000_desc_t* desc)
{}

void vg5000_discard(vg5000_t* sys)
{}

void vg5000_reset(vg5000_t* sys)
{}

chips_display_info_t vg5000_display_info(vg5000_t* sys)
{
    const chips_display_info_t res = {
        .frame = {
            .dim = {
                .width = 320,   // TODO: verify
                .height = 200   // TODO: verify (and take for EF9345)
            },
            .bytes_per_pixel = 1,   // TODO: verify
            .buffer = {
                .ptr = sys?sys->fb:0,     // TODO: get from EF9345 ?
                .size = VG5000_FRAMEBUFFER_SIZE,    // TODO: get from EF9345
            }
        },
        .screen = {
            .x = 0,
            .y = 0,
            .width = 320, // TODO: get from EF9345
            .height = 200, // TODO: get from EF9345
        },
        .palette = {
            .ptr = sys?sys->palette:0, // TODO: get from EF9345
            .size = VG5000_PALETTE_SIZE // TODO: get from EF9345
        }
    };

    CHIPS_ASSERT(((sys == 0) && (res.frame.buffer.ptr == 0)) || ((sys != 0) && (res.frame.buffer.ptr != 0)));
    CHIPS_ASSERT(((sys == 0) && (res.palette.ptr == 0)) || ((sys != 0) && (res.palette.ptr != 0)));

    return res;
}

uint32_t vg5000_exec(vg5000_t* sys, uint32_t micro_seconds)
{}

void vg5000_key_down(vg5000_t* sys, int key_code)
{}

void vg5000_key_up(vg5000_t* sys, int key_code)
{}

bool vg5000_quickload(vg5000_t* sys, chips_range_t data)
{}

void vg5000_save_snapshot(vg5000_t* sys, vg5000_t* dst)
{}

bool vg5000_load_snapshot(vg5000_t* sys, uint32_t version, vg5000_t* src)
{}


#endif // CHIPS_IMPL
