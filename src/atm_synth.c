
#include <string.h>
#ifdef __AVR__
#define exit(val)
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#else
#include <stdlib.h>
#include <dummy_pgmspace.h>
#define cli()
#define sei()
#endif

#include "atm_cmd_constants.h"
#include "atm_synth.h"
#include "atm_pool.h"
#include "atm_log.h"

/* Reduce progmem used by note table

	Instead of storing all 64 notes (64*2 bytes) store just the last 12
	with highest frequency (the ones with highest information content in
	terms of significant bits) and use a 'multiply by constant' techinique
	to find which frequency to use for a given note.
*/

const uint16_t noteTable[12] PROGMEM = {
	OSC_FREQ_TO_PHI(1975.53f), // B6
	OSC_FREQ_TO_PHI(2093.00f), // C7
	OSC_FREQ_TO_PHI(2217.46f), // C#7
	OSC_FREQ_TO_PHI(2349.32f), // D7

	OSC_FREQ_TO_PHI(2489.02f), // D#7
	OSC_FREQ_TO_PHI(2637.02f), // E7
	OSC_FREQ_TO_PHI(2793.83f), // F7
	OSC_FREQ_TO_PHI(2959.96f), // F#7

	OSC_FREQ_TO_PHI(3135.96f), // G7
	OSC_FREQ_TO_PHI(3322.44f), // G#7
	OSC_FREQ_TO_PHI(3520.00f), // A7
	OSC_FREQ_TO_PHI(3729.31f), // A#7
};

#define U8REC1(A, M, S) (uint8_t)( (((uint16_t)(A) * (uint16_t)(M)) >> 8u) >> (S) )
#define U8DIVBY12(A)    U8REC1(A, 0xABu, 3u)
/* default to a tick every 40 OSC ticks (i.e. every 40ms) */
#define ATM_DEFAULT_TICK_RATE (39)

/* WARN: passing note_idx == 0 will not return 0 */
__attribute__((weak)) uint16_t note_index_2_phase_inc(const uint8_t note_idx)
{
	const uint8_t adj = U8DIVBY12(note_idx);
	return pgm_read_word(&noteTable[(note_idx-12*adj)]) >> (9-adj);
}

static int16_t slide_quantity_i16(const int16_t amount, const int16_t value, const int16_t bottom, const int16_t top)
{
	const int16_t res = value + amount;
	if (res < bottom) {
		return bottom;
	} else if (res > top) {
		return top;
	}
	return res;
}

#if 0
static int8_t slide_quantity_i8(const int8_t amount, const int8_t value, const int8_t bottom, const int8_t top)
{
	const int16_t res = value + amount;
	if (res < bottom) {
		return bottom;
	} else if (res > top) {
		return top;
	}
	return res;
}
#else
#define slide_quantity_i8 slide_quantity_i16
#endif

static void atm_synth_voice_note_reset(struct atm_note_state *const note_state)
{
	memset(note_state, 0, sizeof(*note_state));
}

__attribute__((weak)) void atm_score_header(const uint8_t *score, struct atm_score_hdr *hdr)
{
	memcpy_P(hdr, score, sizeof(*hdr));
}

__attribute__ ((weak)) const uint8_t *get_pattern_start_ptr(const uint8_t *s, const uint8_t pattern_idx)
{
	const uint8_t ofs = sizeof(struct atm_score_hdr)+sizeof(uint16_t)*pattern_idx;
	return s + pgm_read_word(s+ofs);
}

void atm_synth_player_set_pause(const uint8_t player_id, const bool pause)
{
	struct atm_player_state *p = &atm_players[player_id];
	if (pause) {
		p->active_voices_mask = 0;
	} else {
		p->active_voices_mask = p->used_voices_mask;
	}
}

bool atm_synth_player_get_pause(const uint8_t player_id)
{
	struct atm_player_state *p = &atm_players[player_id];
	return !p->active_voices_mask;
}

static struct atm_voice_state *const init_voice(const struct atm_player_state *const p, atm_pool_slot_idx_t *const slots, const uint8_t pattern_idx, const uint8_t dst_osc_ch_idx)
{
	const atm_pool_slot_idx_t top_frame_slot = slots[0];
	struct atm_voice_frame *const vf = atm_pool_idx2data_ptr(top_frame_slot);
	vf->parent_frame = ATM_POOL_INVALID_SLOT;
	vf->pattern_id = pattern_idx;
	vf->next_cmd_ptr = get_pattern_start_ptr(p->score_start, pattern_idx);

	const atm_pool_slot_idx_t note_state_slot = slots[1];
	struct atm_note_state *const n = atm_pool_idx2data_ptr(note_state_slot);
	atm_synth_voice_note_reset(n);

	const atm_pool_slot_idx_t voice_slot = slots[2];
	struct atm_voice_state *const v = atm_pool_idx2data_ptr(voice_slot);
	atm_pool_next_slot_from_dataptr(v) = ATM_POOL_INVALID_SLOT;
	v->delay = 0;
	v->dst_osc_ch_idx = dst_osc_ch_idx | 0xF8;
	v->fx_head = top_frame_slot;
	v->cur_frame = top_frame_slot;
	v->note_state = n;
	v->next_voice = slots[5];
	return v;
}

