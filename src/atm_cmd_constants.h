
#include <stdint.h>

#ifndef ATM_CMD_CONSTANTS_H
#define ATM_CMD_CONSTANTS_H

#define MAX_VOLUME (127)

#define ATM_SYNTH_PARAM_MASK (0x03)
#define ATM_SYNTH_PARAM_VOL (0)
#define ATM_SYNTH_PARAM_MOD (1)
#define ATM_SYNTH_PARAM_TSP (2)
#define ATM_SYNTH_PARAM_PHI (3)

enum atm_cmd_blocks_constants {
	ATM_CMD_BLK_N_PARAMETER = 0x80,
};

enum atm_note_cmd_constants {

	/* Use as ATM_CMD_I_DELAY_1_TICK+[num-ticks] e.g. ATM_CMD_I_DELAY_1_TICK+3 for 4 ticks */
	ATM_CMD_I_DELAY_1_TICK = 0xA0,

	ATM_CMD_I_NOTE_OFF = 0xC0,

	ATM_CMD_I_NOTE_C2,
	ATM_CMD_I_NOTE_C2_,
	ATM_CMD_I_NOTE_D2,
	ATM_CMD_I_NOTE_D2_,
	ATM_CMD_I_NOTE_E2,
	ATM_CMD_I_NOTE_F2,
	ATM_CMD_I_NOTE_F2_,
	ATM_CMD_I_NOTE_G2,
	ATM_CMD_I_NOTE_G2_,
	ATM_CMD_I_NOTE_A2,
	ATM_CMD_I_NOTE_A2_,
	ATM_CMD_I_NOTE_B2,

	ATM_CMD_I_NOTE_C3,
	ATM_CMD_I_NOTE_C3_,
	ATM_CMD_I_NOTE_D3,
	ATM_CMD_I_NOTE_D3_,
	ATM_CMD_I_NOTE_E3,
	ATM_CMD_I_NOTE_F3,
	ATM_CMD_I_NOTE_F3_,
	ATM_CMD_I_NOTE_G3,
	ATM_CMD_I_NOTE_G3_,
	ATM_CMD_I_NOTE_A3,
	ATM_CMD_I_NOTE_A3_,
	ATM_CMD_I_NOTE_B3,

	ATM_CMD_I_NOTE_C4,
	ATM_CMD_I_NOTE_C4_,
	ATM_CMD_I_NOTE_D4,
	ATM_CMD_I_NOTE_D4_,
	ATM_CMD_I_NOTE_E4,
	ATM_CMD_I_NOTE_F4,
	ATM_CMD_I_NOTE_F4_,
	ATM_CMD_I_NOTE_G4,
	ATM_CMD_I_NOTE_G4_,
	ATM_CMD_I_NOTE_A4,
	ATM_CMD_I_NOTE_A4_,
	ATM_CMD_I_NOTE_B4,

	ATM_CMD_I_NOTE_C5,
	ATM_CMD_I_NOTE_C5_,
	ATM_CMD_I_NOTE_D5,
	ATM_CMD_I_NOTE_D5_,
	ATM_CMD_I_NOTE_E5,
	ATM_CMD_I_NOTE_F5,
	ATM_CMD_I_NOTE_F5_,
	ATM_CMD_I_NOTE_G5,
	ATM_CMD_I_NOTE_G5_,
	ATM_CMD_I_NOTE_A5,
	ATM_CMD_I_NOTE_A5_,
	ATM_CMD_I_NOTE_B5,

	ATM_CMD_I_NOTE_C6,
	ATM_CMD_I_NOTE_C6_,
	ATM_CMD_I_NOTE_D6,
	ATM_CMD_I_NOTE_D6_,
	ATM_CMD_I_NOTE_E6,
	ATM_CMD_I_NOTE_F6,
	ATM_CMD_I_NOTE_F6_,
	ATM_CMD_I_NOTE_G6,
	ATM_CMD_I_NOTE_G6_,
	ATM_CMD_I_NOTE_A6,
	ATM_CMD_I_NOTE_A6_,
	ATM_CMD_I_NOTE_B6,

