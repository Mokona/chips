#pragma once
/*#
    # ui_taperecorder.h

    Tape Recorder visualization for the VG5000µ.

    TODO: make it a generic tape recorder visualization?

    Do this:
    ~~~C
    #define CHIPS_UI_IMPL
    ~~~
    before you include this file in *one* C++ file to create the 
    implementation.

    Optionally provide the following macros with your own implementation
    
    ~~~C
    CHIPS_ASSERT(c)
    ~~~
        your own assert macro (default: assert(c))

    You need to include the following headers before including the
    *implementation*:

        - imgui.h
        - ui_util.h

        TODO: check dependencies

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

/* setup params for ui_taperecorder_init() */
typedef struct {
    const char* title;  /* window title */
    int x, y;           /* initial window pos */
    int w, h;           /* initial window size, or 0 for default size */
    bool open;          /* initial open state */
} ui_taperecorder_desc_t;

/* ui_taperecorder window state */
typedef struct {
    const char* title;
    float init_x, init_y;
    float init_w, init_h;
    bool open;
    bool valid;
} ui_taperecorder_t;

void ui_taperecorder_init(ui_taperecorder_t* win, const ui_taperecorder_desc_t* desc);
void ui_taperecorder_discard(ui_taperecorder_t* win);
void ui_taperecorder_draw(ui_taperecorder_t* win);

void ui_taperecorder_reset(ui_taperecorder_t* win);

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

void ui_taperecorder_init(ui_taperecorder_t* win, const ui_taperecorder_desc_t* desc) {
    CHIPS_ASSERT(win && desc);
    memset(win, 0, sizeof(ui_taperecorder_t));
    win->title = desc->title;
    win->init_x = (float) desc->x;
    win->init_y = (float) desc->y;
    win->init_w = (float) desc->w;
    win->init_h = (float) desc->h;
    win->open = desc->open;
    win->valid = true;
}

void ui_taperecorder_discard(ui_taperecorder_t* win) {
    CHIPS_ASSERT(win && win->valid);
    win->valid = false;
}

void ui_taperecorder_draw(ui_taperecorder_t* win) {
    CHIPS_ASSERT(win && win->valid);
    if (win->open) {
        ImGui::SetNextWindowPos(ImVec2(win->init_x, win->init_y), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(win->init_w, win->init_h), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(win->title, &win->open)) {
            ImGui::Text("TODO: tape recorder visualization");
        }
        ImGui::End();
    }
}

void ui_taperecorder_reset(ui_taperecorder_t* win) {
    CHIPS_ASSERT(win && win->valid);
}

#endif /* CHIPS_UI_IMPL */
