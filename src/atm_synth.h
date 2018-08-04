
#ifndef _ATM_LIB_H
#define _ATM_LIB_H

#include <stddef.h>
#include <stdint.h>

#include "osc.h"
#include "atm_config.h"
#include "atm_cmd_constants.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))
#endif /* ARRAY_SIZE */

ct_assert(ATM_SCORE_CHANNEL_COUNT <= OSC_CH_COUNT, channel_count);

#include "atm_synth_internal.h"

#ifdef __AVR__
ct_assert(sizeof(struct atm_pool_slot) <= 8, pool_slot_size);
#endif /* __AVR__ */

struct atm_player_state {
	const uint8_t *score_start;
	uint8_t active_voices_mask;
	uint8_t used_voices_mask;
	uint8_t used_osc_ch_mask;
	uint8_t tick_rate;
	uint8_t tick_counter;
	atm_pool_slot_idx_t voices_head;
} __attribute__((packed));

struct atm_score_hdr {
	uint8_t voice_count;
	uint8_t pattern_count;
} __attribute__((packed));

struct atm_entry_patterns {
	uint8_t voice_count;
	struct {
		uint8_t pattern_idx;
		uint8_t osc_idx;
	} voices[ATM_SCORE_CHANNEL_COUNT];
} __attribute__((packed));

#define ATM_PLAYERS(player_count) \
	const uint8_t atm_player_count = (player_count); \
	struct atm_player_state atm_players[(player_count)];

extern struct atm_player_state atm_players[];
extern const uint8_t atm_player_count;

void atm_setup(void);

void atm_score_header(const uint8_t *score, struct atm_score_hdr *hdr);
int8_t atm_synth_player_setup(const uint8_t player_id, const void *score, const struct atm_entry_patterns *const ep);
void atm_synth_player_shutdown(const uint8_t player_id);

bool atm_synth_player_get_pause(const uint8_t player_id);
void atm_synth_player_set_pause(const uint8_t player_id, const bool pause);

/* hooks */
uint8_t atm_tick_handler(uint8_t used_osc_ch_mask);
uint8_t atm_player_loop(struct atm_player_state *const p);
void atm_next_command(const uint8_t *const cmd_ptr, struct atm_cmd_data *const dst);

#endif /* _ATM_LIB_H */
