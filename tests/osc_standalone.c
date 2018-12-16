
#include <string.h>
#include "osc.h"

#define OSC_HI(v) ((v)>>8)
#define OSC_LO(v) ((v)&0xFF)
#define OSC_SAMPLE_HEADROOM_BITS (16-10)
#define OSC_GAIN_MUL (1<<(OSC_SAMPLE_HEADROOM_BITS-1))


static void osc_reset(void);

static uint8_t osc_int_count;
static bool osc_active = false;
struct osc_state osc_state_array[OSC_CH_COUNT];


void osc_setup(void)
{
	osc_reset();
}

static void osc_reset(void)
{
	osc_set_isr_active(0);
	osc_int_count = 1;
	memset(osc_state_array, 0, sizeof(osc_state_array));
}

void osc_set_isr_active(const uint8_t active_flag)
{
	osc_active = active_flag;
}

int16_t osc_next_sample(void)
{
	int16_t pcm = 0;
	if (!osc_active) {
		return pcm;
	}

	struct osc_state *s = osc_state_array;
	for (uint8_t i = 0; i < OSC_CH_COUNT ; i++,s++) {
		int8_t vol = (int8_t)s->params.vol;
		if (!vol) {
			/* skip if volume or phase increment is zero and save some cycles */
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

	if (!(--osc_int_count)) {
		osc_int_count = OSC_ISR_PRESCALER_DIV;
		osc_tick_handler();
	}
	return pcm*OSC_GAIN_MUL;
}