	ATM_CMD_I_NOTE_C7,
	ATM_CMD_I_NOTE_C7_,
	ATM_CMD_I_NOTE_D7,

};

#define ATM_CMD_PNUM(sz) (sz << 4)

/*

 76543210
 0-sscccc s: size, c: cmd
 100-----
 101ddddd d: delay-1
 11nnnnnn n: note idx

*/

enum atm_parametrised_cmd_id_constants {
	ATM_CMD_NP_NOP = 0,
	ATM_CMD_NP_CALL,
	ATM_CMD_NP_ARPEGGIO,
	ATM_CMD_NP_SLIDE,
	ATM_CMD_NP_LFO,
	ATM_CMD_NP_ADD_TO_PARAM,
	ATM_CMD_NP_SET_PARAM,
	ATM_CMD_NP_SET_TEMPO,
	ATM_CMD_NP_ADD_TEMPO,
	ATM_CMD_NP_SET_WAVEFORM,
	ATM_CMD_NP_SET_LOOP_PATTERN,
};

enum atm_parametrised_cmd_constants {
	ATM_CMD_P_CALL              = ATM_CMD_NP_CALL              + ATM_CMD_PNUM(1),
	ATM_CMD_P_RETURN            = ATM_CMD_NP_CALL              + ATM_CMD_PNUM(0),
	ATM_CMD_P_ARPEGGIO_ON       = ATM_CMD_NP_ARPEGGIO          + ATM_CMD_PNUM(2),
	ATM_CMD_P_ARPEGGIO_OFF      = ATM_CMD_NP_ARPEGGIO          + ATM_CMD_PNUM(0),
	ATM_CMD_P_NOTECUT_ON        = ATM_CMD_NP_ARPEGGIO          + ATM_CMD_PNUM(1),
	ATM_CMD_P_SLIDE_ON          = ATM_CMD_NP_SLIDE             + ATM_CMD_PNUM(2),
	ATM_CMD_P_SLIDE_ADV_ON      = ATM_CMD_NP_SLIDE             + ATM_CMD_PNUM(3),
	ATM_CMD_P_SLIDE_OFF         = ATM_CMD_NP_SLIDE             + ATM_CMD_PNUM(1),
	ATM_CMD_P_LFO_ON            = ATM_CMD_NP_LFO               + ATM_CMD_PNUM(3),
	ATM_CMD_P_LFO_OFF           = ATM_CMD_NP_LFO               + ATM_CMD_PNUM(1),
	ATM_CMD_P_PARAM_ADD         = ATM_CMD_NP_ADD_TO_PARAM      + ATM_CMD_PNUM(2),
	ATM_CMD_P_PARAM_SET         = ATM_CMD_NP_SET_PARAM         + ATM_CMD_PNUM(2),
	ATM_CMD_P_TEMPO_SET         = ATM_CMD_NP_SET_TEMPO         + ATM_CMD_PNUM(1),
	ATM_CMD_P_TEMPO_ADD         = ATM_CMD_NP_ADD_TEMPO         + ATM_CMD_PNUM(1),
	ATM_CMD_P_SET_WAVEFORM      = ATM_CMD_NP_SET_WAVEFORM      + ATM_CMD_PNUM(1),
	ATM_CMD_P_SET_LOOP_PATTERN  = ATM_CMD_NP_SET_LOOP_PATTERN  + ATM_CMD_PNUM(1),
};

#define ATM_CMD_I_STOP ATM_CMD_P_RETURN
#define ATM_CMD_I_RETURN ATM_CMD_P_RETURN

#define ATM_CMD_M_NOTE(note) (note)

/* delay <= 32 */
#define ATM_CMD_M_DELAY_TICKS(delay) (ATM_CMD_I_DELAY_1_TICK+((delay)-1))
#define ATM_CMD_M_DELAY_TICKS_1(delay) ATM_CMD_M_DELAY_TICKS(32), ATM_CMD_M_DELAY_TICKS(delay-32)

#define ATM_CMD_M_CALL(pattern_index) ATM_CMD_P_CALL, (pattern_index)
#define ATM_CMD_M_CALL_REPEAT(pattern_index, repeat_count) ATM_CMD_P_CALL, (((repeat_count-1) << 5) | pattern_index)

