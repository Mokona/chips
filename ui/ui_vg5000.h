#pragma once
/*#
    # ui_vg5000.h

    Integrated debugging UI for vg5000.h

    Include the following headers (and their dependencies) before including
    ui_vg5000.h both for the declaration and implementation.

    - vg5000.h
    - mem.h
    - TODO: check needed dependencies

    - TODO: check if using zlib/libpng

    ## zlib/libpng license

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

#ifdef __cplusplus
extern "C" {
#endif

/* reboot callback */
typedef void (*ui_vg5000_boot_cb)(vg5000_t* sys);

typedef struct {
    vg5000_t* vg5000;
    ui_vg5000_boot_cb boot_cb;
    ui_dbg_texture_callbacks_t dbg_texture;     // texture create/update/destroy callbacks
    ui_dbg_keys_desc_t dbg_keys;                // user-defined hotkeys for ui_dbg_t
    ui_snapshot_desc_t snapshot;                // snapshot ui setup params
} ui_vg5000_desc_t;

typedef struct {
    vg5000_t* vg5000;
    ui_vg5000_boot_cb boot_cb;
    ui_z80_t cpu;
    ui_ef9345_t vdp;
    ui_audio_t audio;
    ui_kbd_t kbd;
    ui_memmap_t memmap;
    ui_memedit_t memedit[4];
    ui_dasm_t dasm[4];
    ui_dbg_t dbg;
    ui_snapshot_t snapshot;
} ui_vg5000_t;

void ui_vg5000_init(ui_vg5000_t* ui, const ui_vg5000_desc_t* desc);
void ui_vg5000_discard(ui_vg5000_t* ui);
void ui_vg5000_draw(ui_vg5000_t* ui);
chips_debug_t ui_vg5000_get_debug(ui_vg5000_t* ui);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*-- IMPLEMENTATION (include in C++ source) ----------------------------------*/
#ifdef CHIPS_UI_IMPL
#ifndef __cplusplus
#error "implementation must be compiled as C++"
#endif
#include <string.h> /* memset */
#ifndef CHIPS_ASSERT
    #include <assert.h>
    #define CHIPS_ASSERT(c) assert(c)
#endif
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

static void _ui_vg5000_draw_menu(ui_vg5000_t* ui) {
    CHIPS_ASSERT(ui && ui->vg5000 && ui->boot_cb);
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("System")) {
            ui_snapshot_menus(&ui->snapshot);
            if (ImGui::MenuItem("Reset")) {
                vg5000_reset(ui->vg5000);
                ui_dbg_reset(&ui->dbg);
            }
            if (ImGui::MenuItem("Cold Boot")) {
                ui->boot_cb(ui->vg5000);
                ui_dbg_reboot(&ui->dbg);
            }
            // TODO: joystick support
            // if (ImGui::BeginMenu("Joystick")) {
            //     if (ImGui::MenuItem("None", 0, (ui->vg5000->joystick_type == vg5000_JOYSTICKTYPE_NONE))) {
            //         ui->vg5000->joystick_type = vg5000_JOYSTICKTYPE_NONE;
            //     }
            //     if (ImGui::MenuItem("MMC", 0, (ui->vg5000->joystick_type == vg5000_JOYSTICKTYPE_MMC))) {
            //         ui->vg5000->joystick_type = vg5000_JOYSTICKTYPE_MMC;
            //     }
            //     ImGui::EndMenu();
            // }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Hardware")) {
            ImGui::MenuItem("Memory Map", 0, &ui->memmap.open);
            ImGui::MenuItem("Keyboard Matrix", 0, &ui->kbd.open);
            ImGui::MenuItem("Audio Output", 0, &ui->audio.open);
            ImGui::MenuItem("Z80 CPU", 0, &ui->cpu.open);
            ImGui::MenuItem("EF9345 VDP", 0, &ui->vdp.open);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug")) {
            ImGui::MenuItem("CPU Debugger", 0, &ui->dbg.ui.open);
            ImGui::MenuItem("Breakpoints", 0, &ui->dbg.ui.show_breakpoints);
            ImGui::MenuItem("Execution History", 0, &ui->dbg.ui.show_history);
            ImGui::MenuItem("Memory Heatmap", 0, &ui->dbg.ui.show_heatmap);
            if (ImGui::BeginMenu("Memory Editor")) {
                ImGui::MenuItem("Window #1", 0, &ui->memedit[0].open);
                ImGui::MenuItem("Window #2", 0, &ui->memedit[1].open);
                ImGui::MenuItem("Window #3", 0, &ui->memedit[2].open);
                ImGui::MenuItem("Window #4", 0, &ui->memedit[3].open);
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Disassembler")) {
                ImGui::MenuItem("Window #1", 0, &ui->dasm[0].open);
                ImGui::MenuItem("Window #2", 0, &ui->dasm[1].open);
                ImGui::MenuItem("Window #3", 0, &ui->dasm[2].open);
                ImGui::MenuItem("Window #4", 0, &ui->dasm[3].open);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ui_util_options_menu();
        ImGui::EndMainMenuBar();
    }
}

