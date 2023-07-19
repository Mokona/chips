#pragma once

/*#
    # ui_ef9345.h

    Debug visualization UI of EF9345 VDP.

    Include the following headers before the including the *declaration*:
        - ef9345.h
        - ui_chip.h

    Include the following headers before including the *implementation*:
        - imgui.h
        - ef9345.h
        - ui_chip.h
        - ui_util.h
        // TODO: to verify

    All strings provided to ui_ef9345_init() must remain alive until
    ui_ef9345_discard() is called!

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

typedef struct {
    const char* title;          /* window title */
    ef9345_t* vdp;              /* pointer to VDP to track */
    int x, y;                   /* initial window pos */
    int w, h;                   /* initial window size, or 0 for default size */
    bool open;                  /* initial open state */
    ui_chip_desc_t chip_desc;   /* chip visualization desc */
} ui_ef9345_desc_t;

typedef struct {
    const char* title;
    ef9345_t* vdp;
    float init_x, init_y;
    float init_w, init_h;
    bool open;
    bool valid;
    ui_chip_t chip;
} ui_ef9345_t;

void ui_ef9345_init(ui_ef9345_t* win, const ui_ef9345_desc_t* desc);
void ui_ef9345_discard(ui_ef9345_t* win);
void ui_ef9345_draw(ui_ef9345_t* win);

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

void ui_ef9345_init(ui_ef9345_t* win, const ui_ef9345_desc_t* desc) {
    CHIPS_ASSERT(win && desc);
    CHIPS_ASSERT(desc->title);
    CHIPS_ASSERT(desc->vdp);
    memset(win, 0, sizeof(ui_ef9345_t));
    win->title = desc->title;
    win->vdp = desc->vdp;
    win->init_x = (float) desc->x;
    win->init_y = (float) desc->y;
    win->init_w = (float) ((desc->w == 0) ? 360 : desc->w);
    win->init_h = (float) ((desc->h == 0) ? 340 : desc->h);
    win->open = desc->open;
    win->valid = true;
    ui_chip_init(&win->chip, &desc->chip_desc);
}

void ui_ef9345_discard(ui_ef9345_t* win) {
    CHIPS_ASSERT(win && win->valid);
    win->valid = false;
}

static void _ui_ef9345_regs(ui_ef9345_t* win) {
    ef9345_t* vdp = win->vdp;
    ImGui::Text("R0: %02X  R1: %02X", vdp->direct_r0, vdp->direct_r1);
    ImGui::Text("R2: %02X  R3: %02X", vdp->direct_r2, vdp->direct_r3);
    ImGui::Text("R4: %02X  R5: %02X", vdp->direct_r4, vdp->direct_r5);
    ImGui::Text("R6: %02X  R7: %02X", vdp->direct_r6, vdp->direct_r7);
    ImGui::Separator();
    ImGui::Text("TGS: %02X", vdp->indirect_tgs);
    ImGui::Text("MAT: %02X  PAT:  %02X", vdp->indirect_mat, vdp->indirect_pat);
    ImGui::Text("DOR: %02X  ROR:  %02X", vdp->indirect_dor, vdp->indirect_ror);
    ImGui::Separator();
    ImGui::Text("Latched Addr: %04X", vdp->l_address);
    ImGui::Text("Latched DS  : %04X", vdp->l_ds);
    ImGui::Separator();
    ImGui::Text("TGS decoding");
    ImGui::Text(vdp->interlaced?"- Interlaced":"- Non interlaced");
    ImGui::Text("- # of lines: %3d", vdp->lines_per_frame);
    ImGui::Text(vdp->indirect_tgs&4?"- Composite Sync":"- H/V Sync Out");
}

void ui_ef9345_draw(ui_ef9345_t* win) {
    CHIPS_ASSERT(win && win->valid && win->title && win->vdp);
    if (!win->open) {
        return;
    }
    ImGui::SetNextWindowPos(ImVec2(win->init_x, win->init_y), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(win->init_w, win->init_h), ImGuiCond_Once);
    if (ImGui::Begin(win->title, &win->open)) {
        ImGui::BeginChild("##ef9345_chip", ImVec2(176, 0), true);
        ui_chip_draw(&win->chip, win->vdp->pins);
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("##ef9345_regs", ImVec2(0, 0), true);
        _ui_ef9345_regs(win);
        ImGui::EndChild();
    }
    ImGui::End();
}
#endif /* CHIPS_UI_IMPL */
