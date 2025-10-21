/*
 * SPDX-License-Identifier: X11
 * Copyright (C) 2025 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_GRAPH_H
#define AGA_GRAPH_H

#include <asys/result.h>

#include <apro.h>

#include <mil/base.h>

struct asys_main_data;

/* TODO: Rename to "profile graph". */
struct aga_graph {
	mil_widget_t gl_area;

	asys_size_t segments;
	asys_size_t max;

	asys_size_t period;
	asys_size_t inter;

	apro_unit_t* running;

	apro_unit_t* histories;
	float* heights;
};

enum asys_result aga_graph_new(struct aga_graph*, struct mil_ctx*);
enum asys_result aga_graph_delete(struct aga_graph*);

enum asys_result aga_graph_update(struct aga_graph*, struct mil_ctx*);
enum asys_result aga_graph_plot(
		struct aga_graph*, unsigned, unsigned, enum apro_section);

#endif
