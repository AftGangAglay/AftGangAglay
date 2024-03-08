/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_STARTUP_H
#define AGA_STARTUP_H

#include <agaconf.h>

#include <agaresult.h>

struct aga_respack;

struct aga_opts {
	const char* config_file;
	const char* display;
	const char* chdir;
	const char* version;

	const char* respack;

	const char* audio_dev;
	aga_bool_t audio_enabled;

	const char* startup_script;
	const char* python_path;

	aga_size_t width;
	aga_size_t height;

	float fov;

	aga_bool_t verbose;

	struct aga_conf_node config;
};

/* NOTE: We try to leave sensible defaults in `opts' during failure states. */
enum aga_result aga_setopts(struct aga_opts*, int, char**);

enum aga_result aga_setconf(struct aga_opts*, struct aga_respack*);

enum aga_result aga_prerun_hook(struct aga_opts*);

#endif
