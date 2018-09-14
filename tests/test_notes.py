

import subprocess
import unittest
import atmlib_score


def write_score_file(dst_path, score_bytes):
	with open(dst_path, 'wb+') as f:
		f.write(score_bytes)


def run_test_synth(score_path):
	res = subprocess.check_output('./test_synth {}'.format(score_path), stderr=subprocess.STDOUT, shell=True)
	return res.decode().splitlines()


def group_trace_values(lines):
	groups = {}
	for line in lines:
		# Discard lines not starting with '#'
		if not line.startswith('#'):
			continue
		toks = line[1:].split()
		timestamp, id_str, val, *rest = toks
		if val != 'e':
			val = int(val)
		groups.setdefault(id_str, []).append((int(timestamp), val))
	return groups


def trace_for_score(self, test_id_str, score_dict):
	dst_path = str(test_id_str)+'.atm'
	score = atmlib_score.score_from_dict(score_dict)
	write_score_file(dst_path, score.score_bytes())
	output = run_test_synth(dst_path)
	return group_trace_values(output)


def assert_expected_traces(traces, expected_values):
	for k, v in expected_values.items():
		assert k in traces, '{} not in traces'.format(k)
		assert traces[k] == v, '{}: expected {}, found {}'.format(k, v, traces[k])


class TestNotes(unittest.TestCase):

	# test triggering of other FX

	def test_note1(self):
		"""Simple notes sequence"""
		traces = trace_for_score(self, self.id(), {
			'voice_count': 1,
			'patterns': [
				atmlib_score.concat_bytes(
					atmlib_score.set_tempo(1),
					atmlib_score.note(1),
					atmlib_score.delay(1),
					atmlib_score.note(2),
					atmlib_score.delay(1),
					atmlib_score.note(0),
					atmlib_score.end_pattern(),
				),
			]
		})
		assert_expected_traces(traces, {
			'osc.tick': [(0, 'e'), (1000000, 'e'), (2000000, 'e')],
			'atm.player.0.tick': [(0, 'e'), (1000000, 'e'), (2000000, 'e')],
			'atm.player.0.voice.0.tempo': [(0, 0)],
			'atm.player.0.voice.0.note': [(0, 1), (1000000, 2), (2000000, 0)],
			'atm.player.0.voice.0.stop': [(2000000, 'e')],
			'atm.player.0.stop': [(2000000, 'e')],
		})


class TestDefaults(unittest.TestCase):

	def test_default_volume(self):
		"""Volume should default to 0 at song start"""
		traces = trace_for_score(self, self.id(), {
			'voice_count': 1,
			'patterns': [
				atmlib_score.concat_bytes(
					atmlib_score.set_tempo(1),
					atmlib_score.note(1),
					atmlib_score.delay(1),
					atmlib_score.delay(1),
					atmlib_score.end_pattern(),
				),
			]
		})
		assert_expected_traces(traces, {
			'osc.channels.0.vol': [(0, 0), (1000000, 0)],
		})

	def test_default_tempo(self):
		"""Tempo should default to 40 at song start"""
		traces = trace_for_score(self, self.id(), {
			'voice_count': 1,
			'patterns': [
				atmlib_score.concat_bytes(
					atmlib_score.delay(1),
					atmlib_score.delay(1),
					atmlib_score.end_pattern(),
				),
			]
		})
		assert_expected_traces(traces, {
			# one OSC tick every millisecond for 80 ms (80 ticks)
			'osc.tick': [(tm, 'e') for tm in range(0, int(81*1E6), int(1E6))],
			# one player tick every 40 milliseconds for 80 ms (2 ticks)
			'atm.player.0.tick': [(tm, 'e') for tm in range(0, int(81*1E6), int(40*1E6))]
		})

	def test_default_mod(self):
		"""Square duty cycle should default to 0 at song start"""
		traces = trace_for_score(self, self.id(), {
			'voice_count': 1,
			'patterns': [
				atmlib_score.concat_bytes(
					atmlib_score.set_tempo(1),
					# start a note to make sure the mod field in OSC params is updated
					atmlib_score.note(1),
					atmlib_score.delay(1),
					atmlib_score.delay(1),
					atmlib_score.end_pattern(),
				),
			]
		})
		assert_expected_traces(traces, {
			'osc.channels.0.mod': [(0, 0), (1000000, 0)],
		})


class TestTempo(unittest.TestCase):
	# test set tempo
	# test add tempo
	pass


class TestTransposition(unittest.TestCase):
	# test set trasnposition
	# test add transposition
	pass


class TestVolume(unittest.TestCase):
	# test set volume
	# test add volume
	pass


class TestVolume(unittest.TestCase):
	# test set volume
	# test add volume
	pass


class TestSlideFX(unittest.TestCase):
	# simple slide up
	# simple slide down
	# slide up with interval
	# slide down with interval
	# limited slide up
	# limited slide down
	# test re-triggering behaviour
	pass


