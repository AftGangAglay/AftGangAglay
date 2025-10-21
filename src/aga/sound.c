/*
 * SPDX-License-Identifier: X11
 * Copyright (C) 2025 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/sound.h>
#include <aga/pack.h>

#include <asys/base.h>
#include <asys/log.h>
#include <asys/memory.h>
#include <asys/error.h>

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
	result = asys_stream_new_write(&dev->device_stream, "/dev/audio");
	if(result) return result;

	result = asys_stream_set_nonblock(&dev->device_stream);
	if(result) goto cleanup;

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

	if(!(dev->buffer = asys_memory_allocate(size))) {
		result = ASYS_RESULT_OOM;
		goto cleanup;
	}

	if(!(dev->scratch = asys_memory_allocate(size))) {
		result = ASYS_RESULT_OOM;
		goto cleanup;
	}

	return ASYS_RESULT_OK;

	cleanup: {
		enum asys_result cleanup_result;

		cleanup_result = asys_stream_delete(&dev->device_stream);
		asys_result_check(__FILE__, "asys_stream_delete", cleanup_result);

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
	if(!dev) return ASYS_RESULT_BAD_PARAM;

	asys_memory_free(dev->buffer);
	asys_memory_free(dev->scratch);
	asys_memory_free(dev->streams);

	return asys_stream_delete(&dev->device_stream);
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

	asys_memory_zero(dev->buffer, dev->size);

	while(ASYS_TRUE) {
		asys_size_t remainder = dev->size - total;
		asys_size_t want = remainder > dev->size ? dev->size : remainder;

		for(i = 0; i < dev->count; ++i) {
			struct aga_sound_stream* stream = &dev->streams[i];
			struct asys_stream* fp;

			asys_size_t read_count;
			asys_bool_t eof = ASYS_FALSE;

			stream->did_finish = ASYS_FALSE;

			/*
			 * TODO: Add sound device sweep function to clean up finished
			 * 		 Streams.
			 */
			if(stream->done) continue;

			result = aga_resource_seek(stream->resource, &fp);
			if(result) return result;

			/*
			 * TODO: Just splice the streams if there is only one user allows
			 * 		 Clipping to be overridden.
			 */

			result = asys_stream_seek(fp, ASYS_SEEK_CURRENT, stream->offset);
			if(result) return result;

			result = asys_stream_read(fp, &read_count, dev->scratch, want);
			if(result) {
				if(result == ASYS_RESULT_EOF) eof = ASYS_TRUE;
				else return result;
			}

			stream->last_seek = read_count;
			stream->offset += (asys_offset_t) read_count;

			for(j = 0; j < read_count; ++j) {
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
			asys_size_t seek_corrected, over, write_count;

			result = asys_stream_write(
					&dev->device_stream, &write_count, dev->buffer, want);

			if(result) {
				if(result == ASYS_RESULT_BLOCKING) {
					over = want;
				}
				else return result;
			}
			else over = want - write_count;

			if(over) {
				for (i = 0; i < dev->count; ++i) {
					struct aga_sound_stream* stream = &dev->streams[i];

					if(stream->done) {
						if(stream->did_finish) {
							stream->done = ASYS_FALSE;
							stream->did_finish = ASYS_FALSE;
						}
						else continue;
					}

					seek_corrected = stream->last_seek > over ?
								stream->last_seek : over;

					if(!stream->offset) {
						stream->offset =
								(asys_offset_t) (stream->resource->size -
													seek_corrected);
					}
					else stream->offset -= (asys_offset_t) seek_corrected;
				}
			}

			if(want == remainder) break;
			total += write_count;
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

	/*
	 * TODO: Once we have resource limits -- warn on dev about exceeding
	 * 		 Concurrent streams.
	 */

	dev->streams = asys_memory_reallocate_safe(
			dev->streams, ++dev->count * sizeof(struct aga_sound_stream));

	if(!dev->streams) return ASYS_RESULT_OOM;

	stream = &dev->streams[*ind];
	asys_memory_zero(stream, sizeof(struct aga_sound_stream));

	stream->resource = res;

	/*
	 * TODO: Looping streams are broken at the moment. Are we relying on
	 * 		 Stream EOF?
	 */
	stream->loop = loop;

	return ASYS_RESULT_OK;
}

enum asys_result aga_sound_clear(struct aga_sound_device* dev) {
	if(!dev) return ASYS_RESULT_BAD_PARAM;

	if(dev->streams) asys_memory_free(dev->streams);
	dev->streams = 0;
	dev->count = 0;

	return ASYS_RESULT_OK;
}
