
import struct
from itertools import chain, accumulate


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