static void _ui_vg5000_update_memmap(ui_vg5000_t* ui) {
    CHIPS_ASSERT(ui && ui->vg5000);
    ui_memmap_reset(&ui->memmap);
    ui_memmap_layer(&ui->memmap, "ROM");
        ui_memmap_region(&ui->memmap, "ROM", 0x0000, 0x4000, true);
    ui_memmap_layer(&ui->memmap, "MAIN RAM");
        ui_memmap_region(&ui->memmap, "VIDEO RAM", 0x4000, 0x4000, true);
    // TODO: only show extended when present
    ui_memmap_layer(&ui->memmap, "EXTENDED");
        // TODO: dynamic when extended ROM is supported
        ui_memmap_region(&ui->memmap, "RAM", 0x8000, 0x8000, true);
    // TODO: make a BASIC pointer view
}


static uint8_t _ui_vg5000_mem_read(int layer, uint16_t addr, void* user_data) {
    CHIPS_ASSERT(user_data);
    (void)layer;
    vg5000_t* vg5000 = (vg5000_t*) user_data;
    return mem_rd(&vg5000->mem, addr);
}

static void _ui_vg5000_mem_write(int layer, uint16_t addr, uint8_t data, void* user_data) {
    CHIPS_ASSERT(user_data);
    (void)layer;
    vg5000_t* vg5000 = (vg5000_t*) user_data;
    mem_wr(&vg5000->mem, addr, data);
}

static const ui_chip_pin_t _ui_vg5000_cpu_pins[] = {
    { "D0",     0,      Z80_D0 },
    { "D1",     1,      Z80_D1 },
    { "D2",     2,      Z80_D2 },
    { "D3",     3,      Z80_D3 },
    { "D4",     4,      Z80_D4 },
    { "D5",     5,      Z80_D5 },
    { "D6",     6,      Z80_D6 },
    { "D7",     7,      Z80_D7 },
    { "M1",     8,      Z80_M1 },
    { "MREQ",   9,      Z80_MREQ },
    { "IORQ",   10,     Z80_IORQ },
    { "RD",     11,     Z80_RD },
    { "WR",     12,     Z80_WR },
    { "RFSH",   13,     Z80_RFSH },
    { "HALT",   14,     Z80_HALT },
    { "INT",    15,     Z80_INT },
    { "NMI",    16,     Z80_NMI },
    { "WAIT",   17,     Z80_WAIT },
    { "A0",     18,     Z80_A0 },
    { "A1",     19,     Z80_A1 },
    { "A2",     20,     Z80_A2 },
    { "A3",     21,     Z80_A3 },
    { "A4",     22,     Z80_A4 },
    { "A5",     23,     Z80_A5 },
    { "A6",     24,     Z80_A6 },
    { "A7",     25,     Z80_A7 },
    { "A8",     26,     Z80_A8 },
    { "A9",     27,     Z80_A9 },
    { "A10",    28,     Z80_A10 },
    { "A11",    29,     Z80_A11 },
    { "A12",    30,     Z80_A12 },
    { "A13",    31,     Z80_A13 },
    { "A14",    32,     Z80_A14 },
    { "A15",    33,     Z80_A15 },
};