int8_t atm_synth_player_setup(const uint8_t player_id, const void *score, const struct atm_entry_patterns *const ep)
{
	/* stop player */
	atm_synth_player_shutdown(player_id);
	/* figure out voice count */
	struct atm_score_hdr hdr;
	atm_score_header(score, &hdr);
	const uint8_t voice_count = ep ? ep->voice_count : hdr.voice_count;
	/* wipe player struct */
	struct atm_player_state *const p = &atm_players[player_id];
	memset(p, 0, sizeof(*p));
	/* setup player */
	p->score_start = score;
	p->tick_rate = ATM_DEFAULT_TICK_RATE;
	/* alloc voices */
	atm_pool_slot_idx_t dst[voice_count*3];
	/* each voice requires 3 slots:
		1 for voice state
		1 for global note state
		1 for the top level effect frame
	*/
	if (atm_pool_alloc(sizeof(dst), dst) == ATM_POOL_INVALID_SLOT) {
		/* allocation failed, abort */
		atm_pool_trace(true, ATM_POOL_INVALID_SLOT);
		return -1;
	}
	/* nonzero used_voices_mask also marks the player as setup */
	p->used_voices_mask = ~(0xFF << voice_count);
	p->voices_head = dst[2];
	/* setup voices */
	struct atm_voice_state *v;
	for (uint8_t voice_index = 0; voice_index < voice_count; voice_index++) {
		atm_set_log_current_voice_index(voice_index);
		const uint8_t osc_idx = ep ? ep->voices[voice_index].osc_idx : voice_index;
		const uint8_t pattern_idx = ep ? ep->voices[voice_index].pattern_idx : voice_index;
		v = init_voice(p, &dst[voice_index*3], pattern_idx, osc_idx);
		atm_log(LOG_DEBUG, "Voice index %hhu -> pattern %hhu\n", atm_current_voice_index(), ep->voices[voice_index].pattern_idx);
	}
	v->next_voice = ATM_POOL_INVALID_SLOT;
	return 0;
}

void atm_synth_player_shutdown(const uint8_t player_index)
{
	struct atm_player_state *const p = &atm_players[player_index];
	atm_set_log_current_player_index(player_index);
	/* stop playback */
	p->active_voices_mask = 0;
	/* bail out if the player wasn't initialised in the first place */
	if (!p->used_voices_mask) {
		return;
	}
	p->used_voices_mask = 0;
	uint8_t voice_slot = p->voices_head;
	while (voice_slot != ATM_POOL_INVALID_SLOT) {
		const struct atm_voice_state *const v = atm_pool_idx2data_ptr(voice_slot);
		voice_slot = v->next_voice;
		atm_pool_trace(false, v->fx_head);
		atm_pool_free_list1(v->fx_head);
	}
}

static inline int16_t _abs_i16(int16_t v)
{
	return v > 0 ? v : -v;
}

static void apply_fx_param_delta(int16_t *const dst_param_array, const uint8_t param_idx, const uint16_t acc_amount)
{
	dst_param_array[param_idx] += acc_amount;
}

struct fx_processing_state {
	uint8_t note;
	int8_t transpose;
	uint8_t triggered_flags;
	uint8_t note_flags;
	int16_t acc_fx_param[4];
};

static void *handle_fx_storage(struct atm_voice_state *const v, const uint8_t id, const uint8_t sz, const uint8_t alloc)
{
	/* iterate over all fx in the current frame */
	atm_pool_slot_idx_t prev_slot = ATM_POOL_INVALID_SLOT;
	atm_pool_slot_idx_t fx_slot = v->fx_head;
	const atm_pool_slot_idx_t sentinel_slot = v->cur_frame;
	struct fx_common_state *c = NULL;
	while (fx_slot != sentinel_slot) {
		c = atm_pool_idx2data_ptr(fx_slot);
		if (c->id == id) {
			if (alloc) {
				/* found it, return it */
				goto clear_and_return;
			} else {
				/* found it, free it */
				const atm_pool_slot_idx_t next_slot = atm_pool_next_slot_from_dataptr(c);
				if (prev_slot != ATM_POOL_INVALID_SLOT) {
					atm_pool_idx2slot_ptr(prev_slot)->next = next_slot;
				} else {
					v->fx_head = next_slot;
				}
				atm_pool_trace(false, fx_slot);
				atm_pool_free(fx_slot);
				return NULL;
			}
		}
		prev_slot = fx_slot;
		fx_slot = atm_pool_next_slot_from_dataptr(c);
	}

	if (!alloc) {
		/* not found but we wanted to free it, return */
		atm_log_event("atm.player.%hhu.voice.%hhu.fx.notfound", "e", atm_current_player_index(), atm_current_voice_index());
		return NULL;
	}

	/* didn't find existing fx, allocate and return */
	fx_slot = atm_pool_alloc1(v->fx_head);
	atm_pool_trace(true, fx_slot);
	if (fx_slot == ATM_POOL_INVALID_SLOT) {
		atm_log_event("atm.player.%hhu.voice.%hhu.fx.outofmemory", "e", atm_current_player_index(), atm_current_voice_index());
		return NULL;
	}
	v->fx_head = fx_slot;
	c = atm_pool_idx2data_ptr(fx_slot);

clear_and_return:
	memset(c, 0, sz);
	/* set ID */
	c->id = id;
	return c;
}

