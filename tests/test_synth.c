
#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>

#include <atm_log.h>
#include <atm_synth.h>

/* 1 tick = 1 ms (10^6 ns) */
#define timestamp_increment (1000000ULL)

ATM_PLAYERS(1);
ATM_MEM_POOL(250);

uint8_t atm_player_loop(struct atm_player_state *const p)
{
	return 0;
}

int main(int argc, const char *argv[])
{
	if (argc < 2) {
		return 1;
	}

	FILE *f = fopen(argv[1], "r");
	if (!f) {
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

	atm_setup();
	assert(!atm_synth_player_setup(0, score, NULL));
	atm_synth_player_set_pause(0, 0);

	while (true) {
		if (atm_synth_player_get_pause(0)) {
			exit(0);
		}
		osc_tick_handler();
		atm_log_timestamp += timestamp_increment;
	}
}
