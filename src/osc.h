
#ifndef _OSC_H
#define _OSC_H

#include <stdbool.h>
#include <stdint.h>

#define ct_assert(condition, name) typedef char name##_failed[1]; typedef char name##_failed[(condition)?1:0]

#ifndef OSC_SAMPLERATE
#define OSC_SAMPLERATE (16000)
#endif

#define OSC_FREQ_TO_PHI(hz) (65536.0f/(OSC_SAMPLERATE/hz))

#ifndef OSC_CH_COUNT
#define OSC_CH_COUNT (4)
#endif

ct_assert(OSC_CH_COUNT <= 8 && OSC_CH_COUNT > 0, osc_channel_count);

#define OSC_MAX_VOLUME (127)
#define OSC_MOD_MIN (-127)
#define OSC_MOD_MAX (127)

/* osc tick duration is 1ms */
#define OSC_TICKRATE (1000)
#define OSC_HZ_TO_TICKS(hz) (OSC_TICKRATE/(hz))
#define OSC_ISR_PRESCALER_DIV (OSC_SAMPLERATE/OSC_TICKRATE)

/*
OSC period is 2**16 so Nyquist corresponds to 2**16/2. Note that's way too high
for square waves to sound OK.
*/
#define OSC_PHASE_INC_MAX (0x3FFF)

union osc_u16 {
	uint16_t u16;
	struct {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		uint8_t lo;
		uint8_t hi;
#else
		uint8_t hi;
		uint8_t lo;
#endif
	} u8;
}  __attribute__((packed));

struct osc_params {
	int8_t  mod; /* square wave duty cycle: -128-127, center 0 */
	uint8_t  vol; /* volume: 0-127 */
	union osc_u16 phase_inc; /*
						bits 0:13: phase accumulator increment per sample
						bit 14: reserved
						bit 15: waveform select 0 square, 1 noise
						*/
};

struct osc_state {
	struct osc_params params;
	union osc_u16 phase_acc;
};

enum osc_channels_e {
	OSC_CH_0 = 0,
	OSC_CH_1,
	OSC_CH_2,
	OSC_CH_3,
	OSC_CH_4,
	OSC_CH_5,
	OSC_CH_6,
	OSC_CH_7,
};

extern uint8_t osc_gain;
extern struct osc_state osc_state_array[OSC_CH_COUNT];

/* osc_tick_handler must be defined by the user of this library

It is called at OSC_TICKRATE Hz (1kHz by default) when there is at least one active
OSC channel.

*/
extern void osc_tick_handler(void);

void osc_setup(void);
void osc_set_isr_active(const uint8_t active_flag);
int16_t osc_next_sample(void);

#endif /* _OSC_H */