static const ui_chip_pin_t _ui_ef9345_vdp_pins[] = {
    { "AD0",    0,      EF9345_MASK_AD0 },
    { "AD1",    1,      EF9345_MASK_AD1 },
    { "AD2",    2,      EF9345_MASK_AD2 },
    { "AD3",    3,      EF9345_MASK_AD3 },
    { "AD4",    4,      EF9345_MASK_AD4 },
    { "AD5",    5,      EF9345_MASK_AD5 },
    { "AD6",    6,      EF9345_MASK_AD6 },
    { "AD7",    7,      EF9345_MASK_AD7 },
    { "AS",     8,      EF9345_MASK_AS },
    { "DS",     9,      EF9345_MASK_DS },
    { "R/W",    10,     EF9345_MASK_RW },
    { "PC/VS",  11,     EF9345_MASK_PC_VS },
    { "HVS/HS", 12,     EF9345_MASK_HVS_HS },
    { "ADM0",   13,     EF9345_MASK_ADM0 },
    { "ADM1",   14,     EF9345_MASK_ADM1 },
    { "ADM2",   15,     EF9345_MASK_ADM2 },
    { "ADM3",   16,     EF9345_MASK_ADM3 },
    { "ADM4",   17,     EF9345_MASK_ADM4 },
    { "ADM5",   18,     EF9345_MASK_ADM5 },
    { "ADM6",   19,     EF9345_MASK_ADM6 },
    { "ADM7",   20,     EF9345_MASK_ADM7 },
    { "AM8",    21,     EF9345_MASK_AM8 },
    { "AM9",    22,     EF9345_MASK_AM9 },
    { "AM10",   23,     EF9345_MASK_AM10 },
    { "AM11",   24,     EF9345_MASK_AM11 },
    { "AM12",   25,     EF9345_MASK_AM12 },
    { "AM13",   26,     EF9345_MASK_AM13 },
    { "OE",     27,     EF9345_MASK_OE },
    { "WE",     28,     EF9345_MASK_WE },
    { "ASM",    29,     EF9345_MASK_ASM },
};


