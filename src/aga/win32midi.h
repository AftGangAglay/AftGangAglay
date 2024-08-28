/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_W_MIDI_H
#define AGA_W_MIDI_H

#include <aga/log.h>
#include <aga/utility.h>

#define AGA_WANT_WINDOWS_H
#include <aga/win32.h>

static enum aga_result aga_mmsys_result(unsigned err) {
	switch(err) {
		default: return AGA_RESULT_ERROR;
		case MMSYSERR_NOERROR: return AGA_RESULT_OK;

		case MMSYSERR_NOMEM: return AGA_RESULT_OOM;

		case MIDIERR_NODEVICE: return AGA_RESULT_ERROR;

		case MMSYSERR_ALLOCATED: return AGA_RESULT_BAD_PARAM;
		case MMSYSERR_BADDEVICEID: return AGA_RESULT_BAD_PARAM;
		case MMSYSERR_INVALHANDLE: return AGA_RESULT_BAD_PARAM;
		case MMSYSERR_INVALPARAM: return AGA_RESULT_BAD_PARAM;

		case MIDIERR_STILLPLAYING: return AGA_RESULT_BAD_OP;
		case MIDIERR_UNPREPARED: return AGA_RESULT_BAD_PARAM;
	}
}

enum aga_result aga_midi_device_new(struct aga_midi_device* dev) {
	unsigned res, id;

	if(!dev) return AGA_RESULT_BAD_PARAM;

	id = MIDI_MAPPER;
	res = midiStreamOpen((void*) &dev->dev, &id, 1, 0, 0, CALLBACK_NULL);
	if(res == MIDIERR_NODEVICE) {
		aga_log(__FILE__, "err: No MIDI devices available");
	}

	return aga_mmsys_result(res);
}

enum aga_result aga_midi_device_delete(struct aga_midi_device* dev) {
	if(!dev) return AGA_RESULT_BAD_PARAM;

	return AGA_RESULT_OK;
}

enum aga_result aga_midi_new(
		struct aga_midi_device* dev, struct aga_midi* midi, void* buf,
		aga_size_t len) {

	enum aga_result result;
	unsigned err;
	MIDIHDR* hdr;

	if(!dev) return AGA_RESULT_BAD_PARAM;
	if(!midi) return AGA_RESULT_BAD_PARAM;
	if(!buf) return AGA_RESULT_BAD_PARAM;

	if(!(midi->hdr = aga_calloc(1, sizeof(MIDIHDR)))) return AGA_RESULT_OOM;
	hdr = midi->hdr;

	hdr->lpData = buf;
	hdr->dwBytesRecorded = hdr->dwBufferLength = len;

	err = midiOutPrepareHeader(dev->dev, hdr, sizeof(MIDIHDR));
	result = aga_mmsys_result(err);
	if(result) goto cleanup;

	return AGA_RESULT_OK;

	cleanup: {
		free(hdr);

		return result;
	};
}

enum aga_result aga_midi_delete(
		struct aga_midi_device* dev, struct aga_midi* midi) {

	if(!dev) return AGA_RESULT_BAD_PARAM;
	if(!midi) return AGA_RESULT_BAD_PARAM;

	return AGA_RESULT_OK;
}

enum aga_result aga_midi_play(
		struct aga_midi_device* dev, struct aga_midi* midi) {

	unsigned err;

	if(!dev) return AGA_RESULT_BAD_PARAM;
	if(!midi) return AGA_RESULT_BAD_PARAM;

	/*
	 * NOTE: MSDN claims "invalid parameter" here means invalid header pointer,
	 * 		 But it can also refer to invalid MIDI event data. This probably
	 * 		 Indicates you're loading the wrong file (.mid instead of .mid.raw)
	 * 		 Or our importer is just broken.
	 */
	err = midiStreamOut(dev->dev, midi->hdr, sizeof(MIDIHDR));
	if(err != MMSYSERR_NOERROR) return aga_mmsys_result(err);

	err = midiStreamRestart(dev->dev);
	return aga_mmsys_result(err);
}

#endif
