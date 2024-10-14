/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_SOUND_H
#define AGA_SOUND_H

#include <aga/environment.h>
#include <aga/result.h>

struct aga_sound_stream {
	void* fp;

	aga_bool_t loop;
	aga_bool_t done;

	aga_bool_t did_finish;
	aga_size_t last_seek;

	aga_size_t size;
	aga_size_t base;
	aga_size_t offset;
};

struct aga_sound_device {
	int fd;

	aga_uchar_t* buffer;
	aga_uchar_t* scratch;

	aga_size_t size;

	struct aga_sound_stream* streams;
	aga_size_t count;
};

enum aga_result aga_sound_device_new(struct aga_sound_device*, aga_size_t);
enum aga_result aga_sound_device_delete(struct aga_sound_device*);

enum aga_result aga_sound_device_update(struct aga_sound_device*);

/* Start a new sound stream into the device */
enum aga_result aga_sound_play(struct aga_sound_device*, aga_size_t*);

#endif