static void process_np_cmd(struct atm_player_state *const p, struct atm_voice_state *const v, const struct atm_cmd_data *const cmd, const uint8_t csz)
{
	const uint8_t cid = cmd->id << 4;
	const uint8_t fx_target_param = cmd->params[0] & ATM_SYNTH_PARAM_MASK;

	/* Parametrised commands */
	if (cid == (ATM_CMD_NP_CALL<<4)) {
		struct atm_voice_frame *vf = atm_pool_idx2data_ptr(v->cur_frame);
		uint8_t new_pattern = ATM_VOICE_PATTERN_ID_MASK;
		if (csz) {
			new_pattern = cmd->params[0] & ATM_VOICE_PATTERN_ID_MASK;
			/* ignore calls to the same pattern */
			if (new_pattern != (vf->pattern_id & ATM_VOICE_PATTERN_ID_MASK)) {
				/* ignore call command if the stack is full */
				atm_pool_slot_idx_t new_frame_slot = atm_pool_alloc1(v->fx_head);
				atm_pool_trace(true, new_frame_slot);
				if (new_frame_slot != ATM_POOL_INVALID_SLOT) {
					v->fx_head = new_frame_slot;
					struct atm_voice_frame *const nf = atm_pool_idx2data_ptr(new_frame_slot);
					nf->parent_frame = v->cur_frame;
					nf->pattern_id = cmd->params[0];
					v->cur_frame = new_frame_slot;
					vf = nf;
				}
			}
	update_next_cmd_ptr:
			if (new_pattern != ATM_VOICE_PATTERN_ID_MASK) {
				vf->next_cmd_ptr = get_pattern_start_ptr(p->score_start, new_pattern);
			}
		} else {
			uint8_t repeat_counter = vf->pattern_id >> 5;
			if (--repeat_counter != 255) {
				new_pattern = vf->pattern_id & ATM_VOICE_PATTERN_ID_MASK;
				vf->pattern_id = new_pattern | (repeat_counter << 5);
				/* Repeat track */
				goto update_next_cmd_ptr;
			} else {
				/* Check stack depth */
				if (vf->parent_frame == ATM_POOL_INVALID_SLOT) {
					v->delay = 0xFF;
					/* reset voice vol, mod, note */
					atm_synth_voice_note_reset(v->note_state);
					/* reset pattern pointer in preparation for loop */
					new_pattern = (v->dst_osc_ch_idx >> 3) & ATM_VOICE_PATTERN_ID_MASK;
					goto update_next_cmd_ptr;
				} else {
					/* pop stack */
					const atm_pool_slot_idx_t first_discarded_slot = v->fx_head;
					const atm_pool_slot_idx_t last_discarded_slot = v->cur_frame;
					v->fx_head = atm_pool_next_slot_from_dataptr(vf);
					v->cur_frame = vf->parent_frame;
					atm_pool_trace(false, first_discarded_slot);
					atm_pool_free_list2(first_discarded_slot, last_discarded_slot);
				}
			}
		}
	} else if (cid == (ATM_CMD_NP_ARPEGGIO<<4)) {
#if ATM_HAS_FX_NOTE_RETRIG
		const uint8_t id = ATM_SYNTH_PARAM_TSP | cid;
		struct atm_fx_arpeggio_slot *fx = handle_fx_storage(v, id, sizeof(struct atm_fx_arpeggio_slot), csz);
		if (fx) {
			atm_log_event("atm.player.%hhu.voice.%hhu.fx.arp.add", "e", atm_current_player_index(), atm_current_voice_index());
			fx->common.flags = ((cmd->params[0] & FX_COMMON_FLAGS_RETRIGGER_ON_NOTEON_MASK) |
								FX_ARPEGGIO_DEFAULT_BASE_FLAGS);
			fx->common.interval = cmd->params[0] & FX_ARPEGGIO_INTERVAL_MASK;
			fx->timing = cmd->params[0] & FX_ARPEGGIO_ENDSTATE_MASK;
			if (csz > 1) {
				fx->notes = cmd->params[1];
			} else {
				/* make it 0xFF */
				fx->notes = ~fx->notes;
			}
		} else {
			atm_log_event("atm.player.%hhu.voice.%hhu.fx.arp.remove", "e", atm_current_player_index(), atm_current_voice_index());
		}
#endif /* ATM_HAS_FX_NOTE_RETRIG */
	} else if (cid == (ATM_CMD_NP_SLIDE<<4)) {
#if ATM_HAS_FX_SLIDE
		const uint8_t id = fx_target_param | cid;
		struct atm_fx_slide_slot *fx = handle_fx_storage(v, id, sizeof(struct atm_fx_slide_slot), csz);
		if (fx) {
			atm_log_event("atm.player.%hhu.voice.%hhu.fx.slide.add", "e", atm_current_player_index(), atm_current_voice_index());
			fx->common.flags = cmd->params[0];
			fx->amount = (int8_t)cmd->params[1];
			if (csz > 2) {
				if (cmd->params[0] & FX_SLIDE_LIMIT_FLAG_MASK) {
					fx->target = (int8_t)cmd->params[2];
				} else {
					fx->common.interval = cmd->params[2];
				}
			} else {
				fx->target = fx->amount > 0 ? MAX_VOLUME : -MAX_VOLUME;
			}
			atm_log_event("atm.player.%hhu.voice.%hhu.fx.slide.amount", "%hhd f", atm_current_player_index(), atm_current_voice_index(), fx->amount);
			atm_log_event("atm.player.%hhu.voice.%hhu.fx.slide.target", "%hhd f", atm_current_player_index(), atm_current_voice_index(), fx->target);
			atm_log_event("atm.player.%hhu.voice.%hhu.fx.slide.interval", "%hhd f", atm_current_player_index(), atm_current_voice_index(), fx->common.interval);
			atm_log_event("atm.player.%hhu.voice.%hhu.fx.slide.flags", "%hhd f", atm_current_player_index(), atm_current_voice_index(), fx->common.flags);
		}
#endif /* ATM_HAS_FX_SLIDE */
	} else if (cid == (ATM_CMD_NP_LFO<<4)) {
#if ATM_HAS_FX_LFO
		const uint8_t id = fx_target_param | cid;
		struct atm_fx_lfo_slot *fx = handle_fx_storage(v, id, sizeof(struct atm_fx_lfo_slot), csz);
		if (fx) {
			fx->common.flags = (cmd->params[0] & (FX_COMMON_FLAGS_RETRIGGER_ON_NOTEON_MASK | FX_COMMON_FLAGS_DESTINATION_PARAM_MASK));
			fx->depth = cmd->params[1] & FX_LFO_DEPTH_MASK;
			fx->step = (cmd->params[2] >> 4)+1;
			fx->common.interval = cmd->params[2] & 0x0F;
		}
#endif /* ATM_HAS_FX_LFO */
	} else if (cid == (ATM_CMD_NP_ADD_TO_PARAM<<4)) {
		atm_log_event("atm.player.%hhu.voice.%hhu.cmd.add.%s", "%hhd f", atm_current_player_index(), atm_current_voice_index(), atm_log_fx_dest_label(cmd->params[0]), cmd->params[1]);
		v->note_state->params[fx_target_param] += (int8_t)cmd->params[1];
	} else if (cid == (ATM_CMD_NP_SET_PARAM<<4)) {
		atm_log_event("atm.player.%hhu.voice.%hhu.cmd.set.%s", "%hhd f", atm_current_player_index(), atm_current_voice_index(), atm_log_fx_dest_label(cmd->params[0]), cmd->params[1]);
		v->note_state->params[fx_target_param] = cmd->params[1];
	} else if (cid == (ATM_CMD_NP_SET_TEMPO<<4)) {
		p->tick_rate = cmd->params[0];
		atm_log_event("atm.player.%hhu.voice.%hhu.tempo", "%hhu f", atm_current_player_index(), atm_current_voice_index(), p->tick_rate);
	} else if (cid == (ATM_CMD_NP_ADD_TEMPO<<4)) {
		p->tick_rate += (int8_t)cmd->params[0];
		atm_log_event("atm.player.%hhu.voice.%hhu.tempo", "%hhu f", atm_current_player_index(), atm_current_voice_index(), p->tick_rate);
	} else if (cid == (ATM_CMD_NP_SET_WAVEFORM<<4)) {
		v->note_state->flags ^= (v->note_state->flags ^ cmd->params[0]) & 0x3;
		/* Seed noise LSFR (will not affect square waveforms) */
		const uint8_t dst_osc_ch_idx = v->dst_osc_ch_idx & 0x07;
		osc_state_array[dst_osc_ch_idx].phase_acc.u8.lo |= 0x01;
	} else if (cid == (ATM_CMD_NP_SET_LOOP_PATTERN<<4)) {
		v->dst_osc_ch_idx = (v->dst_osc_ch_idx & 0x7) | (cmd->params[0] << 3);
	} else {
		atm_log(LOG_ERR, "Unknown command: 0x%02hhx", cmd->id);
		exit(1);
	}
}

