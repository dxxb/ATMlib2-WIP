
import struct
from itertools import chain, accumulate


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


def note_off():
	return bytes([0xC0])


def note(index):
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


def set_param(param_id, value):
	return struct.pack('BBb', mkcmd(CMDS.SET_PARAM, 2), param_id, value)


def addto_param(param_id, value):
	return struct.pack('BBb', mkcmd(CMDS.ADDTO_PARAM, 2), param_id, value)


def set_tempo(ticks):
	return struct.pack('BB', mkcmd(CMDS.SET_TEMPO, 1), ticks-1)


def add_tempo(ticks):
	return struct.pack('Bb', mkcmd(CMDS.ADD_TEMPO, 1), ticks)


def concat_bytes(*insn):
	return b''.join(chain(insn))


class Score:

	def __init__(self, voice_count, pattern_count):
		self.voice_count = voice_count
		self.pattern_count = pattern_count
		assert(pattern_count >= voice_count)
		self.patterns = [None]*pattern_count

	def set_pattern_data(self, index, data):
		self.patterns[index] = data

	def _gen_score(self):
		yield struct.pack('BB', self.voice_count, self.pattern_count)
		pattern_sizes = list(map(len, self.patterns))[:-1]
		for offset in accumulate([2+self.pattern_count*2]+pattern_sizes):
			yield struct.pack('H', offset)
		yield b''.join(self.patterns)

	def score_bytes(self):
		assert(all(map(lambda x: x is not None, self.patterns)))
		return b''.join(self._gen_score())


def score_from_dict(dictionary):
	patterns = dictionary['patterns']
	s = Score(dictionary['voice_count'], len(patterns))
	for i, p in enumerate(patterns):
		s.set_pattern_data(i, p)
	return s
