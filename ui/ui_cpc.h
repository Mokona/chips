#pragma once
/*#
    # ui_cpc.h

    Integrated debugging UI for cpc.h

    Do this:
    ~~~C
    #define CHIPS_IMPL
    ~~~
    before you include this file in *one* C++ file to create the 
    implementation.

    Optionally provide the following macros with your own implementation
    
    ~~~C
    CHIPS_ASSERT(c)
    ~~~
        your own assert macro (default: assert(c))

    Include the following headers (and their depenencies) before including
    ui_cpc.h both for the declaration and implementation.

    - cpc.h
    - mem.h
    - ui_chip.h
    - ui_util.h
    - ui_z80.h
    - ui_i8255.h
    - ui_mc6845.h
    - ui_upd765.h
    - ui_ay38910.h
    - ui_audio.h
    - ui_dasm.h
    - ui_memedit.h
    - ui_memmap.h

    ## zlib/libpng license

    Copyright (c) 2018 Andre Weissflog
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
/* general callback type for rebooting to different configs */
typedef void (*ui_cpc_boot_t)(cpc_t* sys, cpc_type_t type);

typedef struct {
    cpc_t* cpc;
    ui_cpc_boot_t boot_cb; /* user-provided callback to reboot to different config */
} ui_cpc_desc_t;

typedef struct {
    cpc_t* cpc;
    ui_cpc_boot_t boot_cb;
    ui_z80_t cpu;
    ui_ay38910_t psg;
    ui_audio_t audio;
    ui_memmap_t memmap;
    ui_memedit_t memedit[4];
    ui_dasm_t dasm[4];
} ui_cpc_t;

void ui_cpc_init(ui_cpc_t* ui, const ui_cpc_desc_t* desc);
void ui_cpc_discard(ui_cpc_t* ui);
void ui_cpc_draw(ui_cpc_t* ui, double time_ms);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*-- IMPLEMENTATION (include in C++ source) ----------------------------------*/
#ifdef CHIPS_IMPL
#ifndef __cplusplus
#error "implementation must be compiled as C++"
#endif
#include <string.h> /* memset */
#ifndef CHIPS_ASSERT
    #include <assert.h>
    #define CHIPS_ASSERT(c) assert(c)
#endif
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"

static void _ui_cpc_draw_menu(ui_cpc_t* ui, double time_ms) {
    CHIPS_ASSERT(ui && ui->cpc && ui->boot_cb);
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("System")) {
            if (ImGui::MenuItem("Reset")) {
                cpc_reset(ui->cpc);
            }
            if (ImGui::MenuItem("CPC 464", 0, ui->cpc->type == CPC_TYPE_464)) {
                ui->boot_cb(ui->cpc, CPC_TYPE_464);
            }
            if (ImGui::MenuItem("CPC 6128", 0, ui->cpc->type == CPC_TYPE_6128)) {
                ui->boot_cb(ui->cpc, CPC_TYPE_6128);
            }
            if (ImGui::MenuItem("KC Compact", 0, ui->cpc->type == CPC_TYPE_KCCOMPACT)) {
                ui->boot_cb(ui->cpc, CPC_TYPE_KCCOMPACT);
            }
            if (ImGui::MenuItem("Joystick", 0, ui->cpc->joystick_type != CPC_JOYSTICK_NONE)) {
                ui->cpc->joystick_type = CPC_JOYSTICK_DIGITAL;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Hardware")) {
            ImGui::MenuItem("Memory Map", 0, &ui->memmap.open);
            ImGui::MenuItem("Audio Output", 0, &ui->audio.open);
            ImGui::MenuItem("System State (TODO)");
            ImGui::MenuItem("Z80 CPU", 0, &ui->cpu.open);
            ImGui::MenuItem("AY-3-8912", 0, &ui->psg.open);
            ImGui::MenuItem("i8255 (TODO)");
            ImGui::MenuItem("MC6845 (TODO)");
            ImGui::MenuItem("uPD765 (TODO)");
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Debug")) {
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
            ImGui::MenuItem("CPU Debugger (TODO)");
            ImGui::EndMenu();
        }
        ImGui::SameLine(ImGui::GetWindowWidth() - 120);
        ImGui::Text("emu: %.2fms", time_ms);
        ImGui::EndMainMenuBar();
    }
}