void atm_process_cmd(struct atm_player_state *const p, struct atm_voice_state *const v, const struct atm_cmd_data *const cmd)
{
	struct atm_voice_frame *const vf = atm_pool_idx2data_ptr(v->cur_frame);

	vf->next_cmd_ptr += 1;

	if (cmd->id >= ATM_CMD_I_NOTE_OFF) {
		/* 0 ... 63 : NOTE ON/OFF */
		struct atm_note_state *const n = v->note_state;
		if (cmd->id == ATM_CMD_I_NOTE_OFF) {
			/* cmd->id == ATM_CMD_I_NOTE_OFF : Note OFF */
			n->note = 0;
		} else if (cmd->id == 255) {
			/* when cmd->id == 255 note value is in cmd->params[0] (allows to access more octaves) */
			/* cmd->params[0] == 0 : Note OFF, cmd->params[0] == 1 : C-2, cmd->params[0] == 119 : A#7 */
			n->note = cmd->params[0];
		} else {
			/* cmd->id == ATM_CMD_I_NOTE_OFF+1 : C2, cmd->id == 255-1 : C#7 */
			n->note = cmd->id-ATM_CMD_I_NOTE_OFF+48;
		}
		n->flags |= FX_COMMON_FLAGS_RETRIGGER_ON_NOTEON_MASK;
		atm_log_event("atm.player.%hhu.voice.%hhu.note", "%hhu f", atm_current_player_index(), atm_current_voice_index(), n->note);
	} else if (cmd->id >= ATM_CMD_I_DELAY_1_TICK) {
		/* delay */
		v->delay = cmd->id - ATM_CMD_I_DELAY_1_TICK+1;
		atm_log_event("atm.player.%hhu.voice.%hhu.delay", "%hhu f", atm_current_player_index(), atm_current_voice_index(), v->delay);
	} else if (cmd->id < ATM_CMD_BLK_N_PARAMETER) {
		const uint8_t csz = (cmd->id >> 4);
		/* n parameter byte command */
		atm_log_event("atm.player.%hhu.voice.%hhu.cmd", "%hhu f (%hhu) 0x%02hhx 0x%02hhx 0x%02hhx 0x%02hhx", atm_current_player_index(), atm_current_voice_index(), cmd->id, csz, cmd->id, cmd->params[0], cmd->params[1], cmd->params[2]);
		/* process_np_cmd() can modify next_cmd_ptr so increase it first */
		vf->next_cmd_ptr += csz;
		process_np_cmd(p, v, cmd, csz);
	} else {
		atm_log(LOG_ERR, "Unknown command: 0x%02hhx", cmd->id);
		exit(1);
	}
}

