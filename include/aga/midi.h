/*
 * SPDX-License-Identifier: X11
 * Copyright (C) 2025 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_MIDI_H
#define AGA_MIDI_H

#include <asys/base.h>
#include <asys/result.h>

#ifdef ASYS_WIN32
struct aga_midi_device {
	void* dev;
};

struct aga_midi {
	void* hdr;
};
#else
struct aga_midi_device {
    char dummy;
};

struct aga_midi {
    char dummy;
};
#endif

enum asys_result aga_midi_device_new(struct aga_midi_device*);
enum asys_result aga_midi_device_delete(struct aga_midi_device*);

enum asys_result aga_midi_new(
		struct aga_midi_device*, struct aga_midi*, void*, asys_size_t);

enum asys_result aga_midi_delete(struct aga_midi_device*, struct aga_midi*);

enum asys_result aga_midi_play(struct aga_midi_device*, struct aga_midi*);

#endif
