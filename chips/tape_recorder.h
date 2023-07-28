#pragma once
/*#
    # cass_vg5000.h

    Cassette recorder for VG5000µ.

    Do this:
    ~~~C
    #define CHIPS_IMPL
    ~~~
    before you include this file in *one* C or C++ file to create the 
    implementation.

    Optionally provide the following macros with your own implementation
    
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
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VG5000_MAX_TAPE_DATA_SIZE (32*1024)

typedef struct {
    size_t size;
    uint8_t data[VG5000_MAX_TAPE_DATA_SIZE];
} tape_t;

typedef struct {
    uint8_t format;
    char name[6+1];
    uint8_t version;
    char start_line[5+1];
    uint8_t protection;
    uint16_t check_pos;
    uint16_t start_adr;
    uint16_t data_length;
    uint16_t checksum;
} tape_information_t;

// a tape recorder description
typedef struct {
    size_t tape_index;
    tape_t tape;
    tape_information_t tape_info;
    bool motor_on;
} tape_recorder_t;

// initialize a cassette recorder
void tape_recorder_init(tape_recorder_t* recorder);
// insert a tape, the tape structure and data will be copied
bool tape_recorder_insert_tape(tape_recorder_t* recorder, chips_range_t k7_file_data);
// eject current tape (there will always be a default tape present)
void tape_recorder_eject_tape(tape_recorder_t* recorder);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*--- IMPLEMENTATION ---------------------------------------------------------*/
#ifdef CHIPS_IMPL
#include <string.h>
#include <stdio.h> // TODO: remove when not using printf anymore
#ifndef CHIPS_ASSERT
    #include <assert.h>
    #define CHIPS_ASSERT(c) assert(c)
#endif

void tape_recorder_init(tape_recorder_t* recorder) {
    memset(recorder, 0, sizeof(*recorder));
}

bool _read_tape_information(tape_recorder_t* recorder) {
    const size_t tape_data_size = recorder->tape.size;
    if (tape_data_size < 32) { // The must be at least a header
        printf("Tape: too short\n");
        return false; // TODO: explain the reason of the failure
    }

    const uint8_t* data = recorder->tape.data;
    const uint8_t* header = data;

    for (int i = 0; i < 10; i++) {
        if (header[i] != 0xd3) {
            printf("Tape: wrong 0xd3 part\n");
            return false; // TODO: explain the reason of the failure
        }
    }

    tape_information_t* tape_info = &recorder->tape_info;
    tape_info->format = header[10];

    memcpy(tape_info->name, &header[11], 6);
    tape_info->name[6] = '\0';

    tape_info->version = header[17];

    memcpy(tape_info->start_line, &header[18], 5);
    tape_info->start_line[5] = '\0';

    tape_info->protection = header[23];
    tape_info->check_pos = (header[25] << 8) | header[24];
    tape_info->start_adr = (header[27] << 8) | header[26];
    tape_info->data_length = (header[29] << 8) | header[28];
    tape_info->checksum = (header[31] << 8) | header[30];

    // Write the content of the header to stdout
    printf("Format: %d\n", tape_info->format);
    printf("Name: %s\n", tape_info->name);
    printf("Version: %d\n", tape_info->version);
    printf("Start Line: %s\n", tape_info->start_line);
    printf("Protection: %d\n", tape_info->protection);
    printf("Check Pos.: %04x\n", tape_info->check_pos);
    printf("Start Adr.: %04x\n", tape_info->start_adr);
    printf("Data Length: %d\n", tape_info->data_length);
    printf("Checksum: %04x\n", tape_info->checksum);

    // Check that the following data is 10 times 0xd6
    for (int i = 0; i < 10; i++) {
        if (data[32 + i] != 0xd6) {
            printf("Tape: wrong 0xd6 part\n");
            return false; // TODO: explain the reason of the failure
        }
    }

    if (tape_data_size < 32 + tape_info->data_length + 10) {
        printf("Tape: wrong total data length\n");
        return false; // TODO: explain the reason of the failure
    }

    return true;
}

bool tape_recorder_insert_tape(tape_recorder_t* recorder, chips_range_t k7_file_data) {
    memcpy(recorder->tape.data, k7_file_data.ptr, k7_file_data.size);
    recorder->tape.size = k7_file_data.size;

    recorder->tape_index = 0;
    recorder->motor_on = false;

    bool success = _read_tape_information(recorder);

    return success;
}

void tape_recorder_eject_tape(tape_recorder_t* recorder) {
    recorder->tape.size = 0;
    memset(recorder->tape.data, 0, sizeof(recorder->tape.data));

    recorder->tape_index = 0;
    recorder->motor_on = false;
}

#endif /* CHIPS_IMPL */
