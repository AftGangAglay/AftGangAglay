/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <asys/base.h>

#include <aga/midi.h>

/* TODO: MIDI. */
#ifdef ASYS_WIN32_DISABLED
#include <asys/log.h>
#include <asys/system.h>

static enum asys_result aga_mmsys_result(unsigned err) {
	switch(err) {
		default: return ASYS_RESULT_ERROR;
		case MMSYSERR_NOERROR: return ASYS_RESULT_OK;

		case MMSYSERR_NOMEM: return ASYS_RESULT_OOM;

		case MIDIERR_NODEVICE: return ASYS_RESULT_ERROR;

		case MMSYSERR_ALLOCATED: return ASYS_RESULT_BAD_PARAM;
		case MMSYSERR_BADDEVICEID: return ASYS_RESULT_BAD_PARAM;
		case MMSYSERR_INVALHANDLE: return ASYS_RESULT_BAD_PARAM;
		case MMSYSERR_INVALPARAM: return ASYS_RESULT_BAD_PARAM;

		case MIDIERR_STILLPLAYING: return ASYS_RESULT_BAD_OP;
		case MIDIERR_UNPREPARED: return ASYS_RESULT_BAD_PARAM;
	}
}

enum asys_result aga_midi_device_new(struct aga_midi_device* dev) {
	unsigned res, id;

	if(!dev) return ASYS_RESULT_BAD_PARAM;

	id = MIDI_MAPPER;
	res = midiStreamOpen((void*) &dev->dev, &id, 1, 0, 0, CALLBACK_NULL);
	if(res == MIDIERR_NODEVICE) {
		asys_log(__FILE__, "err: No MIDI devices available");
	}

	return aga_mmsys_result(res);
}

enum asys_result aga_midi_device_delete(struct aga_midi_device* dev) {
	if(!dev) return ASYS_RESULT_BAD_PARAM;

	return ASYS_RESULT_OK;
}

enum asys_result aga_midi_new(
		struct aga_midi_device* dev, struct aga_midi* midi, void* buffer,
		asys_size_t len) {

	enum asys_result result;
	unsigned err;
	MIDIHDR* hdr;

	if(!dev) return ASYS_RESULT_BAD_PARAM;
	if(!midi) return ASYS_RESULT_BAD_PARAM;
	if(!buffer) return ASYS_RESULT_BAD_PARAM;

	if(!(midi->hdr = asys_calloc(1, sizeof(MIDIHDR)))) return ASYS_RESULT_OOM;
	hdr = midi->hdr;

	hdr->lpData = buffer;
	hdr->dwBytesRecorded = hdr->dwBufferLength = (DWORD) len;

	err = midiOutPrepareHeader(dev->dev, hdr, sizeof(MIDIHDR));
	result = aga_mmsys_result(err);
	if(result) goto cleanup;

	return ASYS_RESULT_OK;

	cleanup: {
		free(hdr);

		return result;
	}
}

enum asys_result aga_midi_delete(
		struct aga_midi_device* dev, struct aga_midi* midi) {

	if(!dev) return ASYS_RESULT_BAD_PARAM;
	if(!midi) return ASYS_RESULT_BAD_PARAM;

	return ASYS_RESULT_OK;
}

enum asys_result aga_midi_play(
		struct aga_midi_device* dev, struct aga_midi* midi) {

	unsigned err;

	if(!dev) return ASYS_RESULT_BAD_PARAM;
	if(!midi) return ASYS_RESULT_BAD_PARAM;

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

#else
#endif
