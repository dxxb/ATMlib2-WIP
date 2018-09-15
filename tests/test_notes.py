

import subprocess
import unittest
import atmlib_score


def write_score_file(dst_path, score_bytes):
	with open(dst_path, 'wb+') as f:
		f.write(score_bytes)


def run_test_synth(score_path):
	return subprocess.check_output('./test_synth {}'.format(score_path), stderr=subprocess.STDOUT, shell=True)


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
	with open(str(test_id_str)+'.trace', 'wb+') as f:
		f.write(output)
	return group_trace_values(output.decode().splitlines())


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
			'osc.tick': [(int(1E6*tm), 'e') for tm in range(0, 81)],
			# one player tick every 40 milliseconds for 80 ms (2 ticks)
			'atm.player.0.tick': [(int(tm*1E6), 'e') for tm in range(0, 81, 40)]
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
	def test_slide_down(self):
		"""test slide down"""
		traces = trace_for_score(self, self.id(), {
			'voice_count': 1,
			'patterns': [
				atmlib_score.concat_bytes(
					atmlib_score.set_tempo(1),
					atmlib_score.set_param(0, 127),
					atmlib_score.slide(0, -8),
					atmlib_score.note(1),
					atmlib_score.delay(20),
					atmlib_score.end_pattern(),
				),
			]
		})
		expected_values = [
			(1000000, -8),
			(2000000, -16),
			(3000000, -24),
			(4000000, -32),
			(5000000, -40),
			(6000000, -48),
			(7000000, -56),
			(8000000, -64),
			(9000000, -72),
			(10000000, -80),
			(11000000, -88),
			(12000000, -96),
			(13000000, -104),
			(14000000, -112),
			(15000000, -120),
			(16000000, -127),
			(17000000, -127),
			(18000000, -127),
			(19000000, -127),
		]
		assert_expected_traces(traces, {
			'atm.player.0.voice.0.fx.slide.vol': expected_values,
			'osc.channels.0.vol': [(0, 127)]+[(tm, 127+val) for tm, val in expected_values],
		})

	def test_slide_up(self):
		"""test slide up"""
		traces = trace_for_score(self, self.id(), {
			'voice_count': 1,
			'patterns': [
				atmlib_score.concat_bytes(
					atmlib_score.set_tempo(1),
					atmlib_score.set_param(0, 0),
					atmlib_score.slide(0, 8),
					atmlib_score.note(1),
					atmlib_score.delay(20),
					atmlib_score.end_pattern(),
				),
			]
		})
		expected_values = [
			(1000000, 8),
			(2000000, 16),
			(3000000, 24),
			(4000000, 32),
			(5000000, 40),
			(6000000, 48),
			(7000000, 56),
			(8000000, 64),
			(9000000, 72),
			(10000000, 80),
			(11000000, 88),
			(12000000, 96),
			(13000000, 104),
			(14000000, 112),
			(15000000, 120),
			(16000000, 127),
			(17000000, 127),
			(18000000, 127),
			(19000000, 127),
		]
		assert_expected_traces(traces, {
			'atm.player.0.voice.0.fx.slide.vol': expected_values,
			'osc.channels.0.vol': [(0, 0)]+expected_values,
		})


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
			'atm.player.0.voice.0.fx.lfo.vol': [(i*int(1E6), i) for i in range(0, 11)]+
												[(i*int(1E6), 20-i) for i in range(11, 31)]+
												[(i*int(1E6), i-40) for i in range(31, 41)],
			# no note active so LFO FX does not make it to OSC params
			'osc.channels.0.vol': [(int(tm*1E6), 0) for tm in range(0, 41)]
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
		expected_vol = ([(i*int(1E6), i) for i in range(0, 11)]+
						[(i*int(1E6), 20-i) for i in range(11, 31)]+
						[(i*int(1E6), i-40) for i in range(31, 41)])
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
			(3000000, 0),
			(4000000, 4),
			(5000000, 4),
			(6000000, 4),
			(7000000, 4),
			(8000000, 8),
			(9000000, 8),
			(10000000, 8),
			(11000000, 8),
			(12000000, 8),
			(13000000, 8),
			(14000000, 8),
			(15000000, 8),
			(16000000, 4),
			(17000000, 4),
			(18000000, 4),
			(19000000, 4),
			(20000000, 0),
			(21000000, 0),
			(22000000, 0),
			(23000000, 0),
			(24000000, -4),
			(25000000, -4),
			(26000000, -4),
			(27000000, -4),
			(28000000, -8),
			(29000000, -8),
			(30000000, -8),
			(31000000, -8),
			(32000000, -8),
			(33000000, -8),
			(34000000, -8),
			(35000000, -8),
			(36000000, -4),
			(37000000, -4),
			(38000000, -4),
			(39000000, -4),
			(40000000, 0)]
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
	def test_notecut(self):
		"""Test notecut with minimal interval"""
		traces = trace_for_score(self, self.id(), {
			'voice_count': 1,
			'patterns': [
				atmlib_score.concat_bytes(
					atmlib_score.set_tempo(1),
					atmlib_score.set_param(0, 127),
					atmlib_score.slide(0, -8),
					atmlib_score.notecut(1),
					atmlib_score.note(1),
					atmlib_score.delay(8),
					atmlib_score.end_pattern(),
				),
			]
		})
		assert_expected_traces(traces, {
			# fx.acc_dev.note alternates between 0 and 1 every tick
			'atm.player.0.voice.0.fx.acc_dev.note': [(int(tm*1E6), (tm+1) % 2) for tm in range(0, 8)],
			# fx.arp.transpose is 0 every other tick
			'atm.player.0.voice.0.fx.arp.transpose': [(int(tm*1E6), 0) for tm in range(0, 8, 2)],
			# fx.arp.notecut event occurs every other tick
			'atm.player.0.voice.0.fx.arp.notecut': [(int(tm*1E6), 'e') for tm in range(1, 8, 2)],
			'atm.player.0.voice.0.fx.arp.vol.update': [(int(tm*1E6), 'e') for tm in range(1, 8)],
			'atm.player.0.voice.0.fx.arp.vol.triggers': [(int(tm*1E6), 'e') for tm in range(0, 8)],
		})
	# test 1 tick arpeggio
	# test auto retrigger
	# test hold
	# test triggering of other FX
	def test_sfx(self):
		traces = trace_for_score(self, self.id(), {
			'voice_count': 1,
			'patterns': [
				atmlib_score.concat_bytes(
					atmlib_score.set_tempo(32),
					atmlib_score.set_param(0, 127),
					atmlib_score.slide(0, -8),
					atmlib_score.note(54),
					atmlib_score.delay(5),
					atmlib_score.note(56),
					atmlib_score.delay(11),
					atmlib_score.end_pattern(),
				),
			]
		})