static int _ui_cpc_ram_config[8][4] = {
    { 0, 1, 2, 3 },
    { 0, 1, 2, 7 },
    { 4, 5, 6, 7 },
    { 0, 3, 2, 7 },
    { 0, 4, 2, 3 },
    { 0, 5, 2, 3 },
    { 0, 6, 2, 3 },
    { 0, 7, 2, 3 }
};

static const char* _ui_cpc_ram_name[8] = {
    "RAM 0", "RAM 1", "RAM 2", "RAM 3", "RAM 4", "RAM 5", "RAM 6", "RAM 7"
};
static const char* _ui_cpc_ram_banks[8] = {
    "RAM Bank 0", "RAM Bank 1", "RAM Bank 2", "RAM Bank 3", "RAM Bank 4", "RAM Bank 5", "RAM Bank 6", "RAM Bank 7"
};

static void _ui_cpc_update_memmap(ui_cpc_t* ui) {
    CHIPS_ASSERT(ui && ui->cpc);
    const cpc_t* cpc = ui->cpc;
    ui_memmap_reset(&ui->memmap);
    if ((cpc->type == CPC_TYPE_464) || (cpc->type == CPC_TYPE_KCCOMPACT)) {
        ui_memmap_layer(&ui->memmap, "ROM");
            ui_memmap_region(&ui->memmap, "Lower ROM (OS)", 0x0000, 0x4000, !(cpc->ga.config & (1<<2)));
            ui_memmap_region(&ui->memmap, "Upper ROM (BASIC)", 0xC000, 0x4000, !(cpc->ga.config & (1<<3)));
        ui_memmap_layer(&ui->memmap, "RAM");
            ui_memmap_region(&ui->memmap, "RAM 0", 0x0000, 0x4000, true);
            ui_memmap_region(&ui->memmap, "RAM 1", 0x4000, 0x4000, true);
            ui_memmap_region(&ui->memmap, "RAM 2", 0x8000, 0x4000, true);
            ui_memmap_region(&ui->memmap, "RAM 3 (Screen)", 0xC000, 0x4000, true);
    }
    else {
        const int ram_config_index = cpc->ga.ram_config & 7;
        ui_memmap_layer(&ui->memmap, "ROM Layer 0");
            ui_memmap_region(&ui->memmap, "OS ROM", 0x0000, 0x4000, true);
            ui_memmap_region(&ui->memmap, "BASIC ROM", 0xC000, 0x4000, !(cpc->ga.config & (1<<3)) && (cpc->upper_rom_select != 7));
        ui_memmap_layer(&ui->memmap, "ROM Layer 1");
            ui_memmap_region(&ui->memmap, "AMSDOS ROM", 0xC000, 0x4000, !(cpc->ga.config & (1<<3)) && (cpc->upper_rom_select == 7));
        for (int bank = 0; bank < 8; bank++) {
            ui_memmap_layer(&ui->memmap, _ui_cpc_ram_banks[bank]);
            bool bank_active = false;
            for (int slot = 0; slot < 4; slot++) {
                if (bank == _ui_cpc_ram_config[ram_config_index][slot]) {
                    ui_memmap_region(&ui->memmap, _ui_cpc_ram_name[bank], 0x4000*slot, 0x4000, true);
                    bank_active = true;
                }
            }
            if (!bank_active) {
                ui_memmap_region(&ui->memmap, _ui_cpc_ram_name[bank], 0x0000, 0x4000, false);
            }
        }
    }
}

static uint8_t _ui_cpc_mem_read(int layer, uint16_t addr, void* user_data) {
    CHIPS_ASSERT(user_data);
    cpc_t* cpc = (cpc_t*) user_data;
    if (layer == 0) {
        /* CPU mapped RAM layer */
        return mem_rd(&cpc->mem, addr);
    }
    else {
        // FIXME
        return 0xFF;
    }
}

