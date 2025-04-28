/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/sound.h>
#include <asys/base.h>
#include <aga/pack.h>

/*
 * TODO: Apparently Cygwin supports `/dev/dsp'? This check may be too
 * 		 Restrictive.
 */
/*
 * TODO: None of these checks are necessary. Try to open `/dev/audio' with
 * 		 asys streams -- if it isn't there it isn't there.
 */
#if !defined(ASYS_WIN32) && \
		defined(AGA_HAVE_UNISTD) && defined(AGA_HAVE_FCNTL) && \
		defined(AGA_HAVE_SYS_STAT) && defined(AGA_HAVE_SYS_TYPES)

# define AGA_HAVE_SUN_SOUND
#endif

#ifdef AGA_HAVE_SUN_SOUND
# define AGA_WANT_UNIX
# include <aga/std.h>
# include <asys/log.h>
# include <aga/error.h>
# include <aga/utility.h>

enum asys_result aga_sound_device_new(
		struct aga_sound_device* dev, asys_size_t size) {

	enum asys_result result;

	if(!dev) return ASYS_RESULT_BAD_PARAM;

	dev->streams = 0;
	dev->count = 0;

	/*
	 * Assume default open state is "8-bit, 8Khz, mono u-Law data" from SunOS
	 * audio(7i) manpage
	 */
	/* TODO: Try `/dev/dsp' after this if missing `/dev/audio'. */
	dev->fd = open("/dev/audio", O_WRONLY | O_NONBLOCK);
	if(dev->fd == -1) return aga_error_system(__FILE__, "open");
	/*
	 * `/dev/audio' exclusivity varies by system -- we'll just have to assume
	 * The user doesn't have any other audio-playing applications open on
	 * Legacy systems, as modern OSS emulation (with `padsp')
	 * Allows for device mixing.
	 *
	 * start = time(0);
	 * do {
	 * 	if(open == -1) {
	 * 		if(errno != EBUSY) error
	 * 		if(!busy_msg) print busy
	 *
	 * 		if((time(0) - start) >= timeout) {
	 * 			print timeout
	 * 			error
	 * 		}
	 * 	}
	 * 	else break;
	 * } while(errno == EBUSY);
	 */

	dev->size = size;

	dev->scratch = 0; /* For `cleanup'. */

	if(!(dev->buffer = aga_malloc(size))) {
		result = aga_error_system(__FILE__, "malloc");
		goto cleanup;
	}

	if(!(dev->scratch = aga_malloc(size))) {
		result = aga_error_system(__FILE__, "malloc");
		goto cleanup;
	}

	return ASYS_RESULT_OK;

	cleanup: {
		if(close(dev->fd) == -1) aga_error_system(__FILE__, "close");

		asys_memory_free(dev->buffer);
		asys_memory_free(dev->scratch);

		return result;
	}
}

/*
 * TODO: Review new/delete functions to ensure we aren't leaking resources in
 * 		 Error conditions.
 */
enum asys_result aga_sound_device_delete(struct aga_sound_device* dev) {
	enum asys_result result = ASYS_RESULT_OK;

	if(!dev) return ASYS_RESULT_BAD_PARAM;

	if(close(dev->fd) == -1) result = aga_error_system(__FILE__, "close");

	free(dev->buffer);
	free(dev->scratch);
	free(dev->streams);

	return result;
}

/* TODO: User controllable clip function. */
static double aga_sound_clip(double a, double b) {
	double imm = a + b;

	imm *= 0.5;

	if(imm > 1.0) imm = 1.0;

	return imm;
}

enum asys_result aga_sound_device_update(struct aga_sound_device* dev) {
	enum asys_result result;

	asys_size_t total = 0;
	asys_size_t i, j;

	if(!dev) return ASYS_RESULT_BAD_PARAM;

	aga_bzero(dev->buffer, dev->size);