static void apply_slide_fx(struct fx_processing_state *const s, struct fx_common_state *const c, const uint8_t triggered, const uint8_t update_due)
{
	struct atm_fx_slide_slot *const fx = container_of(c, struct atm_fx_slide_slot, common);
	if (triggered) {
		fx->acc_amount = 0;
	}

	const uint8_t param = fx->common.flags & FX_COMMON_FLAGS_DESTINATION_PARAM_MASK;
	if (update_due) {
		const int16_t tmp = fx->acc_amount + fx->amount;
		fx->acc_amount = tmp;
		if ((fx->target >= 0 && tmp >= fx->target) || (fx->target < 0 && tmp < fx->target)) {
			fx->acc_amount = fx->target;
			atm_log_event("atm.player.%hhu.voice.%hhu.fx.slide.%s.clamp", "e", atm_current_player_index(), atm_current_voice_index(), atm_log_fx_dest_label(param));
		}
		atm_log_event("atm.player.%hhu.voice.%hhu.fx.slide.%s", "%hhd f", atm_current_player_index(), atm_current_voice_index(), atm_log_fx_dest_label(param), fx->acc_amount);
		atm_log_event("atm.player.%hhu.voice.%hhu.fx.slide.%s.target", "%hhd f", atm_current_player_index(), atm_current_voice_index(), atm_log_fx_dest_label(param), fx->target);
	}
	apply_fx_param_delta(s->acc_fx_param, param, fx->acc_amount);
}

static void apply_lfo_fx(struct fx_processing_state *const s, struct fx_common_state *const c, const uint8_t triggered, const uint8_t update_due)
{
	struct atm_fx_lfo_slot *const fx = container_of(c, struct atm_fx_lfo_slot, common);
	if (triggered) {
		fx->acc_amount = 0;
	}

	if (update_due) {
		int16_t amount = fx->acc_amount;

		amount += fx->step;
		const int16_t diff = _abs_i16(amount) - fx->depth;
		if (diff >= 0) {
			fx->step = -fx->step;
			if (diff) {
				goto skip_update_amount;
			}
		}
		fx->acc_amount = amount;
	skip_update_amount:
		;
	}
	const uint8_t param = fx->common.flags & FX_COMMON_FLAGS_DESTINATION_PARAM_MASK;
	atm_log_event("atm.player.%hhu.voice.%hhu.fx.lfo.%s", "%hhd f", atm_current_player_index(), atm_current_voice_index(), atm_log_fx_dest_label(param), fx->acc_amount);
	apply_fx_param_delta(s->acc_fx_param, param, fx->acc_amount);
}