static void _ui_cpc_mem_write(int layer, uint16_t addr, uint8_t data, void* user_data) {
    CHIPS_ASSERT(user_data);
    cpc_t* cpc = (cpc_t*) user_data;
    if (layer == 0) {
        mem_wr(&cpc->mem, addr, data);
    }
}

static const ui_chip_pin_t _ui_cpc_cpu_pins[] = {
    { "D0",     0,      Z80_D0 },
    { "D1",     1,      Z80_D1 },
    { "D2",     2,      Z80_D2 },
    { "D3",     3,      Z80_D3 },
    { "D4",     4,      Z80_D4 },
    { "D5",     5,      Z80_D5 },
    { "D6",     6,      Z80_D6 },
    { "D7",     7,      Z80_D7 },
    { "M1",     9,      Z80_M1 },
    { "MREQ",   10,     Z80_MREQ },
    { "IORQ",   11,     Z80_IORQ },
    { "RD",     12,     Z80_RD },
    { "WR",     13,     Z80_WR },
    { "HALT",   14,     Z80_HALT },
    { "INT",    15,     Z80_INT },
    { "NMI",    16,     Z80_NMI },
    { "WAIT",   17,     Z80_WAIT_MASK },
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
    { 0, 0, 0 }
};

static const ui_chip_pin_t _ui_cpc_psg_pins[] = {
    { "DA0",  0, AY38910_DA0 },
    { "DA1",  1, AY38910_DA1 },
    { "DA2",  2, AY38910_DA2 },
    { "DA3",  3, AY38910_DA3 },
    { "DA4",  4, AY38910_DA4 },
    { "DA5",  5, AY38910_DA5 },
    { "DA6",  6, AY38910_DA6 },
    { "DA7",  7, AY38910_DA7 },
    { "BDIR", 9, AY38910_BDIR },
    { "BC1",  10, AY38910_BC1 },
    { "IOA0", 11, AY38910_IOA0 },
    { "IOA1", 12, AY38910_IOA1 },
    { "IOA2", 13, AY38910_IOA2 },
    { "IOA3", 14, AY38910_IOA3 },
    { "IOA4", 15, AY38910_IOA4 },
    { "IOA5", 16, AY38910_IOA5 },
    { "IOA6", 17, AY38910_IOA6 },
    { "IOA7", 18, AY38910_IOA7 },
    { 0, 0, 0 }
};

