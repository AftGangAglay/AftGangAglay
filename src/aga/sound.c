/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/sound.h>
#include <aga/environment.h>

#if defined(AGA_HAVE_POLL) && defined(AGA_HAVE_SYS_IOCTL) && \
    defined(AGA_HAVE_SYS_SOUNDCARD) && defined(AGA_HAVE_SYS_TYPES) && \
    defined(AGA_HAVE_FCNTL)
# define AGA_HAVE_SOUND
#endif

#ifdef AGA_HAVE_SOUND
# define AGA_WANT_UNIX
# include <aga/std.h>
# include <aga/log.h>
# include <aga/error.h>

#define AGA_SOUND_TIMEOUT (1)

enum aga_result aga_sound_device_new(
		const char* dev, struct aga_sound_device* sound_device) {

	aga_bool_t busy_msg = AGA_FALSE;
	unsigned value;
	time_t start;

	if(!sound_device) return AGA_RESULT_BAD_PARAM;
	if(!dev) return AGA_RESULT_BAD_PARAM;

	memset(sound_device->buf, 0, sizeof(sound_device->buf));

	start = time(0);
	do {
		if((sound_device->fd = open(dev, O_WRONLY | O_NONBLOCK)) == -1) {
			if(errno != EBUSY) {
				return aga_error_system_path(__FILE__, "open", dev);
			}
			if(!busy_msg) {
				aga_log(__FILE__, "Sound device `%s' busy. Waiting...", dev);
				busy_msg = AGA_TRUE;
				errno = EBUSY;
			}

			if((time(0) - start) >= AGA_SOUND_TIMEOUT) {
				aga_log(__FILE__, "Sound device `%s' timed out", dev);
				return AGA_RESULT_ERROR;
			}
		}
		else break;
	} while(errno == EBUSY);

	aga_log(__FILE__, "Sound device `%s' acquired!", dev);

	value = AGA_SOUND_SAMPLE_BITS;
	if(ioctl(sound_device->fd, SOUND_PCM_WRITE_BITS, &value) == -1) {
		return aga_error_system(__FILE__, "ioctl");
	}
	value = AGA_SOUND_CHANNELS;
	if(ioctl(sound_device->fd, SOUND_PCM_WRITE_CHANNELS, &value) == -1) {
		return aga_error_system(__FILE__, "ioctl");
	}
	value = AGA_SOUND_SAMPLE_RATE;
	if(ioctl(sound_device->fd, SOUND_PCM_WRITE_RATE, &value) == -1) {
		return aga_error_system(__FILE__, "ioctl");
	}
	value = 1;
	if(ioctl(sound_device->fd, SOUND_PCM_NONBLOCK, &value) == -1) {
		return aga_error_system(__FILE__, "ioctl");
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_sound_device_delete(
		struct aga_sound_device* sound_device) {

	if(!sound_device) return AGA_RESULT_BAD_PARAM;

	if(ioctl(sound_device->fd, SOUND_PCM_RESET) == -1) {
		return aga_error_system(__FILE__, "ioctl");
	}

	if(close(sound_device->fd) == -1) {
		return aga_error_system(__FILE__, "close");
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_sound_device_flush(
		struct aga_sound_device* sound_device, aga_size_t* written) {

	struct pollfd fd;
	int rdy;

	if(!sound_device) return AGA_RESULT_BAD_PARAM;

	fd.fd = sound_device->fd;
	fd.events = POLLOUT;

	*written = 0;

	if((rdy = poll(&fd, 1, 0)) == -1) {
		return aga_error_system(__FILE__, "poll");
	}

	if(rdy && (fd.revents & POLLOUT)) {
		ssize_t res = write(
				sound_device->fd, sound_device->buf,
				sizeof(sound_device->buf));

		*written = res;

		if(res != sizeof(sound_device->buf)) {
			if(errno && errno != EAGAIN) {
				return aga_error_system(__FILE__, "write");
			}
		}
	}

	return AGA_RESULT_OK;
}

enum aga_result aga_sound_play(
		struct aga_sound_device* sound_device, struct aga_sound* clip) {

	enum aga_result result;
	aga_size_t written;

	if(!sound_device) return AGA_RESULT_BAD_PARAM;
	if(!clip) return AGA_RESULT_BAD_PARAM;

	result = aga_sound_device_flush(sound_device, &written);
	if(result) return result;

	clip->pos += written;
	if(clip->pos < clip->len) {
		aga_size_t sz = sizeof(sound_device->buf);
		aga_size_t rem = clip->len - clip->pos;
		aga_size_t cpy = sz < rem ? sz : rem;
		memcpy(sound_device->buf, &clip->pcm[clip->pos], cpy);
	}

	return AGA_RESULT_OK;
}

#else

enum aga_result aga_sound_device_new(
		const char* dev, struct aga_sound_device* sound_device) {

	(void) sound_device;
	(void) dev;

	return AGA_RESULT_OK;
}

enum aga_result aga_sound_device_delete(
		struct aga_sound_device* sound_device) {

	(void) sound_device;

	return AGA_RESULT_OK;
}

enum aga_result aga_sound_device_flush(
		struct aga_sound_device* sound_device, aga_size_t* written) {

	(void) sound_device;

	*written = 0;

	return AGA_RESULT_OK;
}

enum aga_result aga_sound_play(
		struct aga_sound_device* sound_device, struct aga_sound* clip) {

	(void) sound_device;
	(void) clip;

	return AGA_RESULT_OK;
}

#endif