static void apply_arpeggio_fx(struct fx_processing_state *const s, struct fx_common_state *const c, const uint8_t triggered, const uint8_t update_due)
{
	struct atm_fx_arpeggio_slot *const fx = container_of(c, struct atm_fx_arpeggio_slot, common);

	if (triggered) {
		fx->state = 0;
	}

	/* unpack arpeggio FX state */
	const uint8_t arp_cmp = fx->timing;
	/* arp_state: sss-----
		000: first note
		001: second note
		010: third note
		011: done
	*/

	if (fx->state != arp_cmp) {
		if (update_due) {
			fx->state += 0x20;
			if (fx->state == arp_cmp) {
				/* auto re-trigger or note trigger? */
				const uint8_t note_retrigger = fx->common.flags & FX_COMMON_FLAGS_RETRIGGER_ON_NOTEON_MASK;
				if (note_retrigger) {
					atm_log_event("atm.player.%hhu.voice.%hhu.fx.arp.hold", "e", atm_current_player_index(), atm_current_voice_index());
					goto arp_hold;
				}
				fx->state = 0x00;
			}
		}
		/* apply effect */
		uint8_t arp_transpose = 0;
		if (fx->state == 0x20) {
			arp_transpose = fx->notes >> 4;
		} else if (fx->state == 0x40) {
			arp_transpose = fx->notes & 0x0F;
		}
		if (arp_transpose == 0xF) {
			/* notecut */
			atm_log_event("atm.player.%hhu.voice.%hhu.fx.arp.notecut", "e", atm_current_player_index(), atm_current_voice_index());
			goto arp_hold;
		} else {
			atm_log_event("atm.player.%hhu.voice.%hhu.fx.arp.transpose", "%hhu f", atm_current_player_index(), atm_current_voice_index(), arp_transpose);
			apply_fx_param_delta(s->acc_fx_param, ATM_SYNTH_PARAM_TSP, arp_transpose);
		}
	} else {
		atm_log_event("atm.player.%hhu.voice.%hhu.fx.arp.hold", "e", atm_current_player_index(), atm_current_voice_index());
arp_hold:
		/* when arpeggio is on hold note/transpose force note OFF */
		s->note = 0;
	}
}


static void dispatch_voice_fx(struct fx_processing_state *const s, struct fx_common_state *const c, const uint8_t triggered, const uint8_t update_due)
{
	const uint8_t fx_id = c->id & 0xF0;
	if (fx_id == (ATM_CMD_NP_ARPEGGIO<<4)) {
		#if ATM_HAS_FX_NOTE_RETRIG
			apply_arpeggio_fx(s, c, triggered, update_due);
		#endif /* ATM_HAS_FX_NOTE_RETRIG */
	} else if (fx_id == (ATM_CMD_NP_LFO<<4)) {
		#if ATM_HAS_FX_LFO
			apply_lfo_fx(s, c, triggered, update_due);
		#endif /* ATM_HAS_FX_LFO */
	} else if (fx_id == (ATM_CMD_NP_SLIDE<<4)) {
		#if ATM_HAS_FX_SLIDE
			apply_slide_fx(s, c, triggered, update_due);
		#endif /* ATM_HAS_FX_SLIDE */
	}
}

static void process_voice_fx(struct fx_processing_state *const s, struct fx_common_state *const c)
{
	/* skip if note is off and FX requires note to be on */
	if ((c->flags & FX_COMMON_FLAGS_SKIP_ON_NOTEOFF_MASK) && !s->note) {
		atm_log_event("atm.player.%hhu.voice.%hhu.fx.%s.%s.skip", "e", atm_current_player_index(), atm_current_voice_index(), atm_log_cmd_label(c->id >> 4), atm_log_fx_dest_label(c->id));
		return;
	}
	atm_log_event("atm.player.%hhu.voice.%hhu.fx.%s.%s.process", "e", atm_current_player_index(), atm_current_voice_index(), atm_log_cmd_label(c->id >> 4), atm_log_fx_dest_label(c->id));

	/* figure out if we should re-trigger */
	const uint8_t triggered = (c->flags & FX_COMMON_FLAGS_RETRIGGER_ON_ANY_MASK) & s->triggered_flags;
	if (triggered) {
		atm_log_event("atm.player.%hhu.voice.%hhu.fx.%s.%s.triggered", "e", atm_current_player_index(), atm_current_voice_index(), atm_log_cmd_label(c->id >> 4), atm_log_fx_dest_label(c->id));
		c->count = 0;
	}

	const uint8_t update_due = c->count > c->interval;
	if (update_due) {
		atm_log_event("atm.player.%hhu.voice.%hhu.fx.%s.%s.update", "e", atm_current_player_index(), atm_current_voice_index(), atm_log_cmd_label(c->id >> 4), atm_log_fx_dest_label(c->id));
		c->count = 1;
	} else {
		c->count++;
	}

	dispatch_voice_fx(s, c, triggered, update_due);

	if ((c->flags & FX_COMMON_FLAGS_TRIGGERS_TRANSPOSITION_MASK) && (c->count > c->interval)) {
		atm_log_event("atm.player.%hhu.voice.%hhu.fx.%s.%s.triggers", "e", atm_current_player_index(), atm_current_voice_index(), atm_log_cmd_label(c->id >> 4), atm_log_fx_dest_label(c->id));
		s->note_flags |= FX_COMMON_FLAGS_RETRIGGER_ON_TRANSPOSITION_MASK;
	}
}

static struct osc_params apply_voice_effects(const struct atm_player_state *const p, struct atm_voice_state *const v) {
	struct atm_note_state *const note_state = v->note_state;
	struct fx_processing_state fx_s = {
		.note = note_state->note,
		.triggered_flags = (note_state->flags & FX_COMMON_FLAGS_RETRIGGER_ON_ANY_MASK),
		/* turn off `just triggered` flags */
		.note_flags = note_state->flags & (~FX_COMMON_FLAGS_RETRIGGER_ON_ANY_MASK),
		.acc_fx_param = {0, 0, note_state->params[ATM_SYNTH_PARAM_TSP], 0},
	};