#define ATM_CMD_M_ARPEGGIO_ON(flags, notes) ATM_CMD_P_ARPEGGIO_ON, (flags), (notes)
#define ATM_CMD_M_NOTECUT_ON(flags) ATM_CMD_P_NOTECUT_ON, ((flags) | 0x40)

#define ATM_CMD_M_SLIDE_ON(flags, amount_per_tick) ATM_CMD_P_SLIDE_ON, (flags), (uint8_t)(amount_per_tick)
#define ATM_CMD_M_SLIDE_ON_ADV(flags, amount_per_interval, interval) ATM_CMD_P_SLIDE_ADV_ON , (flags), (uint8_t)(amount_per_interval), (uint8_t)(interval)
#define ATM_CMD_M_SLIDE_ON_LIMITED(flags, amount_per_tick, limit) ATM_CMD_P_SLIDE_ADV_ON, (flags) | FX_SLIDE_LIMIT_FLAG_MASK, (uint8_t)(amount_per_tick), (uint8_t)(limit)

#define ATM_CMD_M_SLIDE_OFF(flags) ATM_CMD_P_SLIDE_OFF, (flags)

#define ATM_CMD_M_SLIDE_VOL_ON(flags, amount_per_tick) ATM_CMD_M_SLIDE_ON((flags) | ATM_SYNTH_PARAM_VOL, (uint8_t)(amount_per_tick))
#define ATM_CMD_M_SLIDE_MOD_ON(flags, amount_per_tick) ATM_CMD_M_SLIDE_ON((flags) | ATM_SYNTH_PARAM_MOD, (uint8_t)(amount_per_tick))
#define ATM_CMD_M_SLIDE_FREQ_ON(flags, amount_per_tick) ATM_CMD_M_SLIDE_ON((flags) | ATM_SYNTH_PARAM_PHI, (uint8_t)(amount_per_tick))
#define ATM_CMD_M_GLISSANDO_ON(flags, amount_per_tick) ATM_CMD_M_SLIDE_ON((flags) | ATM_SYNTH_PARAM_TSP, (uint8_t)(amount_per_tick))

#define ATM_CMD_M_SLIDE_VOL_ADV_ON(flags, target_value, amount_per_interval) ATM_CMD_M_SLIDE_ON_ADV((flags) | ATM_SYNTH_PARAM_VOL, (uint8_t)(target_value), (uint8_t)(amount_per_interval))
#define ATM_CMD_M_SLIDE_MOD_ADV_ON(flags, target_value, amount_per_interval) ATM_CMD_M_SLIDE_ON_ADV((flags) | ATM_SYNTH_PARAM_MOD, (uint8_t)(target_value), (uint8_t)(amount_per_interval))
#define ATM_CMD_M_SLIDE_FREQ_ADV_ON(flags, target_value, amount_per_interval) ATM_CMD_M_SLIDE_ON_ADV((flags) | ATM_SYNTH_PARAM_PHI, (uint8_t)(target_value), (uint8_t)(amount_per_interval))
#define ATM_CMD_M_GLISSANDO_ADV_ON(flags, target_value, amount_per_interval) ATM_CMD_M_SLIDE_ON_ADV((flags) | ATM_SYNTH_PARAM_TSP, (uint8_t)(target_value), (uint8_t)(amount_per_interval))

#define ATM_CMD_M_SLIDE_VOL_OFF ATM_CMD_M_SLIDE_OFF(ATM_SYNTH_PARAM_VOL)
#define ATM_CMD_M_SLIDE_MOD_OFF ATM_CMD_M_SLIDE_OFF(ATM_SYNTH_PARAM_MOD)
#define ATM_CMD_M_SLIDE_FREQ_OFF ATM_CMD_M_SLIDE_OFF(ATM_SYNTH_PARAM_PHI)
#define ATM_CMD_M_GLISSANDO_OFF ATM_CMD_M_SLIDE_OFF(ATM_SYNTH_PARAM_TSP)

