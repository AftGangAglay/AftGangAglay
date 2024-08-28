/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_MIDI_H
#define AGA_MIDI_H

#include <aga/environment.h>
#include <aga/result.h>

#ifdef _WIN32
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

enum aga_result aga_midi_device_new(struct aga_midi_device*);
enum aga_result aga_midi_device_delete(struct aga_midi_device*);

enum aga_result aga_midi_new(
		struct aga_midi_device*, struct aga_midi*, void*, aga_size_t);
enum aga_result aga_midi_delete(struct aga_midi_device*, struct aga_midi*);

enum aga_result aga_midi_play(struct aga_midi_device*, struct aga_midi*);

#endif
