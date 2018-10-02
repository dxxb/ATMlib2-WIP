
#include <stdio.h>
#include <unistd.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>

#include <atm_log.h>
#include <atm_synth.h>

#define timestamp_increment (1000000000ULL/OSC_SAMPLERATE)

ATM_PLAYERS(1);
ATM_MEM_POOL(250);

uint8_t atm_player_loop(struct atm_player_state *const p)
{
	return 0;
}

int main(int argc, char *argv[])
{
	int ch;
	bool emit_samples = false;

	while ((ch = getopt(argc, argv, "sh")) != -1) {
		switch (ch) {
			case 's':
				emit_samples = true;
				break;
			case 'h':
			default:
				return 4;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 1) {
		return 1;
	}

	FILE *f = fopen(argv[0], "r");
	if (!f) {
		fprintf(stdout, "Cannot open score file: %s\n", argv[0]);
		return 2;
	}

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);
	uint8_t *const score = malloc(size);
	if (!score) {
		return 3;
	}

	fread(score, size, 1, f);

	atm_log_timestamp = 0;

	osc_setup();
	atm_setup();
	osc_set_isr_active(true);
	assert(!atm_synth_player_setup(0, score, NULL));
	atm_synth_player_set_pause(0, 0);

	while (true) {
		if (atm_synth_player_get_pause(0)) {
			exit(0);
		}
		int16_t pcm_sample = osc_next_sample();
		if (emit_samples) {
			fwrite(&pcm_sample, 2, 1, stdout);
		}
		atm_log_event("osc.pcm", "%hd f", pcm_sample);
		atm_log_timestamp += timestamp_increment;
	}
}
