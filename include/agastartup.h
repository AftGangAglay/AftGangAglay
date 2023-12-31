/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_STARTUP_H
#define AGA_STARTUP_H

#include <agaconf.h>

#include <afeirsa/afeirsa.h>

struct aga_opts {
	const char* config_file;
	const char* display;
	const char* chdir;
	const char* version;

	const char* audio_dev;
	af_bool_t audio_enabled;

	const char* startup_script;
	const char* python_path;

	af_size_t width;
	af_size_t height;

	float fov;

	struct aga_conf_node config;
};

/* NOTE: We try to leave sensible defaults in `opts' during failure states. */
enum af_err aga_setopts(struct aga_opts* opts, int argc, char** argv);

enum af_err aga_prerun_hook(struct aga_opts* opts);

#endif
