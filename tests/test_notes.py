

import subprocess
import unittest
import atmlib_score as score
import atmlib_synth as synth


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
	s = score.score_from_dict(score_dict)
	write_score_file(dst_path, s.score_bytes())
	output = run_test_synth(dst_path)
	with open(str(test_id_str)+'.trace', 'wb+') as f:
		f.write(output)
	return group_trace_values(output.decode().splitlines())


def assert_expected_traces(traces, expected_values):
	for k, v in expected_values.items():
		assert k in traces, '{} not in traces'.format(k)
		assert traces[k] == v, '{}:\nExpected\n{}\nFound\n{}'.format(k, v, traces[k])


class TestNotes(unittest.TestCase):

	# test triggering of other FX

	def test_note1(self):
		"""Simple notes sequence"""
		traces = trace_for_score(self, self.id(), {
			'voice_count': 1,
			'patterns': [
				score.concat_bytes(
					synth.set_tempo(1),
					synth.note(1),
					synth.delay(1),
					synth.note(2),
					synth.delay(1),
					synth.note(0),
					synth.end_pattern(),
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
				score.concat_bytes(
					synth.set_tempo(1),
					synth.note(1),
					synth.delay(1),
					synth.delay(1),
					synth.end_pattern(),
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
				score.concat_bytes(
					synth.delay(1),
					synth.delay(1),
					synth.end_pattern(),
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
				score.concat_bytes(
					synth.set_tempo(1),
					# start a note to make sure the mod field in OSC params is updated
					synth.note(1),
					synth.delay(1),
					synth.delay(1),
					synth.end_pattern(),
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
	def test_transposition_no_note(self):
		traces = trace_for_score(self, self.id(), {
			'voice_count': 1,
			'patterns': [
				score.concat_bytes(
					synth.set_tempo(1),
					synth.set_param(synth.FX_PARM.VOL, 127),
					synth.set_param(synth.FX_PARM.TSP, 3),
					synth.delay(20),
					synth.end_pattern(),
				),
			]
		})
		timestamps = range(0, 20000000, 1000000)
		assert_expected_traces(traces, {
			'osc.channels.0.vol': [(tm, 0) for tm in timestamps],
			'atm.player.0.voice.0.fx.acc_dev.transpose': [(tm, 3) for tm in timestamps]
		})

	def test_set_transposition(self):
		traces = trace_for_score(self, self.id(), {
			'voice_count': 1,
			'patterns': [
				score.concat_bytes(
					synth.set_tempo(1),
					synth.set_param(synth.FX_PARM.VOL, 127),
					synth.note(10),
					synth.delay(5),
					synth.set_param(synth.FX_PARM.TSP, 3),
					synth.delay(5),
					synth.set_param(synth.FX_PARM.TSP, -3),
					synth.delay(5),
					synth.end_pattern(),
				),
			]
		})
		timestamps = range(0, 15000000, 1000000)
		assert_expected_traces(traces, {
			'osc.channels.0.vol': [(tm, 127) for tm in timestamps],
			'atm.player.0.voice.0.fx.acc_dev.note': [(tm, 10) for tm in timestamps],
			'atm.player.0.voice.0.fx.acc_dev.transpose': ([(tm, 0) for tm in timestamps[:5]]+
												[(tm, 3) for tm in timestamps[5:10]]+
												[(tm, -3) for tm in timestamps[10:]]),
			'osc.channels.0.phi': ([(tm, 450) for tm in timestamps[:5]]+
												[(tm, 535) for tm in timestamps[5:10]]+
												[(tm, 378) for tm in timestamps[10:]])
		})

	def test_add_transposition(self):
		traces = trace_for_score(self, self.id(), {
			'voice_count': 1,
			'patterns': [
				score.concat_bytes(
					synth.set_tempo(1),
					synth.set_param(synth.FX_PARM.VOL, 127),
					synth.note(10),
					synth.delay(5),
					synth.addto_param(2, 3),
					synth.delay(5),
					synth.addto_param(2, -6),
					synth.delay(5),
					synth.end_pattern(),
				),
			]
		})
		timestamps = range(0, 15000000, 1000000)
		assert_expected_traces(traces, {
			'osc.channels.0.vol': [(tm, 127) for tm in timestamps],
			'atm.player.0.voice.0.fx.acc_dev.note': [(tm, 10) for tm in timestamps],
			'atm.player.0.voice.0.fx.acc_dev.transpose': ([(tm, 0) for tm in timestamps[:5]]+
												[(tm, 3) for tm in timestamps[5:10]]+
												[(tm, -3) for tm in timestamps[10:]]),
			'osc.channels.0.phi': ([(tm, 450) for tm in timestamps[:5]]+
												[(tm, 535) for tm in timestamps[5:10]]+
												[(tm, 378) for tm in timestamps[10:]])
		})


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
				score.concat_bytes(
					synth.set_tempo(1),
					synth.set_param(synth.FX_PARM.VOL, 127),
					synth.slide(synth.FX_PARM.VOL, -8),
					synth.note(1),
					synth.delay(20),
					synth.end_pattern(),
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
				score.concat_bytes(
					synth.set_tempo(1),
					synth.set_param(synth.FX_PARM.VOL, 0),
					synth.slide(synth.FX_PARM.VOL, 8),
					synth.note(1),
					synth.delay(20),
					synth.end_pattern(),
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
	def _test_param_unit_rate(self, fx_parm, fx_parm_str):
		"""Full period of volume LFO with depth 10, step 1, interval 1 no notes"""
		traces = trace_for_score(self, self.id(), {
			'voice_count': 1,
			'patterns': [
				score.concat_bytes(
					synth.set_tempo(1),
					synth.lfo(fx_parm, 10, 0x00),
					synth.delay(41),
					synth.end_pattern(),
				),
			]
		})
		assert_expected_traces(traces, {
			# 0 -> 10 -> 0 -> -10 -> 0 (in steps of 1)
			'atm.player.0.voice.0.fx.lfo.'+fx_parm_str: [(i*int(1E6), i) for i in range(0, 11)]+
												[(i*int(1E6), 20-i) for i in range(11, 31)]+
												[(i*int(1E6), i-40) for i in range(31, 41)],
			# no note active so LFO FX does not make it to OSC params
			'osc.channels.0.vol': [(int(tm*1E6), 0) for tm in range(0, 41)]
		})

	def test_vol_unit_rate(self):
		"""Full period of volume LFO with depth 10, step 1, interval 1 no notes"""
		self._test_param_unit_rate(synth.FX_PARM.VOL, 'vol')

	def test_mod_unit_rate(self):
		"""Full period of mod LFO with depth 10, step 1, interval 1 no notes"""
		self._test_param_unit_rate(synth.FX_PARM.MOD, 'mod')

	def test_phi_unit_rate(self):
		"""Full period of phi LFO with depth 10, step 1, interval 1 no notes"""
		self._test_param_unit_rate(synth.FX_PARM.PHI, 'phi')

	def test_tsp_unit_rate(self):
		"""Full period of tsp LFO with depth 10, step 1, interval 1 no notes"""
		self._test_param_unit_rate(synth.FX_PARM.TSP, 'tsp')

	def _test_param_with_note(self, fx_parm, fx_parm_str):
		"""Full period of volume LFO with depth 10, step 1, interval 1 note active"""
		traces = trace_for_score(self, self.id(), {
			'voice_count': 1,
			'patterns': [
				score.concat_bytes(
					synth.set_tempo(1),
					synth.note(20),
					synth.lfo(fx_parm, 10, 0x00),
					synth.delay(41),
					synth.end_pattern(),
				),
			]
		})
		expected_parm = ([(i*int(1E6), i) for i in range(0, 11)]+
						[(i*int(1E6), 20-i) for i in range(11, 31)]+
						[(i*int(1E6), i-40) for i in range(31, 41)])
		assert_expected_traces(traces, {
			# 0 -> 10 -> 0 -> -10 -> 0 (in steps of 1)
			'atm.player.0.voice.0.fx.lfo.'+fx_parm_str: expected_parm,
		})
		if fx_parm == synth.FX_PARM.VOL:
			assert_expected_traces(traces, {
				# OSC volume must be >= 0 so we expect zeros when LFO is negative
				'osc.channels.0.vol': [(t, v if v >= 0 else 0) for t,v in expected_parm]
			})
		elif fx_parm == synth.FX_PARM.TSP:
			assert_expected_traces(traces, {
				# OSC volume must be >= 0 so we expect zeros when LFO is negative
				'atm.player.0.voice.0.fx.acc_dev.transpose': expected_parm
			})
		elif fx_parm == synth.FX_PARM.PHI:
			assert_expected_traces(traces, {
				# OSC volume must be >= 0 so we expect zeros when LFO is negative
				'osc.channels.0.phi': [(t, v+802) for t,v in expected_parm]
			})
		else:
			assert_expected_traces(traces, {
				# OSC volume must be >= 0 so we expect zeros when LFO is negative
				'osc.channels.0.'+fx_parm_str: [(t, v) for t,v in expected_parm]
			})

	def test_vol_with_note(self):
		"""Full period of volume LFO with depth 10, step 1, interval 1 note active"""
		self._test_param_with_note(synth.FX_PARM.VOL, 'vol')

	def test_mod_with_note(self):
		"""Full period of mod LFO with depth 10, step 1, interval 1 note active"""
		self._test_param_with_note(synth.FX_PARM.MOD, 'mod')

	def test_phi_with_note(self):
		"""Full period of phi LFO with depth 10, step 1, interval 1 note active"""
		self._test_param_with_note(synth.FX_PARM.PHI, 'phi')

	def test_tsp_with_note(self):
		"""Full period of tsp LFO with depth 10, step 1, interval 1 note active"""
		self._test_param_with_note(synth.FX_PARM.TSP, 'tsp')

	def _test_parm_non_unit_rate(self, fx_parm, fx_parm_str):
		"""Full period of volume LFO with depth 10, step 4, interval 4"""
		traces = trace_for_score(self, self.id(), {
			'voice_count': 1,
			'patterns': [
				score.concat_bytes(
					synth.set_tempo(1),
					synth.lfo(fx_parm, 10, 0x33),
					synth.delay(41),
					synth.end_pattern(),
				),
			]
		})
		expected_vol = (
			[0]*4 + [4]*4 + [8]*8 + [4]*4 +
			[0]*4 + [-4]*4 + [-8]*8 + [-4]*4 +
			[0])
		assert_expected_traces(traces, {
			'atm.player.0.voice.0.fx.lfo.'+fx_parm_str: [(i*int(1E6), v) for i, v in enumerate(expected_vol)],
		})


	def test_vol_non_unit_rate(self):
		"""Full period of volume LFO with depth 10, step 4, interval 4"""
		self._test_parm_non_unit_rate(synth.FX_PARM.VOL, 'vol')

	def test_mod_non_unit_rate(self):
		"""Full period of mod LFO with depth 10, step 4, interval 4"""
		self._test_parm_non_unit_rate(synth.FX_PARM.MOD, 'mod')

	def test_phi_non_unit_rate(self):
		"""Full period of phi LFO with depth 10, step 4, interval 4"""
		self._test_parm_non_unit_rate(synth.FX_PARM.PHI, 'phi')

	def test_tsp_non_unit_rate(self):
		"""Full period of tsp LFO with depth 10, step 4, interval 4"""
		self._test_parm_non_unit_rate(synth.FX_PARM.TSP, 'tsp')


class TestArpeggioFX(unittest.TestCase):

	# test 2 notes arpeggio
	def test_two_notes_arpeggio(self):
		"""Test arpeggio with 2 notes and with minimal interval"""
		traces = trace_for_score(self, self.id(), {
			'voice_count': 1,
			'patterns': [
				score.concat_bytes(
					synth.set_tempo(1),
					synth.set_param(synth.FX_PARM.VOL, 127),
					synth.slide(synth.FX_PARM.VOL, -8),
					synth.arp(1, 2, 0x37),
					synth.note(1),
					synth.delay(8),
					synth.end_pattern(),
				),
			]
		})
		assert_expected_traces(traces, {
			# fx.acc_dev.note alternates between 0 and 1 every tick
			'atm.player.0.voice.0.fx.acc_dev.note': [(int(tm*1E6), 1) for tm in range(0, 8)],
			# fx.arp.transpose is 0 every other tick
			'atm.player.0.voice.0.fx.arp.transpose': [(int(tm*1E6), t) for tm, t in zip(range(0, 8), [0,3,0,3,0,3,0,3])],
			# fx.arp.notecut event occurs every other tick
			'atm.player.0.voice.0.fx.arp.tsp.update': [(int(tm*1E6), 'e') for tm in range(1, 8)],
			'atm.player.0.voice.0.fx.arp.tsp.triggers': [(int(tm*1E6), 'e') for tm in range(0, 8)],
			'atm.player.0.voice.0.fx.arp.tsp.process': [(int(tm*1E6), 'e') for tm in range(0, 8)],
		})
	# test 3 notes arpeggio
	def test_three_notes_arpeggio(self):
		"""Test arpeggio with 2 notes and with minimal interval"""
		traces = trace_for_score(self, self.id(), {
			'voice_count': 1,
			'patterns': [
				score.concat_bytes(
					synth.set_tempo(1),
					synth.set_param(synth.FX_PARM.VOL, 127),
					synth.slide(synth.FX_PARM.VOL, -8),
					synth.arp(1, 3, 0x37),
					synth.note(1),
					synth.delay(8),
					synth.end_pattern(),
				),
			]
		})
		assert_expected_traces(traces, {
			# fx.acc_dev.note alternates between 0 and 1 every tick
			'atm.player.0.voice.0.fx.acc_dev.note': [(int(tm*1E6), 1) for tm in range(0, 8)],
			# fx.arp.transpose is 0 every other tick
			'atm.player.0.voice.0.fx.arp.transpose': [(int(tm*1E6), t) for tm, t in zip(range(0, 8), [0,3,7,0,3,7,0,3])],
			# fx.arp.notecut event occurs every other tick
			'atm.player.0.voice.0.fx.arp.tsp.update': [(int(tm*1E6), 'e') for tm in range(1, 8)],
			'atm.player.0.voice.0.fx.arp.tsp.triggers': [(int(tm*1E6), 'e') for tm in range(0, 8)],
			'atm.player.0.voice.0.fx.arp.tsp.process': [(int(tm*1E6), 'e') for tm in range(0, 8)],
		})
	# test note cut
	def test_notecut(self):
		"""Test notecut with minimal interval"""
		traces = trace_for_score(self, self.id(), {
			'voice_count': 1,
			'patterns': [
				score.concat_bytes(
					synth.set_tempo(1),
					synth.set_param(synth.FX_PARM.VOL, 127),
					synth.slide(synth.FX_PARM.VOL, -8),
					synth.notecut(1),
					synth.note(1),
					synth.delay(8),
					synth.end_pattern(),
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
			'atm.player.0.voice.0.fx.arp.tsp.update': [(int(tm*1E6), 'e') for tm in range(1, 8)],
			'atm.player.0.voice.0.fx.arp.tsp.triggers': [(int(tm*1E6), 'e') for tm in range(0, 8)],
			'atm.player.0.voice.0.fx.arp.tsp.process': [(int(tm*1E6), 'e') for tm in range(0, 8)],
		})
	# test tick > 1 arpeggio
	# test auto retrigger
	# test hold
	# test triggering of other FX