	if (fx_s.triggered_flags) {
		atm_log_event("atm.player.%hhu.voice.%hhu.triggered", "e", atm_current_player_index(), atm_current_voice_index());
	}

	/* Process all effects in the effects stack skipping stack frames */	
	atm_pool_slot_idx_t next_frame_slot = v->cur_frame;
	atm_pool_slot_idx_t fx_slot = v->fx_head;
	while (fx_slot != ATM_POOL_INVALID_SLOT && next_frame_slot != ATM_POOL_INVALID_SLOT) {
		/* skip voice frame */
		if (fx_slot == next_frame_slot) {
			const struct atm_voice_frame *const vf = atm_pool_idx2data_ptr(next_frame_slot);
			next_frame_slot = vf->parent_frame;
			fx_slot = atm_pool_next_slot_from_dataptr(vf);
			continue;
		}
		struct fx_common_state *const c = atm_pool_idx2data_ptr(fx_slot);
		process_voice_fx(&fx_s, c);
		fx_slot = atm_pool_next_slot_from_dataptr(c);
	}

	/* turn off `just triggered` flag */
	const uint8_t note_flags = note_state->flags = fx_s.note_flags;

	atm_log_event("atm.player.%hhu.voice.%hhu.fx.acc_dev.vol", "%hd f", atm_current_player_index(), atm_current_voice_index(), fx_s.acc_fx_param[ATM_SYNTH_PARAM_VOL]);
	atm_log_event("atm.player.%hhu.voice.%hhu.fx.acc_dev.mod", "%hd f", atm_current_player_index(), atm_current_voice_index(), fx_s.acc_fx_param[ATM_SYNTH_PARAM_MOD]);
	atm_log_event("atm.player.%hhu.voice.%hhu.fx.acc_dev.transpose", "%hd f", atm_current_player_index(), atm_current_voice_index(), fx_s.acc_fx_param[ATM_SYNTH_PARAM_TSP]);
	atm_log_event("atm.player.%hhu.voice.%hhu.fx.acc_dev.phi", "%hd f", atm_current_player_index(), atm_current_voice_index(), fx_s.acc_fx_param[ATM_SYNTH_PARAM_PHI]);
	atm_log_event("atm.player.%hhu.voice.%hhu.fx.acc_dev.note_flags", "%hhd f", atm_current_player_index(), atm_current_voice_index(), note_flags);
	atm_log_event("atm.player.%hhu.voice.%hhu.fx.acc_dev.note", "%hhd f", atm_current_player_index(), atm_current_voice_index(), fx_s.note);

	const uint8_t final_vol = slide_quantity_i16(fx_s.acc_fx_param[ATM_SYNTH_PARAM_VOL], note_state->params[ATM_SYNTH_PARAM_VOL], 0, OSC_MAX_VOLUME);
	/* no note -> no sound */
	struct osc_params osc_params;
	if (fx_s.note) {
		osc_params.vol = final_vol;
		osc_params.mod = slide_quantity_i16(fx_s.acc_fx_param[ATM_SYNTH_PARAM_MOD], note_state->params[ATM_SYNTH_PARAM_MOD], OSC_MOD_MIN, OSC_MOD_MAX);
		osc_params.phase_inc.u16 = slide_quantity_i16(fx_s.acc_fx_param[ATM_SYNTH_PARAM_PHI], note_index_2_phase_inc(fx_s.note+fx_s.acc_fx_param[ATM_SYNTH_PARAM_TSP]), 0, OSC_PHASE_INC_MAX);
		if (note_flags & 0x03) {
			/* select noise OSC mode */
			osc_params.phase_inc.u8.hi |= 0x80;
		}
	} else {
		/* WARN: other fields are unintialized. This is not an issue because volume is zero. */
		osc_params.vol = 0;
	#ifndef __AVR__
		/* Zero out OSC parameters on platforms other than the target (for testing and development) */
		osc_params.mod = 0;
		osc_params.phase_inc.u16 = 0;
	#endif
	}
	return osc_params;
}

__attribute__((weak)) void atm_next_command(struct atm_voice_state *const v, const uint8_t voice_index, const uint8_t *const cmd_ptr, struct atm_cmd_data *const dst)
{
	memcpy_P(dst, cmd_ptr, sizeof(*dst));
}

