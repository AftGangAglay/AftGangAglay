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

enum aga_result aga_mksnddev(const char* dev, struct aga_snddev* snddev) {
	aga_bool_t busy_msg = AGA_FALSE;
	unsigned value;

	if(!snddev) return AGA_RESULT_BAD_PARAM;
	if(!dev) return AGA_RESULT_BAD_PARAM;

	memset(snddev->buf, 0, sizeof(snddev->buf));

	do {
		if((snddev->fd = open(dev, O_WRONLY | O_NONBLOCK)) == -1) {
			if(errno != EBUSY) {
				return aga_errno_path(__FILE__, "open", dev);
			}
			if(!busy_msg) {
				aga_log(__FILE__, "Sound device `%s' busy. Waiting...", dev);
				busy_msg = AGA_TRUE;
				errno = EBUSY;
			}
		}
		else break;
	} while(errno == EBUSY);

	aga_log(__FILE__, "Sound device `%s' acquired!", dev);

	value = AGA_SND_SAMPLEBITS;
	if(ioctl(snddev->fd, SOUND_PCM_WRITE_BITS, &value) == -1) {
		return aga_errno(__FILE__, "ioctl");
	}
	value = AGA_SND_CHANNELS;
	if(ioctl(snddev->fd, SOUND_PCM_WRITE_CHANNELS, &value) == -1) {
		return aga_errno(__FILE__, "ioctl");
	}
	value = AGA_SND_SAMPLERATE;
	if(ioctl(snddev->fd, SOUND_PCM_WRITE_RATE, &value) == -1) {
		return aga_errno(__FILE__, "ioctl");
	}
	value = 1;
	if(ioctl(snddev->fd, SOUND_PCM_NONBLOCK, &value) == -1) {
		return aga_errno(__FILE__, "ioctl");
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_killsnddev(struct aga_snddev* snddev) {
	if(!snddev) return AGA_RESULT_BAD_PARAM;

	if(ioctl(snddev->fd, SOUND_PCM_RESET) == -1) {
		return aga_errno(__FILE__, "ioctl");
	}

	if(close(snddev->fd) == -1) return aga_errno(__FILE__, "close");

	return AGA_RESULT_OK;
}

enum aga_result aga_flushsnd(struct aga_snddev* snddev, aga_size_t* written) {
	struct pollfd pollfd;
	int rdy;

	if(!snddev) return AGA_RESULT_BAD_PARAM;

	pollfd.fd = snddev->fd;
	pollfd.events = POLLOUT;

	*written = 0;

	if((rdy = poll(&pollfd, 1, 0)) == -1) {
		return aga_errno(__FILE__, "poll");
	}

	if(rdy && (pollfd.revents & POLLOUT)) {
		ssize_t res = write(snddev->fd, snddev->buf, sizeof(snddev->buf));
		*written = res;
		if(res != sizeof(snddev->buf)) {
			if(errno && errno != EAGAIN) {
				return aga_errno(__FILE__, "write");
			}
		}
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_putclip(struct aga_snddev* snddev, struct aga_clip* clip) {
	aga_size_t written;

	if(!snddev) return AGA_RESULT_BAD_PARAM;
	if(!clip) return AGA_RESULT_BAD_PARAM;

	AGA_CHK(aga_flushsnd(snddev, &written));

	clip->pos += written;
	if(clip->pos < clip->len) {
		aga_size_t sz = sizeof(snddev->buf);
		aga_size_t rem = clip->len - clip->pos;
		aga_size_t cpy = sz < rem ? sz : rem;
		memcpy(snddev->buf, &clip->pcm[clip->pos], cpy);
	}

	return AGA_RESULT_OK;
}

#else

enum aga_result aga_mksnddev(const char* dev, struct aga_snddev* snddev) {
	(void) snddev;
	(void) dev;
	return AGA_RESULT_OK;
}

enum aga_result aga_killsnddev(struct aga_snddev* snddev) {
	(void) snddev;
	return AGA_RESULT_OK;
}

enum aga_result aga_flushsnd(struct aga_snddev* snddev, aga_size_t* written) {
	(void) snddev;
	*written = 0;
	return AGA_RESULT_OK;
}

enum aga_result aga_putclip(struct aga_snddev* snddev, struct aga_clip* clip) {
	(void) snddev;
	(void) clip;
	return AGA_RESULT_OK;
}

#endif
