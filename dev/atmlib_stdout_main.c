
#include <stdio.h>

#include "osc.h"
#include "atm_log.h"

void atmlib_demo_setup(void);
void atmlib_demo_looponce(void);

#define timestamp_increment (1000000000ULL/OSC_SAMPLERATE)

int main()
{
	//atm_log_mask = 0;
	osc_setup();
	atmlib_demo_setup();

	while (true) {
		atmlib_demo_looponce();
		int16_t pcm_sample = osc_next_sample();
		fwrite(&pcm_sample, 2, 1, stdout);
		atm_log_event("osc.pcm", "%hd f", pcm_sample);
		atm_log_timestamp += timestamp_increment;
	}
}