void ui_cpc_init(ui_cpc_t* ui, const ui_cpc_desc_t* desc) {
    CHIPS_ASSERT(ui && desc);
    CHIPS_ASSERT(desc->cpc);
    CHIPS_ASSERT(desc->boot_cb);
    ui->cpc = desc->cpc;
    ui->boot_cb = desc->boot_cb;
    int x = 20, y = 20, dx = 10, dy = 10;
    {
        ui_z80_desc_t desc = {0};
        desc.title = "Z80 CPU";
        desc.cpu = &ui->cpc->cpu;
        desc.x = x;
        desc.y = y;
        ui_chip_init_chip_desc(&desc.chip_desc, "Z80\nCPU", 36, _ui_cpc_cpu_pins);
        ui_z80_init(&ui->cpu, &desc);
    }
    {
        ui_ay38910_desc_t desc = {0};
        desc.title = "AY-3-8912";
        desc.ay = &ui->cpc->psg;
        desc.x = x;
        desc.y = y;
        ui_chip_init_chip_desc(&desc.chip_desc, "8912", 22, _ui_cpc_psg_pins);
        ui_ay38910_init(&ui->psg, &desc);
    }
    x += dx; y += dy;
    {
        ui_audio_desc_t desc = {0};
        desc.title = "Audio Output";
        desc.sample_buffer = ui->cpc->sample_buffer;
        desc.num_samples = ui->cpc->num_samples;
        desc.x = x;
        desc.y = y;
        ui_audio_init(&ui->audio, &desc);
    }
    x += dx; y += dy;
    {
        ui_memedit_desc_t desc = {0};
        desc.layers[0] = "CPU Mapped";
        desc.layers[1] = "System ROMs";
        desc.layers[2] = "AMSDOS ROM";
        desc.layers[3] = "RAM Bank 0";
        desc.layers[4] = "RAM Bank 1";
        desc.layers[5] = "RAM Bank 2";
        desc.layers[6] = "RAM Bank 3";
        desc.layers[7] = "RAM Bank 4";
        desc.layers[8] = "RAM Bank 5";
        desc.layers[9] = "RAM Bank 6";
        desc.layers[10] = "RAM Bank 7";
        desc.read_cb = _ui_cpc_mem_read;
        desc.write_cb = _ui_cpc_mem_write;
        desc.user_data = ui->cpc;
        desc.h = 120;
        static const char* titles[] = { "Memory Editor #1", "Memory Editor #2", "Memory Editor #3", "Memory Editor #4" };
        for (int i = 0; i < 4; i++) {
            desc.title = titles[i]; desc.x = x; desc.y = y;
            ui_memedit_init(&ui->memedit[i], &desc);
            x += dx; y += dy;
        }
    }
    {
        ui_memmap_desc_t desc = {0};
        desc.title = "Memory Map";
        desc.x = x;
        desc.y = y;
        desc.w = 400;
        desc.h = 64;
        ui_memmap_init(&ui->memmap, &desc);
    }
    x += dx; y += dy;
    {
        ui_dasm_desc_t desc = {0};
        desc.layers[0] = "CPU Mapped";
        desc.layers[1] = "System ROMs";
        desc.layers[2] = "AMSDOS ROM";
        desc.layers[3] = "RAM Bank 0";
        desc.layers[4] = "RAM Bank 1";
        desc.layers[5] = "RAM Bank 2";
        desc.layers[6] = "RAM Bank 3";
        desc.layers[7] = "RAM Bank 4";
        desc.layers[8] = "RAM Bank 5";
        desc.layers[9] = "RAM Bank 6";
        desc.layers[10] = "RAM Bank 7";
        desc.start_addr = 0x0000;
        desc.read_cb = _ui_cpc_mem_read;
        desc.user_data = ui->cpc;
        desc.w = 400;
        desc.h = 256;
        static const char* titles[4] = { "Disassembler #1", "Disassembler #2", "Disassembler #2", "Dissassembler #3" };
        for (int i = 0; i < 4; i++) {
            desc.title = titles[i]; desc.x = x; desc.y = y;
            ui_dasm_init(&ui->dasm[i], &desc);
            x += dx; y += dy;
        }
    }
}

void ui_cpc_discard(ui_cpc_t* ui) {
    CHIPS_ASSERT(ui && ui->cpc);
    ui->cpc = 0;
    ui_z80_discard(&ui->cpu);
    ui_ay38910_discard(&ui->psg);
    ui_audio_discard(&ui->audio);
    ui_memmap_discard(&ui->memmap);
    for (int i = 0; i < 4; i++) {
        ui_memedit_discard(&ui->memedit[i]);
        ui_dasm_discard(&ui->dasm[i]);
    }
}

void ui_cpc_draw(ui_cpc_t* ui, double time_ms) {
    CHIPS_ASSERT(ui && ui->cpc);
    _ui_cpc_draw_menu(ui, time_ms);
    if (ui->memmap.open) {
        _ui_cpc_update_memmap(ui);
    }
    ui_audio_draw(&ui->audio, ui->cpc->sample_pos);
    ui_z80_draw(&ui->cpu);
    ui_ay38910_draw(&ui->psg);
    ui_memmap_draw(&ui->memmap);
    for (int i = 0; i < 4; i++) {
        ui_memedit_draw(&ui->memedit[i]);
        ui_dasm_draw(&ui->dasm[i]);
    }
}
#pragma clang diagnostic pop
#endif /* CHIPS_IMPL */
