/*
 * SPDX-License-Identifier: X11
 * Copyright (C) 2025 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_STARTUP_H
#define AGA_STARTUP_H

#include <aga/config.h>

#include <asys/result.h>

struct asys_main_data;
struct aga_resource_pack;

struct aga_settings {
#ifdef AGA_DEVBUILD
	asys_bool_t compile;
	const char* build_file;

	asys_bool_t no_stamp;
#endif

	const char* title;

	const char* config_file;
	const char* display;
	const char* chdir;
	const char* version;

	const char* respack;

	asys_size_t audio_buffer;
	asys_bool_t audio_enabled;

	char* startup_script;
	char* python_path;

	asys_size_t width;
	asys_size_t height;

	asys_bool_t mipmap_default;

	float fov;

	asys_bool_t verbose;
	asys_bool_t profiler;

	struct aga_config_node config;
};

/* NOTE: We try to leave sensible defaults in `opts' during failure states. */
enum asys_result aga_settings_new(
		struct aga_settings*, struct asys_main_data*);

enum asys_result aga_settings_parse_config(
		struct aga_settings*, struct aga_resource_pack*);

#endif
