
#ifndef _ATM_LOG_H
#define _ATM_LOG_H

#define LOG_TM      (1 << 0)
#define LOG_ERR     (1 << 3)
#define LOG_WARNING (1 << 4)
#define LOG_NOTICE  (1 << 5)
#define LOG_INFO    (1 << 6)
#define LOG_DEBUG   (1 << 7)

#ifdef __AVR__

	#include <stdint.h>
	#define atm_log(...)
	#define atm_log_event(...)
	#define atm_current_voice_index()
	#define atm_current_player_index()
	#define atm_set_log_current_voice_index(index)
	#define atm_set_log_current_player_index(index)
	#define atm_log_cmd_label(id)
	#define atm_log_fx_dest_label(id)

#else

	#include <stdio.h>
	#include <stdint.h>
	extern uint8_t log_current_voice_index_;
	extern uint8_t log_current_player_index_;
	extern uint8_t atm_log_mask;
	extern uint64_t atm_log_timestamp;
	#define atm_current_voice_index() (log_current_voice_index_)
	#define atm_current_player_index() (log_current_player_index_)
	#define atm_set_log_current_voice_index(index) log_current_voice_index_ = (index)
	#define atm_set_log_current_player_index(index) log_current_player_index_ = (index)
	#define atm_log(l, fmt, ...) {if ((l) & atm_log_mask) fprintf(stderr, fmt "\n", ##__VA_ARGS__);}
	#define atm_log_event(varname, value_fmt, ...) {if (atm_log_mask & LOG_TM) fprintf(stderr, "#%llu " varname " " value_fmt "\n", atm_log_timestamp, ##__VA_ARGS__);}

	#define atm_log_cmd_label(id) cmd_short_labels[(id) & 0x0F]
	extern const char *cmd_short_labels[];

	#define atm_log_fx_dest_label(id) cmd_fx_dest_short_labels[(id) & 0x03]
	extern const char *cmd_fx_dest_short_labels[];

#endif

#endif /* _ATM_LOG_H */
