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

#define SERVICE_BUS_PIN_RKY 0
#define SERVICE_BUS_PIN_RK7 1
#define SERVICE_BUS_PIN_WK7 2
#define SERVICE_BUS_MASK_RKY (1ULL << SERVICE_BUS_PIN_RKY)
#define SERVICE_BUS_MASK_RK7 (1ULL << SERVICE_BUS_PIN_RK7)
#define SERVICE_BUS_MASK_WK7 (1ULL << SERVICE_BUS_PIN_WK7)

#define VG5000_MAX_TAPE_DATA_SIZE (32*1024)
#define VG5000_MAX_CODEC_SIZE (12)


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

typedef enum {
    TAPE_INITIAL_SYNCHRO,
    TAPE_HEADER_DATA,
    TAPE_SECOND_SYNCHRO,
    TAPE_PAYLOAD_DATA,
    TAPE_FINISHED,
    TAPE_ERROR
} tape_state_t;

typedef struct {
    size_t pos;
    tape_state_t state;
    uint8_t current_byte;
    uint8_t bit_count;
    uint8_t valid_byte;
    uint16_t ticks_buf[VG5000_MAX_CODEC_SIZE];
} tape_codec_t;

// a tape recorder description
typedef struct {
    size_t tape_index;
    tape_t tape;
    tape_information_t tape_info;
    tape_codec_t tape_codec;
    float soundin;

    uint16_t tick_counter;   // count of ticks since the latest value change
    uint8_t data_value;
    uint8_t previous_data_value;

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
    tape_recorder_eject_tape(recorder);
}

bool _is_long_tick(uint16_t tick) {
    return (tick > 1500) && (tick < 2000); // TODO: real calibration to allow various bauds speeds
}

bool _is_short_tick(uint16_t tick) {
    return (tick > 600) && (tick < 1000); // TODO: real calibration to allow various bauds speeds
}

uint8_t _wait_for_synchro(tape_codec_t* tape_codec, bool* synchro_found) {
    uint8_t samples_consumed = 0;
    *synchro_found = false;

    const uint16_t* ticks_buf = tape_codec->ticks_buf;
    if (tape_codec->pos == 2 && _is_long_tick(ticks_buf[0]) && _is_long_tick(ticks_buf[1])) {
        *synchro_found = true;
        tape_codec->valid_byte = tape_codec->current_byte;
        tape_codec->current_byte = 0;
        tape_codec->bit_count = 0;
        samples_consumed = 2;
    }
    else if (tape_codec->pos == 1 && !_is_long_tick(ticks_buf[0])) {
        samples_consumed = 1;
    }
    else if (tape_codec->pos == 1 && _is_long_tick(ticks_buf[0])) {
        // Wait for next long tick
    }
    else {
        samples_consumed = 1;
        tape_codec->state = TAPE_ERROR;
    }

    return samples_consumed;
}