class TestLFOFX(unittest.TestCase):
	def test_vol_unit_rate(self):
		"""Full period of volume LFO with depth 10, step 1, interval 1 no notes"""
		traces = trace_for_score(self, self.id(), {
			'voice_count': 1,
			'patterns': [
				atmlib_score.concat_bytes(
					atmlib_score.set_tempo(1),
					atmlib_score.lfo(0, 10, 0x00),
					atmlib_score.delay(41),
					atmlib_score.end_pattern(),
				),
			]
		})
		assert_expected_traces(traces, {
			# 0 -> 10 -> 0 -> -10 -> 0 (in steps of 1)
			'atm.player.0.voice.0.fx.lfo.vol': [
				(0, 1),
				(1000000, 2),
				(2000000, 3),
				(3000000, 4),
				(4000000, 5),
				(5000000, 6),
				(6000000, 7),
				(7000000, 8),
				(8000000, 9),
				(9000000, 10),
				(10000000, 9),
				(11000000, 8),
				(12000000, 7),
				(13000000, 6),
				(14000000, 5),
				(15000000, 4),
				(16000000, 3),
				(17000000, 2),
				(18000000, 1),
				(19000000, 0),
				(20000000, -1),
				(21000000, -2),
				(22000000, -3),
				(23000000, -4),
				(24000000, -5),
				(25000000, -6),
				(26000000, -7),
				(27000000, -8),
				(28000000, -9),
				(29000000, -10),
				(30000000, -9),
				(31000000, -8),
				(32000000, -7),
				(33000000, -6),
				(34000000, -5),
				(35000000, -4),
				(36000000, -3),
				(37000000, -2),
				(38000000, -1),
				(39000000, 0),
				(40000000, 1),
			],
			# no note active so LFO FX does not make it to OSC params
			'osc.channels.0.vol': [(tm, 0) for tm in range(0, int(41*1E6), int(1E6))]
		})

	def test_vol_with_note(self):
		"""Full period of volume LFO with depth 10, step 1, interval 1 note active"""
		traces = trace_for_score(self, self.id(), {
			'voice_count': 1,
			'patterns': [
				atmlib_score.concat_bytes(
					atmlib_score.set_tempo(1),
					atmlib_score.note(1),
					atmlib_score.lfo(0, 10, 0x00),
					atmlib_score.delay(41),
					atmlib_score.end_pattern(),
				),
			]
		})
		expected_vol = [
			(0, 1),
			(1000000, 2),
			(2000000, 3),
			(3000000, 4),
			(4000000, 5),
			(5000000, 6),
			(6000000, 7),
			(7000000, 8),
			(8000000, 9),
			(9000000, 10),
			(10000000, 9),
			(11000000, 8),
			(12000000, 7),
			(13000000, 6),
			(14000000, 5),
			(15000000, 4),
			(16000000, 3),
			(17000000, 2),
			(18000000, 1),
			(19000000, 0),
			(20000000, -1),
			(21000000, -2),
			(22000000, -3),
			(23000000, -4),
			(24000000, -5),
			(25000000, -6),
			(26000000, -7),
			(27000000, -8),
			(28000000, -9),
			(29000000, -10),
			(30000000, -9),
			(31000000, -8),
			(32000000, -7),
			(33000000, -6),
			(34000000, -5),
			(35000000, -4),
			(36000000, -3),
			(37000000, -2),
			(38000000, -1),
			(39000000, 0),
			(40000000, 1),
			]
		assert_expected_traces(traces, {
			# 0 -> 10 -> 0 -> -10 -> 0 (in steps of 1)
			'atm.player.0.voice.0.fx.lfo.vol': expected_vol,
			# OSC volume must be >= 0 so we expec zeros when LFO is negative
			'osc.channels.0.vol': [(t, v if v >= 0 else 0) for t,v in expected_vol]
		})

	def test_vol_non_unit_rate(self):
		"""Full period of volume LFO with depth 10, step 4, interval 4"""
		traces = trace_for_score(self, self.id(), {
			'voice_count': 1,
			'patterns': [
				atmlib_score.concat_bytes(
					atmlib_score.set_tempo(1),
					atmlib_score.lfo(0, 10, 0x33),
					atmlib_score.delay(41),
					atmlib_score.end_pattern(),
				),
			]
		})
		expected_vol = [
			(0, 0),
			(1000000, 0),
			(2000000, 0),
			(3000000, 4),
			(4000000, 4),
			(5000000, 4),
			(6000000, 4),
			(7000000, 8),
			(8000000, 8),
			(9000000, 8),
			(10000000, 8),
			(11000000, 8),
			(12000000, 8),
			(13000000, 8),
			(14000000, 8),
			(15000000, 4),
			(16000000, 4),
			(17000000, 4),
			(18000000, 4),
			(19000000, 0),
			(20000000, 0),
			(21000000, 0),
			(22000000, 0),
			(23000000, -4),
			(24000000, -4),
			(25000000, -4),
			(26000000, -4),
			(27000000, -8),
			(28000000, -8),
			(29000000, -8),
			(30000000, -8),
			(31000000, -8),
			(32000000, -8),
			(33000000, -8),
			(34000000, -8),
			(35000000, -4),
			(36000000, -4),
			(37000000, -4),
			(38000000, -4),
			(39000000, 0),
			(40000000, 0),
		]
		assert_expected_traces(traces, {
			'atm.player.0.voice.0.fx.lfo.vol': expected_vol,
		})

	# test LFO depth
	# test LFO rate
	pass


class TestArpeggioFX(unittest.TestCase):
	# test 2 notes arpeggio
	# test 3 notes arpeggio
	# test note cut
	# test 1 tick arpeggio
	# test auto retrigger
	# test hold
	# test triggering of other FX
	pass
