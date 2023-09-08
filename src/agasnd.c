/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agasnd.h>
#include <agacore.h>

#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

enum af_err aga_mksnddev(const char* dev, struct aga_snddev* snddev) {
	af_bool_t busy_msg = AF_FALSE;
	unsigned value;

	AF_PARAM_CHK(snddev);
	AF_PARAM_CHK(dev);

	do {
		if ((snddev->fd = open(dev, O_WRONLY)) == -1) {
			if(errno != EBUSY) aga_errno_chk("open");
			if(!busy_msg) {
				fprintf(
					stderr, "Sound device `%s' busy. Waiting...\n", dev);
				busy_msg = AF_TRUE;
			}
		}
		else break;
	} while(errno == EBUSY);
	printf("Sound device `%s' acquired!\n", dev);

	value = AGA_SND_SAMPLEBITS;
	if(ioctl(snddev->fd, SOUND_PCM_WRITE_BITS, &value) == -1) {
		aga_errno_chk("ioctl");
	}
	value = AGA_SND_CHANNELS;
	if(ioctl(snddev->fd, SOUND_PCM_WRITE_CHANNELS, &value) == -1) {
		aga_errno_chk("ioctl");
	}
	value = AGA_SND_SAMPLERATE;
	if(ioctl(snddev->fd, SOUND_PCM_WRITE_RATE, &value) == -1) {
		aga_errno_chk("ioctl");
	}

	return AF_ERR_NONE;
}
enum af_err aga_killsnddev(struct aga_snddev* snddev) {
	AF_PARAM_CHK(snddev);

	if(close(snddev->fd) == -1) aga_errno_chk("close");

	return AF_ERR_NONE;
}

enum af_err aga_flushsnd(struct aga_snddev* snddev) {
	ssize_t res;

	res = write(snddev->fd, snddev->buf, sizeof(snddev->buf));
	if(res != sizeof(snddev->buf)) aga_errno_chk("write");

	return AF_ERR_NONE;
}