	while(ASYS_TRUE) {
		asys_size_t rem = dev->size - total;
		asys_size_t req = rem > dev->size ? dev->size : rem;

		for(i = 0; i < dev->count; ++i) {
			struct aga_sound_stream* stream = &dev->streams[i];
			void* fp;
			asys_size_t rdsz;
			asys_bool_t eof = ASYS_FALSE;

			stream->did_finish = ASYS_FALSE;
			/*
			 * TODO: Add sound device sweep function to clean up finished
			 * 		 Streams.
			 */
			if(stream->done) continue;

			result = aga_resource_seek(stream->resource, &fp);
			if(result) return result;

			if(fseek(fp, (long) stream->offset, SEEK_CUR)) {
				return aga_error_system(__FILE__, "fseek");
			}

			rdsz = fread(dev->scratch, 1, req, fp);
			if(rdsz < req) {
				if(ferror(fp)) {
					return aga_error_system(__FILE__, "fread");
				}
				else eof = ASYS_TRUE;
			}
			stream->last_seek = rdsz;
			stream->offset += rdsz;

			for(j = 0; j < rdsz; ++j) {
				static const double smax = (double) 0xFF;

				double v = aga_sound_clip(
						dev->buffer[j] / smax, dev->scratch[j] / smax);
				dev->buffer[j] = (asys_uchar_t) (v * smax);
			}

			if(eof) {
				if(stream->loop) stream->offset = 0;
				else {
					stream->done = ASYS_TRUE;
					stream->did_finish = ASYS_TRUE;
				}
			}
		}

		{
			asys_size_t reseek, over;
			ssize_t wrsz = write(dev->fd, dev->buffer, req);

			if (wrsz == -1) {
				if (errno == EWOULDBLOCK || errno == EAGAIN) over = req;
				else return aga_error_system(__FILE__, "write");
			}
			else over = req - wrsz;

			if (over) {
				for (i = 0; i < dev->count; ++i) {
					struct aga_sound_stream* stream = &dev->streams[i];

					if (stream->done) {
						if (stream->did_finish) {
							stream->done = ASYS_FALSE;
							stream->did_finish = ASYS_FALSE;
						}
						else continue;
					}

					reseek = stream->last_seek > over ?
								stream->last_seek : over;

					if (!stream->offset) {
						stream->offset = stream->resource->size - reseek;
					}
					else stream->offset -= reseek;
				}
			}

			if (req == rem) break;
			total += wrsz;
		}
	}

	return ASYS_RESULT_OK;
}

enum asys_result aga_sound_play(
		struct aga_sound_device* dev, struct aga_resource* res,
		asys_bool_t loop, asys_size_t* ind) {

	struct aga_sound_stream* stream;

	if(!dev) return ASYS_RESULT_BAD_PARAM;
	if(!res) return ASYS_RESULT_BAD_PARAM;
	if(!ind) return ASYS_RESULT_BAD_PARAM;

	*ind = dev->count;

	dev->streams = aga_realloc(
			dev->streams, ++dev->count * sizeof(struct aga_sound_stream));

	if(!dev->streams) return aga_error_system(__FILE__, "realloc");

	stream = &dev->streams[*ind];
	aga_bzero(stream, sizeof(struct aga_sound_stream));

	stream->resource = res;
	stream->loop = loop;

	return ASYS_RESULT_OK;
}

#else

enum asys_result aga_sound_device_new(struct aga_sound_device* dev, asys_size_t size) {
	(void) dev;
	(void) size;

	return ASYS_RESULT_OK;
}
enum asys_result aga_sound_device_delete(struct aga_sound_device* dev) {
	(void) dev;

	return ASYS_RESULT_OK;
}

enum asys_result aga_sound_device_update(struct aga_sound_device* dev) {
	(void) dev;

	return ASYS_RESULT_OK;
}

/* Start a new sound stream into the device */
enum asys_result aga_sound_play(
		struct aga_sound_device* dev, struct aga_resource* res, asys_bool_t loop,
		asys_size_t* ind) {

	(void) dev;
	(void) res;
	(void) loop;
	(void) ind;

	return ASYS_RESULT_OK;
}

#endif
