/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_GRAPH_H
#define AGA_GRAPH_H

#include <aga/window.h>

#include <apro.h>

struct aga_graph {
	struct aga_window window;

	aga_size_t segments;
	aga_size_t max;

	aga_size_t period;
	aga_size_t inter;

	aga_ulong_t* running;

	aga_ulong_t* histories;
	float* heights;
};

enum aga_result aga_graph_new(
		struct aga_graph*, struct aga_window_device*, int, char**);

enum aga_result aga_graph_delete(struct aga_graph*, struct aga_window_device*);

enum aga_result aga_graph_update(struct aga_graph*, struct aga_window_device*);

enum aga_result aga_graph_plot(
		struct aga_graph*, unsigned, unsigned, enum apro_section);

#endif
