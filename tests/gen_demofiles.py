#!/usr/bin/env python3

import atmlib_score as score
import atmlib_synth as synth


def write_score(name, score_dict):
	dst_path = name+'.atm'
	s = score.score_from_dict(score_dict)
	with open(dst_path, 'wb+') as f:
		f.write(s.score_bytes())


def coin_sfx():
	return {
		'voice_count': 1,
		'patterns': [
			score.concat_bytes(
				synth.set_tempo(32),
				synth.set_param(synth.FX_PARM.VOL, 127),
				synth.slide(synth.FX_PARM.VOL, -8),
				synth.note('F6'),
				synth.delay(5),
				synth.note('G6'),
				synth.delay(11),
				synth.end_pattern(),
			),
		]
	}


def sfx1_sfx():
	return {
		'voice_count': 1,
		'patterns': [
			score.concat_bytes(
				synth.set_tempo(20),
				synth.set_param(synth.FX_PARM.VOL, 127),
				synth.slide(synth.FX_PARM.VOL, -8),
				synth.slide(synth.FX_PARM.TSP, 1),
				synth.notecut(1),
				synth.note('C5'),
				synth.delay(8),
				synth.end_pattern(),
			),
		]
	}


def sfx2_sfx():
	return {
		'voice_count': 1,
		'patterns': [
			score.concat_bytes(
				synth.set_tempo(48),
				synth.set_param(synth.FX_PARM.VOL, 127),
				synth.slide(synth.FX_PARM.PHI, -48),
				synth.slide(synth.FX_PARM.VOL, -16),
				synth.note('F5'),
				synth.delay(8),
				synth.end_pattern(),
			),
		]
	}


def sfx3_sfx():
	return {
		'voice_count': 1,
		'patterns': [
			score.concat_bytes(
				synth.set_tempo(48),
				synth.set_param(synth.FX_PARM.VOL, 0),
				synth.lfo(synth.FX_PARM.VOL, 127, 0xBF)
				synth.slide(synth.FX_PARM.VOL, 8),
				synth.note('F5'),
				synth.delay(24),
				synth.set_param(synth.FX_PARM.VOL, 127),
				synth.slide(synth.FX_PARM.VOL, -4),
				synth.delay(24),
				synth.end_pattern(),
			),
		]
	}

def sfx4_sfx():
	return {
		'voice_count': 1,
		'patterns': [
			score.concat_bytes(
				synth.set_tempo(48),
				synth.set_param(synth.FX_PARM.VOL, 127),
				synth.note('C2'),
				synth.set_param(synth.FX_PARM.PHI, -127),
				synth.slide(synth.FX_PARM.PHI, -8),
				synth.delay(24),
				synth.delay(24),
				synth.end_pattern(),
			),
		]
	}


funcs = [coin_sfx, sfx1_sfx, sfx2_sfx, sfx3_sfx, sfx4_sfx]


def main():
	for f in funcs:
		write_score(f.__name__, f())


if __name__ == '__main__':
	main()