uint64_t tape_recorder_tick(tape_recorder_t* recorder, uint64_t service_bus, uint64_t cpu_pins) {
    const bool write_k7 = (service_bus & SERVICE_BUS_MASK_WK7) == 0;
    const bool read_k7 = (service_bus & SERVICE_BUS_MASK_RK7) == 0;

    if (write_k7) {
        recorder->soundin = (Z80_GET_DATA(cpu_pins) & 0b1000) ? 0.5f : 0.f; // TODO: or move to audio?
        recorder->data_value = (Z80_GET_DATA(cpu_pins) & 0b0001);
    }
    if (write_k7 || read_k7) {
        recorder->motor_on = (Z80_GET_DATA(cpu_pins) & 0b0010);
    }

    if (recorder->motor_on && recorder->tape.size > 0) {
        recorder->tick_counter++;

        // Tape Writing
        if (write_k7) {
            if (recorder->data_value != recorder->previous_data_value) {
                recorder->previous_data_value = recorder->data_value;

                // Record the tick counts
                tape_codec_t* tape_codec = &recorder->tape_codec;
                if (tape_codec->pos < VG5000_MAX_CODEC_SIZE) {
                    tape_codec->ticks_buf[tape_codec->pos] = recorder->tick_counter;
                    tape_codec->pos++;
                }
                recorder->tick_counter = 0;

                const uint16_t* ticks_buf = tape_codec->ticks_buf;
                uint8_t samples_consumed = 0;

                switch (tape_codec->state) {
                    case TAPE_INITIAL_SYNCHRO: {
                        bool synchro_found = false;
                        samples_consumed = _wait_for_synchro(tape_codec, &synchro_found);
                        if (synchro_found) {
                            tape_codec->state = TAPE_HEADER_DATA;
                        }
                        break;
                    }
                    case TAPE_HEADER_DATA: {
                            if (tape_codec->bit_count < 8) {
                                if (tape_codec->pos == 2 &&
                                        _is_long_tick(ticks_buf[0]) &&
                                        _is_long_tick(ticks_buf[1])) {
                                    tape_codec->current_byte >>= 1;
                                    tape_codec->bit_count++;
                                    samples_consumed = 2;
                                }
                                else if (tape_codec->pos == 4 &&
                                        _is_short_tick(ticks_buf[0]) &&
                                        _is_short_tick(ticks_buf[1]) &&
                                        _is_short_tick(ticks_buf[2]) &&
                                        _is_short_tick(ticks_buf[3])) {
                                    tape_codec->current_byte >>= 1;
                                    tape_codec->current_byte |= 0x80;
                                    tape_codec->bit_count++;
                                    samples_consumed = 4;
                                }
                                else if (tape_codec->pos >= 4) {
                                    printf("Tape: wrong header data.\n");
                                    samples_consumed = 4;
                                    tape_codec->state = TAPE_ERROR;
                                    tape_codec->bit_count = 8; // Use for logging after error
                                }
                            }
                            else {
                                bool synchro_found = false;
                                samples_consumed = _wait_for_synchro(tape_codec, &synchro_found);

                                if (synchro_found) {
                                    printf("Byte found: %02x %08b\n", tape_codec->valid_byte, tape_codec->valid_byte);
                                }
                            }
                        }
                        break;
                    case TAPE_SECOND_SYNCHRO: {
                            tape_codec->state = TAPE_PAYLOAD_DATA;
                        }
                        break;
                    case TAPE_PAYLOAD_DATA: {
                            tape_codec->state = TAPE_INITIAL_SYNCHRO;
                        }
                        break;
                    case TAPE_ERROR: {
                        if (tape_codec->bit_count > 0) {
                            printf("Tape: %d (@ %i)\n", ticks_buf[0], tape_codec->pos);
                            samples_consumed = 1;
                            tape_codec->bit_count--;
                        }
                        else {
                            tape_codec->state = TAPE_FINISHED;
                        }
                    }
                    default:
                        break;
                }

                if (samples_consumed > 0) {
                    memmove(&ticks_buf[0], &ticks_buf[samples_consumed], (VG5000_MAX_CODEC_SIZE - samples_consumed) * sizeof(uint16_t));
                    tape_codec->pos -= samples_consumed;
                }

                recorder->soundin = (recorder->data_value)?0.5f:0.f;
            }
        }

        // Tape Reading
        if (read_k7) {
            // TODO: Decode from temp buffer
            // if (sys->tape.pos < sys->tape.size) {
            //     cpu_pins &= ~Z80_D7;
            //     cpu_pins |= sys->tape.data_value?Z80_D7:0;

            //     if (sys->tape.tick_counter >= sys->tape.ticks_buf[sys->tape.pos]) {
            //         sys->tape.tick_counter -= sys->tape.ticks_buf[sys->tape.pos];
            //         sys->tape.pos++;
            //         sys->tape.data_value = !sys->tape.data_value;

            //         recorder->soundin = (recorder->data_value)?0.5f:0.f;
            //     }
            //}
        }
    }
    else {
        // if (!recorder->motor_on && recorder->tape_index > 0) {
        //     // Automatic rewind for tape
        //     // TODO: make this an option
        //     sys->tape.pos = 0;
        //     sys->tape.tick_counter = 0;
        //     sys->tape.data_value = 0;
        // }
    }

    return cpu_pins;
}

bool _read_tape_information(tape_recorder_t* recorder) {
    // TODO: allow multiple files on the same tape. At the moment, it read only one file

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

    if (tape_data_size < (size_t)(32 + tape_info->data_length + 10)) {
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
    recorder->tape.size = VG5000_MAX_TAPE_DATA_SIZE; // Insert a blank tape
    memset(recorder->tape.data, 0, sizeof(recorder->tape.data));

    recorder->tape_index = 0;
    recorder->motor_on = false;

    recorder->tape_codec.state = TAPE_INITIAL_SYNCHRO;
}

#endif /* CHIPS_IMPL */
