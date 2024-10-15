/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/sound.h>
#include <aga/environment.h>
#include <aga/pack.h>

/*
 * TODO: Apparently Cygwin supports `/dev/dsp'? This check may be too
 * 		 Restrictive.
 */
#if !defined(_WIN32) && \
		defined(AGA_HAVE_UNISTD) && defined(AGA_HAVE_FCNTL) && \
		defined(AGA_HAVE_SYS_STAT) && defined(AGA_HAVE_SYS_TYPES)

# define AGA_HAVE_SUN_SOUND
#endif

#ifdef AGA_HAVE_SUN_SOUND
# define AGA_WANT_UNIX
# include <aga/std.h>
# include <aga/log.h>
# include <aga/error.h>
# include <aga/utility.h>

enum aga_result aga_sound_device_new(
		struct aga_sound_device* dev, aga_size_t size) {

	enum aga_result result;

	if(!dev) return AGA_RESULT_BAD_PARAM;

	dev->streams = 0;
	dev->count = 0;

	/*
	 * Assume default open state is "8-bit, 8Khz, mono u-Law data" from SunOS
	 * audio(7i) manpage
	 */
	/* TODO: Try `/dev/dsp' after this. What are the default params there? */
	dev->fd = open("/dev/audio", O_WRONLY | O_NONBLOCK);
	if(dev->fd == -1) return aga_error_system(__FILE__, "open");
	/*
	 * `/dev/audio' exclusivity varies by system -- we'll just have to assume
	 * the user doesn't have any other audio-playing applications open on
	 * legacy systems, as modern OSS emulation allows for device mixing.
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

	return AGA_RESULT_OK;

	cleanup: {
		if(close(dev->fd) == -1) aga_error_system(__FILE__, "close");

		aga_free(dev->buffer);
		aga_free(dev->scratch);

		return result;
	}
}

/*
 * TODO: Review new/delete functions to ensure we aren't leaking resources in
 * 		 Error conditions.
 */
enum aga_result aga_sound_device_delete(struct aga_sound_device* dev) {
	enum aga_result result = AGA_RESULT_OK;

	if(!dev) return AGA_RESULT_BAD_PARAM;

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

enum aga_result aga_sound_device_update(struct aga_sound_device* dev) {
	enum aga_result result;

	aga_size_t total = 0;
	aga_size_t i, j;

	if(!dev) return AGA_RESULT_BAD_PARAM;

	aga_bzero(dev->buffer, dev->size);

	while(AGA_TRUE) {
		aga_size_t rem = dev->size - total;
		aga_size_t req = rem > dev->size ? dev->size : rem;

		for(i = 0; i < dev->count; ++i) {
			struct aga_sound_stream* stream = &dev->streams[i];
			void* fp;
			aga_size_t rdsz;
			aga_bool_t eof = AGA_FALSE;

			stream->did_finish = AGA_FALSE;
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
				else eof = AGA_TRUE;
			}
			stream->last_seek = rdsz;
			stream->offset += rdsz;

			for(j = 0; j < rdsz; ++j) {
				static const double smax = (double) 0xFF;

				double v = aga_sound_clip(
						dev->buffer[j] / smax, dev->scratch[j] / smax);
				dev->buffer[j] = (aga_uchar_t) (v * smax);
			}

			if(eof) {
				if(stream->loop) stream->offset = 0;
				else {
					stream->done = AGA_TRUE;
					stream->did_finish = AGA_TRUE;
				}
			}
		}

		{
			aga_size_t reseek, over;
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
							stream->done = AGA_FALSE;
							stream->did_finish = AGA_FALSE;
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

	return AGA_RESULT_OK;
}

enum aga_result aga_sound_play(
		struct aga_sound_device* dev, struct aga_resource* res,
		aga_bool_t loop, aga_size_t* ind) {

	enum aga_result result;

	struct aga_sound_stream* stream;

	if(!dev) return AGA_RESULT_BAD_PARAM;
	if(!res) return AGA_RESULT_BAD_PARAM;
	if(!ind) return AGA_RESULT_BAD_PARAM;

	*ind = dev->count;

	/* TODO: `printf'/`allocator' attributes for our stdlib wrappers. */
	dev->streams = aga_realloc(
			dev->streams, ++dev->count * sizeof(struct aga_sound_stream));

	if(!dev->streams) return aga_error_system(__FILE__, "realloc");

	stream = &dev->streams[*ind];
	aga_bzero(stream, sizeof(struct aga_sound_stream));

	stream->resource = res;
	stream->loop = loop;

	return AGA_RESULT_OK;
}

#else

enum aga_result aga_sound_device_new(struct aga_sound_device* dev, aga_size_t size) {
	(void) dev;
	(void) size;

	return AGA_RESULT_OK;
}
enum aga_result aga_sound_device_delete(struct aga_sound_device* dev) {
	(void) dev;

	return AGA_RESULT_OK;
}

enum aga_result aga_sound_device_update(struct aga_sound_device* dev) {
	(void) dev;

	return AGA_RESULT_OK;
}

/* Start a new sound stream into the device */
enum aga_result aga_sound_play(
		struct aga_sound_device* dev, struct aga_resource* res, aga_bool_t loop,
		aga_size_t* ind) {

	(void) dev;
	(void) res;
	(void) loop;
	(void) ind;

	return AGA_RESULT_OK;
}

#endif
