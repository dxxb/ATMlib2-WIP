
#ifndef _ATM_SYNTH_INTERNAL_H
#define _ATM_SYNTH_INTERNAL_H

#include "atm_config.h"
#include "atm_pool.h"

#define FX_COMMON_FLAGS_RETRIGGER_ON_NOTEON_MASK (0x80)
#define FX_COMMON_FLAGS_RETRIGGER_ON_TRANSPOSITION_MASK (0x40)
#define FX_COMMON_FLAGS_RETRIGGER_ON_ANY_MASK (FX_COMMON_FLAGS_RETRIGGER_ON_NOTEON_MASK | FX_COMMON_FLAGS_RETRIGGER_ON_TRANSPOSITION_MASK)
#define FX_COMMON_FLAGS_TRIGGERS_TRANSPOSITION_MASK (0x20)
#define FX_COMMON_FLAGS_SKIP_ON_NOTEOFF_MASK (0x10)
#define FX_COMMON_FLAGS_DESTINATION_PARAM_MASK (0x03)

struct fx_common_state {
	uint8_t id;
	uint8_t flags; /* ntTo--pp
								n: retrigger with note
								t: retrigger with transpose
								T: triggers transposition when due
								o: needs note-on
								p: destination param id
					*/
	uint8_t interval;
	uint8_t count;
} __attribute__((packed));

#define FX_ARPEGGIO_DEFAULT_BASE_FLAGS (FX_COMMON_FLAGS_TRIGGERS_TRANSPOSITION_MASK | FX_COMMON_FLAGS_SKIP_ON_NOTEOFF_MASK)
#define FX_ARPEGGIO_INTERVAL_MASK (0x1F)
#define FX_ARPEGGIO_ENDSTATE_MASK (0x60)

//#if ATM_HAS_FX_NOTE_RETRIG
	struct atm_fx_arpeggio_slot {
		struct fx_common_state common;
		uint8_t notes;       // notes: base, base+[7:4], base+[7:4]+[3:0], if FF => note cut ON
		uint8_t timing;      // [7] = reserved, [6] = not third note ,[5] = retrigger, [4:0] = tick count
		uint8_t state;
	} __attribute__((packed));
//#endif /* ATM_HAS_FX_NOTE_RETRIG */

#define FX_SLIDE_INTERVAL_MASK (0x1F)
#define FX_SLIDE_RETRIGGERED_MASK (FX_COMMON_FLAGS_RETRIGGER_ON_ANY_MASK)
#define FX_SLIDE_LIMIT_FLAG_MASK (0x04)

//#if ATM_HAS_FX_SLIDE
struct atm_fx_slide_slot {
	struct fx_common_state common;
	int8_t acc_amount;
	int8_t target; /* making all fx structs the same size results in smaller code */
	int8_t amount;
} __attribute__((packed));
//#endif /* ATM_HAS_FX_SLIDE */


#define FX_LFO_DEPTH_MASK (0x7F)

//#if ATM_HAS_FX_LFO
	struct atm_fx_lfo_slot {
		struct fx_common_state common;
		int8_t acc_amount;
		uint8_t depth;
		int8_t step;
	} __attribute__((packed));
//#endif /* ATM_HAS_FX_LFO */

#define ATM_VOICE_PATTERN_ID_MASK (0x1F)

struct atm_voice_frame {
	atm_pool_slot_idx_t parent_frame;
	uint8_t pattern_id; /* rrrppppp repeat count + pattern id  */
	const uint8_t *next_cmd_ptr; /* ptr */
} __attribute__((packed));

struct atm_note_state {
	uint8_t note; /* --nnnnnn n: note number */
	uint8_t flags; /* ta----ww t: note triggered this round, a: arpeggio trigger next round, w: waveform id */
	int8_t params[3];
} __attribute__((packed));

struct atm_voice_state {
	uint8_t delay; /* --dddddd */
	uint8_t dst_osc_ch_idx; /* lllllddd : loop index + OSC dst channel index */
	atm_pool_slot_idx_t fx_head;
	atm_pool_slot_idx_t cur_frame;
	atm_pool_slot_idx_t next_voice;
	struct atm_note_state *note_state;
} __attribute__((packed));

union atm_slot {
	struct atm_fx_arpeggio_slot arp;
	struct atm_fx_slide_slot slide;
	struct atm_fx_lfo_slot lfo;
	struct atm_voice_frame vframe;
	struct atm_note_state note;
	struct atm_voice_state voice;
	struct atm_note_state note_state;
} __attribute__((packed));

struct atm_pool_slot {
	atm_pool_slot_idx_t next;
	union atm_slot data;
} __attribute__((packed));

extern struct atm_pool_slot atm_mem_pool[];

struct atm_cmd_data {
	uint8_t id;
	uint8_t params[3];
} __attribute__((packed));

#endif /* _ATM_SYNTH_INTERNAL_H */
