
#include <stddef.h>
#include <string.h>
#include <avr/interrupt.h>
#include "osc.h"

/*
  Mixing 4 8bit channels requires 10 bits but by setting ENHC4 in TCCR4E
  bit 0 of OCR4A selects the timer clock edge so while the
  resolution is 10 bits the number of timer bits used to count
  (set in OCR4C) is 9.

  See section "15.6.2 Enhanced Compare/PWM mode" in the ATmega16U4/ATmega32U4
  datasheet. This means the PWM frequency can be double of what it would be
  if we used 10 timer bits for counting.

  Also, 8 8bit channels can be mixed (11bits resolution, 10bits
  timer counter) if the PWM rate is halved (or fast PWM mode is used
  instead of dual slope PWM).
*/

#define OSC_COMPARE_RESOLUTION_BITS (10)
#define OSC_DC_OFFSET (1<<(OSC_COMPARE_RESOLUTION_BITS-1))

#define OSC_TIMER_BITS (9)
#define OSC_PWM_TOP ((1<<OSC_TIMER_BITS)-1)
#define OSC_HI(v) ((v)>>8)
#define OSC_LO(v) ((v)&0xFF)

static void osc_reset(void);

static uint8_t osc_int_count;
uint8_t osc_gain;
struct osc_state osc_state_array[OSC_CH_COUNT];


void osc_setup(void)
{
	osc_reset();
	/* PWM setup using timer 4 */
	/* PINMUX:16MHz XTAL, PLLUSB:48MHz, PLLTM:1, PDIV:96MHz */
	PLLFRQ = 0b01011010;
	PLLCSR = 0b00010010;
	/* Wait for PLL lock */
	while (!(PLLCSR & 0x01)) {}
	/* PWM mode */
	TCCR4A = 0b01000010;
	/* TCCR4B will be se to 0b00000001 for clock source/1, 96MHz/(OCR4C+1)/2 ~ 95703Hz */
	/* Dual Slope PWM (the /2 in the eqn. above is because of dual slope PWM) */
	TCCR4D = 0b00000001;
	/* Enhanced mode (bit 0 in OCR4C selects clock edge) */
	TCCR4E = 0b01000000;
	TC4H   = OSC_HI(OSC_PWM_TOP);
	OCR4C  = OSC_LO(OSC_PWM_TOP);

	TCCR3A = 0b00000000;
	/* Mode CTC, clock source 16MHz */
	TCCR3B = 0b00001001;
	/* 16MHz/1k = 16kHz */
	OCR3A  = (16E6/OSC_SAMPLERATE)-1;
}

static void osc_reset(void)
{
	osc_set_isr_active(0);
	osc_gain = 1;
	memset(osc_state_array, 0, sizeof(osc_state_array));
}

void osc_set_isr_active(const uint8_t active_flag)
{
	if (active_flag) {
		TC4H   = OSC_HI(OSC_DC_OFFSET);
		OCR4A  = OSC_LO(OSC_DC_OFFSET);
		TCCR4B = 0b00000001;    /* clock source/1, 96MHz/(OCR4C+1)/2 ~ 95703Hz */
		TIMSK3 = 0b00000010;    /* interrupts on */
	} else {
		TIMSK3 = 0b00000000;    /* interrupts off */
		TCCR4B = 0b00000000;    /* PWM = off */
	}
}

ISR(TIMER3_COMPA_vect)
{
	int16_t pcm = 0;
	for (uint8_t i = 0; i < OSC_CH_COUNT ; i++) {
		struct osc_state *const s = &osc_state_array[i];
		int8_t vol = (int8_t)s->params.vol;
		if (!vol) {
			/* skip if volume is zero and save some cycles */
			continue;
		}
		{
			const union osc_u16 phi = s->params.phase_inc;
			union osc_u16 pha = s->phase_acc;
			if (phi.u8.hi & 0x80) {
				pha.u16 += pha.u16;
				if (pha.u8.hi & 0x80) {
					pha.u8.lo ^= 0x2D;
				} else {
					vol = -vol;
				}
			} else {
				pha.u16 += phi.u16;
				vol = ((int8_t)pha.u8.hi > s->params.mod) ? vol : -vol;
			}
			s->phase_acc = pha;
			pcm += vol;
		}
	}

	pcm *= osc_gain;
	pcm += OSC_DC_OFFSET;

	TC4H = OSC_HI(pcm);
	OCR4A = OSC_LO(pcm);

	if (!(--osc_int_count)) {
		osc_int_count = OSC_ISR_PRESCALER_DIV;
		osc_tick_handler();
	}
}