#define ATM_CMD_M_SLIDE_VOL_ADV_OFF ATM_CMD_P_SLIDE_OFF, ATM_SYNTH_PARAM_VOL
#define ATM_CMD_M_SLIDE_MOD_ADV_OFF ATM_CMD_P_SLIDE_OFF, ATM_SYNTH_PARAM_MOD
#define ATM_CMD_M_SLIDE_FREQ_ADV_OFF ATM_CMD_P_SLIDE_OFF, ATM_SYNTH_PARAM_PHI

#define ATM_CMD_M_TREMOLO_ON(flags, depth, rate) ATM_CMD_P_LFO_ON, (flags) | ATM_SYNTH_PARAM_VOL, (depth), (rate)
#define ATM_CMD_M_MOD_LFO_ON(flags, depth, rate) ATM_CMD_P_LFO_ON, (flags) | ATM_SYNTH_PARAM_MOD, (depth), (rate)
#define ATM_CMD_M_VIBRATO_ON(flags, depth, rate) ATM_CMD_P_LFO_ON, (flags) | ATM_SYNTH_PARAM_PHI, (depth), (rate)

#define ATM_CMD_M_TREMOLO_OFF ATM_CMD_P_LFO_OFF, ATM_SYNTH_PARAM_VOL
#define ATM_CMD_M_MOD_LFO_OFF ATM_CMD_P_LFO_OFF, ATM_SYNTH_PARAM_MOD
#define ATM_CMD_M_VIBRATO_OFF ATM_CMD_P_LFO_OFF, ATM_SYNTH_PARAM_PHI

#define ATM_CMD_M_ADD_PARAM(param, delta) ATM_CMD_P_PARAM_ADD, (param), ((uint8_t)delta)
#define ATM_CMD_M_SET_PARAM(param, value) ATM_CMD_P_PARAM_SET, (param), ((uint8_t)value)

#define ATM_CMD_M_ADD_VOLUME(delta) ATM_CMD_M_ADD_PARAM(ATM_SYNTH_PARAM_VOL, (uint8_t)(delta))
#define ATM_CMD_M_SET_VOLUME(value) ATM_CMD_M_SET_PARAM(ATM_SYNTH_PARAM_VOL, (uint8_t)(value))
#define ATM_CMD_M_VOLUME_OFF() ATM_CMD_M_SET_PARAM(ATM_SYNTH_PARAM_VOL, 0)

#define ATM_CMD_M_ADD_MOD(delta) ATM_CMD_M_ADD_PARAM(ATM_SYNTH_PARAM_MOD, (uint8_t)(delta))
#define ATM_CMD_M_SET_MOD(value) ATM_CMD_M_SET_PARAM(ATM_SYNTH_PARAM_MOD, (uint8_t)(value))
#define ATM_CMD_M_MOD_OFF() ATM_CMD_M_SET_PARAM(ATM_SYNTH_PARAM_MOD, 0)

#define ATM_CMD_M_ADD_TRANSPOSITION(delta) ATM_CMD_M_ADD_PARAM(ATM_SYNTH_PARAM_TSP, (uint8_t)(delta))
#define ATM_CMD_M_SET_TRANSPOSITION(value) ATM_CMD_M_SET_PARAM(ATM_SYNTH_PARAM_TSP, (uint8_t)(value))
#define ATM_CMD_M_TRANSPOSITION_OFF() ATM_CMD_M_SET_PARAM(ATM_SYNTH_PARAM_TSP, 0)

#define ATM_CMD_M_SET_TEMPO(p1) ATM_CMD_P_TEMPO_SET, ((uint8_t)p1-1)
#define ATM_CMD_M_ADD_TEMPO(p1) ATM_CMD_P_TEMPO_ADD, ((uint8_t)p1)

#define ATM_CMD_M_SET_WAVEFORM(p1) ATM_CMD_P_SET_WAVEFORM, (p1)
#define ATM_CMD_M_SET_LOOP_PATTERN(p1) ATM_CMD_P_SET_LOOP_PATTERN, (p1)

#endif /* ATM_CMD_CONSTANTS_H */
