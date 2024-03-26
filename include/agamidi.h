/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_MIDI_H
#define AGA_MIDI_H

#include <agaenv.h>
#include <agaresult.h>

#ifdef _WIN32
struct aga_mididev {
	void* dev;
};

struct aga_midi {
	void* hdr;
};
#else
#endif

enum aga_result aga_mkmididev(struct aga_mididev*);
enum aga_result aga_killmididev(struct aga_mididev*);

enum aga_result aga_mkmidi(
		struct aga_mididev*, struct aga_midi*, void*, aga_size_t);
enum aga_result aga_killmidi(struct aga_mididev*, struct aga_midi*);

enum aga_result aga_midi_play(struct aga_mididev*, struct aga_midi*);

#endif
