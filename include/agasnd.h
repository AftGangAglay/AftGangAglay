/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_SND_H
#define AGA_SND_H

#include <afeirsa/aferr.h>
#include <afeirsa/aftypes.h>

enum aga_snd_conf {
	AGA_SND_SAMPLERATE = 8000,
	AGA_SND_CHANNELS = 1,
	AGA_SND_SAMPLEBITS = 8,
	AGA_SND_BLOCKS = 200,
	AGA_SND_BLOCKSIZE = AGA_SND_SAMPLERATE / AGA_SND_BLOCKS
};

struct aga_snddev {
	int fd;
	af_uint8_t buf[AGA_SND_SAMPLERATE];
};

enum af_err aga_mksnddev(const char* dev, struct aga_snddev* snddev);
enum af_err aga_killsnddev(struct aga_snddev* snddev);

enum af_err aga_flushsnd(struct aga_snddev* snddev, af_size_t* written);

#endif
