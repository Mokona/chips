#pragma once
/*#
    # tape_deck.h

    A generic cassette tape deck. The tape stores a square wave signal as a
    sequence of pulses, so any content can be recorded on it, independently
    of any file format.

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

    ~~~C
    TAPE_DECK_MAX_PULSES
    ~~~
        the capacity of a tape in pulses (default: 1M pulses, 4 MBytes)

    ## Emulated Pins

    ~~~
                  +-----------+
         MOTOR -->|           |
           REC -->| tape deck |--> READ
         WRITE -->|           |
                  +-----------+
    ~~~

    - MOTOR: motor running on not. 1 is motor on.
    - REC: record mode, the WRITE levels are recorded on tape.
    - WRITE: signal levels written on the tape.
    - READ: signal levels read from the tape.

    ## Howto

    Initialize a tape deck with the frequency of the system ticks:

    ~~~C
    tape_deck_init(&sys->tape_deck, &(tape_deck_desc_t){
        .tick_hz = VG5000_FREQUENCY,
        .auto_rewind = true,
    });
    ~~~

    Tick the deck and read the READ pin:

    ~~~C
    sys->tape_pins = tape_deck_tick(&sys->tape_deck, sys->tape_pins);
    const bool level = (sys->tape_pins & TAPE_DECK_READ) != 0;
    ~~~

    There is no reset: when the computer is reset, it clears its tape pins
    and the deck reacts on the next tick, as the hardware does.

    To write on tape:

    ~~~C
    tape_deck_add_pulse(&sys->tape_deck, true, 833);
    tape_deck_add_pulse(&sys->tape_deck, false, 833);
    ~~~

    Eject the tape. No more content to read, and cannot write on it.
    ~~~C
    tape_deck_eject(&sys->tape_deck);
    ~~~


    Recording overwrites the tape from the head position for the duration
    of the recording as on a real tape.

    ## zlib/libpng license

    Copyright (c) 2026 Sylvain Glaize
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

// deck pin
#define TAPE_DECK_PIN_MOTOR (0) // in: motor running
#define TAPE_DECK_PIN_REC   (1) // in: record mode
#define TAPE_DECK_PIN_WRITE (2) // in: write level
#define TAPE_DECK_PIN_READ  (3) // out: read level

// pin masks
#define TAPE_DECK_MOTOR (1ULL<<TAPE_DECK_PIN_MOTOR)
#define TAPE_DECK_REC   (1ULL<<TAPE_DECK_PIN_REC)
#define TAPE_DECK_WRITE (1ULL<<TAPE_DECK_PIN_WRITE)
#define TAPE_DECK_READ  (1ULL<<TAPE_DECK_PIN_READ)

// tape capacity in pulses. Change this for longer maximum size of tape
#ifndef TAPE_DECK_MAX_PULSES
#define TAPE_DECK_MAX_PULSES (1024*1024)
#endif

// a pulse is the signal in the upper bit and its duration (in ticks) in the other lower bits
#define TAPE_DECK_PULSE_HIGH (1U<<31)
#define TAPE_DECK_PULSE_MAX_TICKS (TAPE_DECK_PULSE_HIGH - 1)

// setup parameters for tape_deck_init()
typedef struct {
    int tick_hz;            // frequency of the system ticks
    bool auto_rewind;       // rewind the tape when the motor stops after the tape moved
} tape_deck_desc_t;

// tape deck state
typedef struct {
    bool valid;
    bool moved;             // the tape moved since the motor was switched on
    bool auto_rewind;
    int tick_hz;
    uint64_t pins;          // pin state of the last tick
    uint32_t num_pulses;    // number of pulses on tape
    uint32_t pos;           // index of the pulse under the head
    uint32_t pulse_ticks;   // ticks already played of the pulse under the head
    struct {
        bool active;
        bool level;         // level of the pulse being recorded
        bool overflow;      // the tape was full, part of the recording was lost
        uint32_t ticks;     // duration of the pulse being recorded
        uint32_t tail_pos;  // first pulse of the content after the head
        uint32_t tail_ticks;// ticks already overwritten at tail_pos
    } rec;
    uint32_t pulses[TAPE_DECK_MAX_PULSES];
} tape_deck_t;

// initialize a tape deck with a blank tape
void tape_deck_init(tape_deck_t* sys, const tape_deck_desc_t* desc);
// discard a tape deck
void tape_deck_discard(tape_deck_t* sys);
// tick the tape deck, return pins with the READ pin updated
uint64_t tape_deck_tick(tape_deck_t* sys, uint64_t pins);
// remove the content of the tape, which becomes blank
void tape_deck_eject(tape_deck_t* sys);
// append a pulse at the end of the tape, return false if the tape is full
bool tape_deck_add_pulse(tape_deck_t* sys, bool level, uint64_t ticks);
// move the head to the start of the tape
void tape_deck_rewind(tape_deck_t* sys);
// move the head to the start of a pulse
void tape_deck_seek(tape_deck_t* sys, uint32_t pos);
// return the index of the first pulse from pos lasting at least min_ticks, or num_pulses if none
uint32_t tape_deck_find_silence(const tape_deck_t* sys, uint32_t pos, uint32_t min_ticks);
// return true if the motor is running
bool tape_deck_is_motor_on(const tape_deck_t* sys);
// return true if the tape deck is recording
bool tape_deck_is_recording(const tape_deck_t* sys);
// return the level of a pulse
static inline bool tape_deck_pulse_level(const tape_deck_t* sys, uint32_t pos) {
    return (sys->pulses[pos] & TAPE_DECK_PULSE_HIGH) != 0;
}
// return the duration in ticks of a pulse
static inline uint32_t tape_deck_pulse_ticks(const tape_deck_t* sys, uint32_t pos) {
    return sys->pulses[pos] & TAPE_DECK_PULSE_MAX_TICKS;
}

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

static uint32_t _tape_deck_make_pulse(bool level, uint32_t ticks) {
    CHIPS_ASSERT(ticks <= TAPE_DECK_PULSE_MAX_TICKS);
    return (level ? TAPE_DECK_PULSE_HIGH : 0) | ticks;
}

void tape_deck_init(tape_deck_t* sys, const tape_deck_desc_t* desc) {
    CHIPS_ASSERT(sys && desc);
    CHIPS_ASSERT(desc->tick_hz > 0);
    memset(sys, 0, sizeof(*sys));
    sys->tick_hz = desc->tick_hz;
    sys->auto_rewind = desc->auto_rewind;
    sys->valid = true;
}

void tape_deck_discard(tape_deck_t* sys) {
    CHIPS_ASSERT(sys && sys->valid);
    sys->valid = false;
}

// move the content after the head to the end of the buffer, recording is done in the gap
static void _tape_deck_start_recording(tape_deck_t* sys, bool level) {
    const uint32_t tail_len = sys->num_pulses - sys->pos;
    sys->rec.tail_pos = TAPE_DECK_MAX_PULSES - tail_len;
    sys->rec.tail_ticks = 0;
    if ((sys->pulse_ticks > 0) && (sys->num_pulses < TAPE_DECK_MAX_PULSES)) {
        // the pulse under the head is split: the part already played stays
        // before the head, the rest starts the tail
        const bool pulse_level = tape_deck_pulse_level(sys, sys->pos);
        const uint32_t pulse_ticks = tape_deck_pulse_ticks(sys, sys->pos);
        memmove(&sys->pulses[sys->rec.tail_pos + 1], &sys->pulses[sys->pos + 1], (tail_len - 1) * sizeof(uint32_t));
        sys->pulses[sys->rec.tail_pos] = _tape_deck_make_pulse(pulse_level, pulse_ticks - sys->pulse_ticks);
        sys->pulses[sys->pos] = _tape_deck_make_pulse(pulse_level, sys->pulse_ticks);
        sys->pos += 1;
    }
    else {
        // no room to split a full tape: the played part of the pulse is lost
        memmove(&sys->pulses[sys->rec.tail_pos], &sys->pulses[sys->pos], tail_len * sizeof(uint32_t));
        if (sys->pulse_ticks > 0) {
            sys->rec.tail_ticks = sys->pulse_ticks;
            sys->rec.overflow = true;
        }
    }
    sys->num_pulses = sys->pos;
    sys->pulse_ticks = 0;
    sys->rec.active = true;
    sys->rec.level = level;
    sys->rec.ticks = 0;
}

// write the recorded pulse in the gap
static void _tape_deck_emit_pulse(tape_deck_t* sys) {
    if (sys->rec.ticks == 0) {
        return;
    }
    if (sys->num_pulses < sys->rec.tail_pos) {
        sys->pulses[sys->num_pulses] = _tape_deck_make_pulse(sys->rec.level, sys->rec.ticks);
        sys->num_pulses += 1;
    }
    else {
        sys->rec.overflow = true;
    }
    sys->rec.ticks = 0;
}

// move what remains of the content after the recording back after it
static void _tape_deck_stop_recording(tape_deck_t* sys) {
    _tape_deck_emit_pulse(sys);

    const uint32_t tail_pos = sys->rec.tail_pos;
    if ((tail_pos < TAPE_DECK_MAX_PULSES) && (sys->rec.tail_ticks > 0)) {
        // shorten the partly overwritten pulse
        const bool level = tape_deck_pulse_level(sys, tail_pos);
        const uint32_t ticks = tape_deck_pulse_ticks(sys, tail_pos);
        CHIPS_ASSERT(ticks > sys->rec.tail_ticks);
        sys->pulses[tail_pos] = _tape_deck_make_pulse(level, ticks - sys->rec.tail_ticks);
    }

    const uint32_t tail_len = TAPE_DECK_MAX_PULSES - tail_pos;
    memmove(&sys->pulses[sys->num_pulses], &sys->pulses[tail_pos], tail_len * sizeof(uint32_t));
    sys->pos = sys->num_pulses;
    sys->pulse_ticks = 0;
    sys->num_pulses += tail_len;
    sys->rec.active = false;
    sys->rec.tail_pos = 0;
    sys->rec.tail_ticks = 0;
}

static void _tape_deck_record_tick(tape_deck_t* sys, bool level) {
    if ((level != sys->rec.level) || (sys->rec.ticks == TAPE_DECK_PULSE_MAX_TICKS)) {
        _tape_deck_emit_pulse(sys);
        sys->rec.level = level;
    }
    sys->rec.ticks += 1;
    // the same duration of the content after the head is overwritten
    if (sys->rec.tail_pos < TAPE_DECK_MAX_PULSES) {
        sys->rec.tail_ticks += 1;
        if (sys->rec.tail_ticks >= tape_deck_pulse_ticks(sys, sys->rec.tail_pos)) {
            sys->rec.tail_pos += 1;
            sys->rec.tail_ticks = 0;
        }
    }
}

static bool _tape_deck_play_tick(tape_deck_t* sys) {
    if (sys->pos >= sys->num_pulses) {
        // blank tape after the end of the content
        return false;
    }
    const bool level = tape_deck_pulse_level(sys, sys->pos);
    sys->pulse_ticks += 1;
    if (sys->pulse_ticks >= tape_deck_pulse_ticks(sys, sys->pos)) {
        sys->pos += 1;
        sys->pulse_ticks = 0;
    }
    return level;
}

uint64_t tape_deck_tick(tape_deck_t* sys, uint64_t pins) {
    CHIPS_ASSERT(sys && sys->valid);
    const bool motor = (pins & TAPE_DECK_MOTOR) != 0;
    const bool rec = motor && ((pins & TAPE_DECK_REC) != 0);
    const bool write_level = (pins & TAPE_DECK_WRITE) != 0;

    if (sys->rec.active && !rec) {
        _tape_deck_stop_recording(sys);
    }
    if (!motor && (sys->pins & TAPE_DECK_MOTOR)) {
        // motor just stopped
        if (sys->auto_rewind && sys->moved) {
            tape_deck_rewind(sys);
        }
        sys->moved = false;
    }

    bool read_level = false;
    if (rec) {
        if (!sys->rec.active) {
            _tape_deck_start_recording(sys, write_level);
        }
        _tape_deck_record_tick(sys, write_level);
        sys->moved = true;
    }
    else if (motor) {
        if (sys->pos < sys->num_pulses) {
            sys->moved = true;
        }
        read_level = _tape_deck_play_tick(sys);
    }

    pins &= ~TAPE_DECK_READ;
    if (read_level) {
        pins |= TAPE_DECK_READ;
    }
    sys->pins = pins;
    return pins;
}

void tape_deck_eject(tape_deck_t* sys) {
    CHIPS_ASSERT(sys && sys->valid);
    CHIPS_ASSERT(!sys->rec.active);
    sys->num_pulses = 0;
    sys->pos = 0;
    sys->pulse_ticks = 0;
    sys->rec.overflow = false;
}

bool tape_deck_add_pulse(tape_deck_t* sys, bool level, uint64_t ticks) {
    CHIPS_ASSERT(sys && sys->valid);
    CHIPS_ASSERT(!sys->rec.active);

    // long pulses are split in several pulses of the same level
    while (ticks > 0) {
        if (sys->num_pulses >= TAPE_DECK_MAX_PULSES) {
            return false;
        }
        const uint32_t pulse_ticks = (ticks > TAPE_DECK_PULSE_MAX_TICKS) ? TAPE_DECK_PULSE_MAX_TICKS : (uint32_t)ticks;
        sys->pulses[sys->num_pulses] = _tape_deck_make_pulse(level, pulse_ticks);
        sys->num_pulses += 1;
        ticks -= pulse_ticks;
    }
    return true;
}

void tape_deck_rewind(tape_deck_t* sys) {
    tape_deck_seek(sys, 0);
}

void tape_deck_seek(tape_deck_t* sys, uint32_t pos) {
    CHIPS_ASSERT(sys && sys->valid);
    CHIPS_ASSERT(!sys->rec.active);
    sys->pos = (pos < sys->num_pulses) ? pos : sys->num_pulses;
    sys->pulse_ticks = 0;
}

uint32_t tape_deck_find_silence(const tape_deck_t* sys, uint32_t pos, uint32_t min_ticks) {
    CHIPS_ASSERT(sys && sys->valid);

    for (; pos < sys->num_pulses; pos += 1) {
        if (tape_deck_pulse_ticks(sys, pos) >= min_ticks) {
            return pos;
        }
    }
    return sys->num_pulses;
}

bool tape_deck_is_motor_on(const tape_deck_t* sys) {
    CHIPS_ASSERT(sys && sys->valid);
    return (sys->pins & TAPE_DECK_MOTOR) != 0;
}

bool tape_deck_is_recording(const tape_deck_t* sys) {
    CHIPS_ASSERT(sys && sys->valid);
    return sys->rec.active;
}

#endif /* CHIPS_IMPL */
