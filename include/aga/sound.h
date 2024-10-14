/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_SOUND_H
#define AGA_SOUND_H

#include <aga/environment.h>
#include <aga/result.h>

enum aga_sound_settings {
	AGA_SOUND_SAMPLE_RATE = 8000,
	AGA_SOUND_CHANNELS = 1,
	AGA_SOUND_SAMPLE_BITS = 8,
	AGA_SOUND_BLOCK_COUNT = 200,
	AGA_SOUND_BLOCK_SIZE = AGA_SOUND_SAMPLE_RATE / AGA_SOUND_BLOCK_COUNT
};

struct aga_sound_device {
	int fd;
	aga_uchar_t buf[AGA_SOUND_SAMPLE_RATE];
};

struct aga_sound {
	aga_uchar_t* pcm;
	aga_size_t len;
	aga_size_t pos;
};

enum aga_result aga_sound_device_new(const char*, struct aga_sound_device*);
enum aga_result aga_sound_device_delete(struct aga_sound_device*);

enum aga_result aga_sound_device_flush(struct aga_sound_device*, aga_size_t*);

enum aga_result aga_sound_play(struct aga_sound_device*, struct aga_sound*);

#endif
