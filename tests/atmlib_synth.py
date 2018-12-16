
import struct


class CMDS:
	NOP = 0
	CALL = 1
	ARPEGGIO = 2
	SLIDE = 3
	LFO = 4
	ADDTO_PARAM = 5
	SET_PARAM = 6
	SET_TEMPO = 7
	ADD_TEMPO = 8
	SET_WAVEFORM = 9
	SET_LOOP_PATTERN = 10


class FX_PARM:
	VOL = 0
	MOD = 1
	TSP = 2
	PHI = 3


note2idx = {
	'C': 0,
	'D': 2,
	'E': 4,
	'F': 5,
	'G': 7,
	'A': 9,
	'B': 11,
}


def noteindex_by_name(note_str):
	note_str = note_str.upper()
	assert len(note_str) in (2, 3), 'Cannot parse note name `{}`'.format(note_str)
	octave = int(note_str[1])
	index = note2idx[note_str[0]] + (octave-2)*12
	if len(note_str) > 2:
		if note_str[2] in ('b'):
			index -=1
		if note_str[2] in ('_', '#'):
			index +=1
	return index+1


def note_off():
	return bytes([0xC0])


def note(index_or_string):
	try:
		index = int(index_or_string)
	except ValueError:
		index = noteindex_by_name(index_or_string)
	assert(0 <= index < 64)
	return bytes([0xC0+index])


def delay(ticks):
	q = ticks // 32
	r = (ticks % 32) - 1
	res = [0xA0+31]*q
	if r >= 0:
		res += [0xA0+r]
	return bytes(res)


def mkcmd(cmd_id, size):
	return cmd_id | (size << 4)


def end_pattern():
	return bytes([mkcmd(CMDS.CALL, 0)])


def slide(param_id, amount_per_interval, interval_ticks=1):
	assert interval_ticks >= 1
	if interval_ticks == 1:
		return struct.pack('BBb', mkcmd(CMDS.SLIDE, 2), param_id, amount_per_interval)
	else:
		return struct.pack('BBbB', mkcmd(CMDS.SLIDE, 3), param_id, amount_per_interval, interval_ticks-1)


def limited_slide(param_id, amount_per_tick, limit):
	return struct.pack('BBbB', mkcmd(CMDS.SLIDE, 3) | FX_SLIDE_LIMIT_FLAG, param_id, amount_per_tick, limit)


def lfo(param_id, depth, rate):
	return struct.pack('BBBB', mkcmd(CMDS.LFO, 3), param_id, depth, rate)


def notecut(interval_ticks):
	assert interval_ticks >= 1
	return struct.pack('BB', mkcmd(CMDS.ARPEGGIO, 1), 0x40 | ((interval_ticks-1) & 0x1F))


def arp(interval_ticks, notes_count, notes):
	assert interval_ticks >= 1
	arp_cmp = (notes_count)*0x20
	return struct.pack('BBB', mkcmd(CMDS.ARPEGGIO, 2), arp_cmp | ((interval_ticks-1) & 0x1F), notes)


def set_param(param_id, value):
	return struct.pack('BBb', mkcmd(CMDS.SET_PARAM, 2), param_id, value)


def addto_param(param_id, value):
	return struct.pack('BBb', mkcmd(CMDS.ADDTO_PARAM, 2), param_id, value)


def set_tempo(ticks):
	return struct.pack('BB', mkcmd(CMDS.SET_TEMPO, 1), ticks-1)


def add_tempo(ticks):
	return struct.pack('Bb', mkcmd(CMDS.ADD_TEMPO, 1), ticks)