void ui_vg5000_init(ui_vg5000_t* ui, const ui_vg5000_desc_t* ui_desc) {
    CHIPS_ASSERT(ui && ui_desc);
    CHIPS_ASSERT(ui_desc->vg5000);
    CHIPS_ASSERT(ui_desc->boot_cb);
    ui_snapshot_init(&ui->snapshot, &ui_desc->snapshot);
    ui->vg5000 = ui_desc->vg5000;
    ui->boot_cb = ui_desc->boot_cb;
    int x = 20, y = 20, dx = 10, dy = 10;
    {
        ui_dbg_desc_t desc = {0};
        desc.title = "CPU Debugger";
        desc.x = x;
        desc.y = y;
        desc.z80 = &ui->vg5000->cpu;
        desc.read_cb = _ui_vg5000_mem_read;
        desc.texture_cbs = ui_desc->dbg_texture;
        desc.keys = ui_desc->dbg_keys;
        desc.user_data = ui->vg5000;
        ui_dbg_init(&ui->dbg, &desc);
    }
    x += dx; y += dy;
    {
        ui_z80_desc_t desc = {0};
        desc.title = "Z80 CPU";
        desc.cpu = &ui->vg5000->cpu;
        desc.x = x;
        desc.y = y;
        UI_CHIP_INIT_DESC(&desc.chip_desc, "Z80\nCPU", 36, _ui_vg5000_cpu_pins);
        ui_z80_init(&ui->cpu, &desc);
    }
    x += dx; y += dy;
    {
        ui_ef9345_desc_t desc = {0};
        desc.title = "EF9345 VDP";
        desc.vdp = &ui->vg5000->vdp;
        desc.x = x;
        desc.y = y;
        UI_CHIP_INIT_DESC(&desc.chip_desc, "EF9345\nVDP", 30, _ui_ef9345_vdp_pins);
        ui_ef9345_init(&ui->vdp, &desc);
    }

    // TODO: add audio debug
    // x += dx; y += dy;
    // {
    //     ui_audio_desc_t desc = {0};
    //     desc.title = "Audio Output";
    //     desc.sample_buffer = ui->vg5000->audio.sample_buffer;
    //     desc.num_samples = ui->vg5000->audio.num_samples;
    //     desc.x = x;
    //     desc.y = y;
    //     ui_audio_init(&ui->audio, &desc);
    // }
    x += dx; y += dy;
    {
        ui_kbd_desc_t desc = {0};
        desc.title = "Keyboard Matrix";
        desc.kbd = &ui->vg5000->kbd;
        desc.layers[0] = "None";
        desc.layers[1] = "Shift";
        desc.layers[2] = "Ctrl";
        desc.x = x;
        desc.y = y;
        ui_kbd_init(&ui->kbd, &desc);
    }
    x += dx; y += dy;
    {
        ui_memedit_desc_t desc = {0};
        desc.layers[0] = "System";
        desc.read_cb = _ui_vg5000_mem_read;
        desc.write_cb = _ui_vg5000_mem_write;
        desc.user_data = ui->vg5000;
        static const char* titles[] = { "Memory Editor #1", "Memory Editor #2", "Memory Editor #3", "Memory Editor #4" };
        for (int i = 0; i < 4; i++) {
            desc.title = titles[i]; desc.x = x; desc.y = y;
            ui_memedit_init(&ui->memedit[i], &desc);
            x += dx; y += dy;
        }
    }
    x += dx; y += dy;
    {
        ui_memmap_desc_t desc = {0};
        desc.title = "Memory Map";
        desc.x = x;
        desc.y = y;
        ui_memmap_init(&ui->memmap, &desc);
        /* the memory map is static. Can be made dynamic (reading the BASIC pointers) */
        ui_memmap_layer(&ui->memmap, "System");
    }
    x += dx; y += dy;
    {
        ui_dasm_desc_t desc = {0};
        desc.layers[0] = "System";
        desc.cpu_type = UI_DASM_CPUTYPE_Z80;
        desc.start_addr = mem_rd16(&ui->vg5000->mem, 0xFFFC);
        desc.read_cb = _ui_vg5000_mem_read;
        desc.user_data = ui->vg5000;
        static const char* titles[4] = { "Disassembler #1", "Disassembler #2", "Disassembler #2", "Dissassembler #3" };
        for (int i = 0; i < 4; i++) {
            desc.title = titles[i]; desc.x = x; desc.y = y;
            ui_dasm_init(&ui->dasm[i], &desc);
            x += dx; y += dy;
        }
    }
}

void ui_vg5000_discard(ui_vg5000_t* ui) {
    CHIPS_ASSERT(ui && ui->vg5000);
    ui->vg5000 = 0;
    ui_z80_discard(&ui->cpu);
    ui_ef9345_discard(&ui->vdp);
    // TODO: support audio
    // ui_audio_discard(&ui->audio);
    ui_kbd_discard(&ui->kbd);
    ui_memmap_discard(&ui->memmap);
    for (int i = 0; i < 4; i++) {
        ui_memedit_discard(&ui->memedit[i]);
        ui_dasm_discard(&ui->dasm[i]);
    }
    ui_dbg_discard(&ui->dbg);

}

void ui_vg5000_draw(ui_vg5000_t* ui) {
    CHIPS_ASSERT(ui && ui->vg5000);
    _ui_vg5000_draw_menu(ui);
    if (ui->memmap.open) {
        _ui_vg5000_update_memmap(ui);
    }
    // TODO: audio support
    // ui_audio_draw(&ui->audio, ui->vg5000->audio.sample_pos);
    ui_z80_draw(&ui->cpu);
    ui_ef9345_draw(&ui->vdp);
    ui_kbd_draw(&ui->kbd);
    ui_memmap_draw(&ui->memmap);
    for (int i = 0; i < 4; i++) {
        ui_memedit_draw(&ui->memedit[i]);
        ui_dasm_draw(&ui->dasm[i]);
    }
    ui_dbg_draw(&ui->dbg);
}

chips_debug_t ui_vg5000_get_debug(ui_vg5000_t* ui) {
    CHIPS_ASSERT(ui);
    chips_debug_t res = {};
    res.callback.func = (chips_debug_func_t)ui_dbg_tick;
    res.callback.user_data = &ui->dbg;
    res.stopped = &ui->dbg.dbg.stopped;
    return res;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif /* CHIPS_UI_IMPL */
