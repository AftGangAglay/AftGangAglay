/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agasnd.h>
#include <agaenv.h>

#if defined(AGA_HAVE_POLL) && defined(AGA_HAVE_SYS_IOCTL) && \
	defined(AGA_HAVE_SYS_SOUNDCARD) && defined(AGA_HAVE_SYS_TYPES) && \
	defined(AGA_HAVE_FCNTL)
# define AGA_HAVE_SOUND
#endif

#ifdef AGA_HAVE_SOUND
# define AGA_WANT_UNIX
# include <agastd.h>
# include <agalog.h>
# include <agaerr.h>

enum af_err aga_mksnddev(const char* dev, struct aga_snddev* snddev) {
	af_bool_t busy_msg = AF_FALSE;
	unsigned value;

	AF_PARAM_CHK(snddev);
	AF_PARAM_CHK(dev);

	af_memset(snddev->buf, 0, sizeof(snddev->buf));

	do {
		if((snddev->fd = open(dev, O_WRONLY | O_NONBLOCK)) == -1) {
			if(errno != EBUSY) {
				return aga_af_patherrno(__FILE__, "open", dev);
			}
			if(!busy_msg) {
				aga_log(__FILE__, "Sound device `%s' busy. Waiting...", dev);
				busy_msg = AF_TRUE;
				errno = EBUSY;
			}
		}
		else break;
	} while(errno == EBUSY);

	aga_log(__FILE__, "Sound device `%s' acquired!", dev);

	value = AGA_SND_SAMPLEBITS;
	if(ioctl(snddev->fd, SOUND_PCM_WRITE_BITS, &value) == -1) {
		return aga_af_errno(__FILE__, "ioctl");
	}
	value = AGA_SND_CHANNELS;
	if(ioctl(snddev->fd, SOUND_PCM_WRITE_CHANNELS, &value) == -1) {
		return aga_af_errno(__FILE__, "ioctl");
	}
	value = AGA_SND_SAMPLERATE;
	if(ioctl(snddev->fd, SOUND_PCM_WRITE_RATE, &value) == -1) {
		return aga_af_errno(__FILE__, "ioctl");
	}
	value = 1;
	if(ioctl(snddev->fd, SOUND_PCM_NONBLOCK, &value) == -1) {
		return aga_af_errno(__FILE__, "ioctl");
	}

	return AF_ERR_NONE;
}

enum af_err aga_killsnddev(struct aga_snddev* snddev) {
	AF_PARAM_CHK(snddev);

	if(ioctl(snddev->fd, SOUND_PCM_RESET) == -1) {
		return aga_af_errno(__FILE__, "ioctl");
	}

	if(close(snddev->fd) == -1) return aga_af_errno(__FILE__, "close");

	return AF_ERR_NONE;
}

enum af_err aga_flushsnd(struct aga_snddev* snddev, af_size_t* written) {
	struct pollfd pollfd;
	int rdy;

	AF_PARAM_CHK(snddev);

	pollfd.fd = snddev->fd;
	pollfd.events = POLLOUT;

	*written = 0;

	if((rdy = poll(&pollfd, 1, 0)) == -1) {
		return aga_af_errno(__FILE__, "poll");
	}

	if(rdy && (pollfd.revents & POLLOUT)) {
		ssize_t res = write(snddev->fd, snddev->buf, sizeof(snddev->buf));
		*written = res;
		if(res != sizeof(snddev->buf)) {
			if(errno && errno != EAGAIN) {
				return aga_af_errno(__FILE__, "write");
			}
		}
	}

	return AF_ERR_NONE;
}

enum af_err aga_putclip(struct aga_snddev* snddev, struct aga_clip* clip) {
	af_size_t written;

	AF_PARAM_CHK(snddev);
	AF_PARAM_CHK(clip);

	AF_CHK(aga_flushsnd(snddev, &written));

	clip->pos += written;
	if(clip->pos < clip->len) {
		af_size_t sz = sizeof(snddev->buf);
		af_size_t rem = clip->len - clip->pos;
		af_size_t cpy = sz < rem ? sz : rem;
		af_memcpy(snddev->buf, &clip->pcm[clip->pos], cpy);
	}

	return AF_ERR_NONE;
}

#else
enum af_err aga_mksnddev(const char* dev, struct aga_snddev* snddev) {
	(void) snddev;
	(void) dev;
	return AF_ERR_NONE;
}

enum af_err aga_killsnddev(struct aga_snddev* snddev) {
	(void) snddev;
	return AF_ERR_NONE;
}

enum af_err aga_flushsnd(struct aga_snddev* snddev, af_size_t* written) {
	(void) snddev;
	*written = 0;
	return AF_ERR_NONE;
}

enum af_err aga_putclip(struct aga_snddev* snddev, struct aga_clip* clip) {
	(void) snddev;
	(void) clip;
	return AF_ERR_NONE;
}

#endif
