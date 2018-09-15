
/* compile only when included by atm_synth.c */
#ifdef _ATM_LIB_H

#include "atm_cmd_constants.h"

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
		struct atm_fx_arpeggio_slot *fx = handle_fx_storage(v, cid, sizeof(struct atm_fx_arpeggio_slot), csz);
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
			fx->common.flags = cmd->params[0];
			if (csz > 2) {
				if (cmd->params[0] & FX_SLIDE_LIMIT_FLAG_MASK) {
					fx->target = (int8_t)cmd->params[2];
				} else {
					fx->common.interval = cmd->params[2];
				}
			} else {
				fx->target = fx->amount > 0 ? MAX_VOLUME : -MAX_VOLUME;
			}
			fx->amount = (int8_t)cmd->params[1];
		}
#endif /* ATM_HAS_FX_SLIDE */
	} else if (cid == (ATM_CMD_NP_LFO<<4)) {
#if ATM_HAS_FX_LFO
		const uint8_t id = fx_target_param | cid;
		struct atm_fx_lfo_slot *fx = handle_fx_storage(v, id, sizeof(struct atm_fx_lfo_slot), csz);
		if (fx) {
			fx->common.flags = (cmd->params[0] & (FX_COMMON_FLAGS_RETRIGGER_ON_NOTEON_MASK | FX_COMMON_FLAGS_DESTINATION_PARAM_MASK));
			fx->depth = cmd->params[1] & FX_LFO_DEPTH_MASK;
			fx->acc_amount = -fx->depth;
			fx->slope = (cmd->params[2] >> 4)+1;
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

static void process_cmd(struct atm_player_state *const p, struct atm_voice_state *const v, const struct atm_cmd_data *const cmd)
{
	struct atm_voice_frame *const vf = atm_pool_idx2data_ptr(v->cur_frame);

	vf->next_cmd_ptr += 1;

	if (cmd->id >= ATM_CMD_I_NOTE_OFF) {
		/* 0 ... 63 : NOTE ON/OFF */
		struct atm_note_state *const n = v->note_state;
		n->note = cmd->id-ATM_CMD_I_NOTE_OFF;
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
#endif
