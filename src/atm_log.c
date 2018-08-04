
#include "atm_log.h"

uint8_t atm_log_mask = LOG_TM | LOG_INFO | LOG_ERR;
uint64_t atm_log_timestamp = 0;
uint8_t log_current_voice_index_ = 0;
uint8_t log_current_player_index_ = 0;

const char *cmd_short_labels[] = {
	"nop",
	"call",
	"arp",
	"slide",
	"trns",
	"vol",
	"tempo",
	"lfo",
	"glissando",
	"mod",
	"wav",
	"loop",
};

const char *cmd_fx_dest_short_labels[] = {
	"vol",
	"mod",
	"phi",
	"tsp",
};
