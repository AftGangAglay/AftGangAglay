/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_STARTUP_H
#define AGA_STARTUP_H

#include <aga/config.h>
#include <aga/result.h>

struct aga_resource_pack;

struct aga_settings {
	aga_bool_t compile;
	const char* build_file;

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

	struct aga_config_node config;
};

/* NOTE: We try to leave sensible defaults in `opts' during failure states. */
enum aga_result aga_settings_new(struct aga_settings*, int, char**);

enum aga_result aga_settings_parse_config(
		struct aga_settings*, struct aga_resource_pack*);

/* TODO: Remove in next breaking update. */
enum aga_result aga_prerun_hook(struct aga_settings*);

#endif