static uint8_t process_voice(struct atm_player_state *const p, struct atm_voice_state *const v, const uint8_t voice_index, const uint8_t unused_osc_ch_mask)
{
	while (v->delay == 0) {
		struct atm_cmd_data cmd;
		struct atm_voice_frame *const vf = atm_pool_idx2data_ptr(v->cur_frame);
		/*
		Reading the command first and then its parameters
		takes up more progmem so we read a fixed amount.
		maximum command size is 4 bytes right now
		*/
		atm_next_command(v, voice_index, vf->next_cmd_ptr, &cmd);
		atm_process_cmd(p, v, &cmd);
		atm_log_event("atm.player.%hhu.voice.%hhu.pattern", "%hhu f", atm_current_player_index(), atm_current_voice_index(), (uint8_t)(vf->pattern_id & ATM_VOICE_PATTERN_ID_MASK));
	}

	if (v->delay != 0xFF) {
		/* effects */
		{
			struct osc_params osc_params = apply_voice_effects(p, v);
			const uint8_t osc_ch_idx = v->dst_osc_ch_idx & 0x7;
			const uint8_t osc_ch_mask = 1 << osc_ch_idx;
			/* skip overwriting OSC params if it was already set */
			if (unused_osc_ch_mask & osc_ch_mask) {
				p->used_osc_ch_mask |= osc_ch_mask;
				/* Make this assignment atomic to avoid glitches */
				cli();
				osc_state_array[osc_ch_idx].params = osc_params;
				sei();
				atm_log_event("atm.player.%hhu.voice.%hhu.oscparam_set", "e", atm_current_player_index(), atm_current_voice_index());
				atm_log_event("osc.channels.%hhu.vol", "%hhu f", osc_ch_idx, osc_params.vol);
				atm_log_event("osc.channels.%hhu.mod", "%hhd f", osc_ch_idx, osc_params.mod);
				atm_log_event("osc.channels.%hhu.phi", "%hu f", osc_ch_idx, osc_params.phase_inc.u16);
			} else {
				atm_log_event("atm.player.%hhu.voice.%hhu.oscparam_skip", "e", atm_current_player_index(), atm_current_voice_index());
			}
		}
		v->delay--;
		atm_log_event("atm.player.%hhu.voice.%hhu.delay", "%hhu f", atm_current_player_index(), atm_current_voice_index(), v->delay);
		return 1;
	}

	atm_log_event("atm.player.%hhu.voice.%hhu.stop", "e", atm_current_player_index(), atm_current_voice_index());
	return 0;
}

static void process_player(struct atm_player_state *const p, const uint8_t unused_osc_ch_mask)
{
	uint8_t active_voices_mask = p->active_voices_mask;
	uint8_t loop_restart = 0;

	if (--p->tick_counter != 255) {
		return;
	}

	atm_log_event("atm.player.%hhu.tick", "e", atm_current_player_index());

start_loop:
	p->used_osc_ch_mask = 0;

	{
		uint8_t voice_slot = p->voices_head;
		uint8_t voice_index = 0;
		uint8_t voice_mask = 1;

		while (voice_slot != ATM_POOL_INVALID_SLOT) {
			struct atm_voice_state *v = atm_pool_idx2data_ptr(voice_slot);
			voice_slot = v->next_voice;
			atm_set_log_current_voice_index(voice_index);
			if (voice_mask & active_voices_mask) {
				/* No loop pattern if all bits are set */
				if (loop_restart && (v->dst_osc_ch_idx >> 3 != ATM_VOICE_PATTERN_ID_MASK)) {
					v->delay = 0;
				}
				if (!process_voice(p, v, voice_index++, unused_osc_ch_mask)) {
					p->active_voices_mask &= ~voice_mask;
				}
			}
			voice_mask <<= 1;
		}

#if ATM_HAS_FX_LOOP
		/* if all channels are inactive, stop playing or check for repeat */
		if (!p->active_voices_mask) {
			loop_restart++;
			if (atm_player_loop(p) && loop_restart == 1) {
				atm_log_event("atm.player.%hhu.loop", "e", atm_current_player_index());
				active_voices_mask = p->active_voices_mask = p->used_voices_mask;
				/* all voices became inactive during this round, check if any is looping */
				goto start_loop;
			} else {
				atm_log_event("atm.player.%hhu.stop", "e", atm_current_player_index());
			}
			/* no pattern loops to restart, exit */
		}
#endif
	}

	p->tick_counter = p->tick_rate;
}

void atm_setup(void)
{
	atm_pool_init();
}

uint8_t atm_tick_handler(uint8_t unused_osc_ch_mask)
{
	atm_log_event("osc.tick", "e");
	for (uint8_t player_index = 0; player_index < atm_player_count; player_index++) {
		atm_set_log_current_player_index(player_index);
		struct atm_player_state *const p = &atm_players[player_index];
		if (!p->active_voices_mask) {
			continue;
		}
		process_player(p, unused_osc_ch_mask);
		unused_osc_ch_mask &= ~p->used_osc_ch_mask;
	}
	return unused_osc_ch_mask;
}

static void atm_silence_unused_osc_channels(uint8_t unused_osc_ch_mask)
{
	/* Set volume of all unused OSC channels to 0 */
	struct osc_state *s = osc_state_array;
	while (unused_osc_ch_mask) {
		if (unused_osc_ch_mask & 0x01) {
			s->params.vol = 0;
		}
		unused_osc_ch_mask >>= 1;
		s++;
	}
}

__attribute__((weak)) void osc_tick_handler(void)
{
	uint8_t unused_osc_ch_mask = atm_tick_handler((1 << OSC_CH_COUNT)-1);
	atm_silence_unused_osc_channels(unused_osc_ch_mask);
}

__attribute__((weak)) uint8_t atm_player_loop(struct atm_player_state *const p)
{
	return 1;
}
