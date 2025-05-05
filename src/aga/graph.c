/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/graph.h>
#include <aga/render.h>

#include <asys/memory.h>
#include <asys/log.h>

#include <mil/widget.h>
#include <mil/gl.h>

enum asys_result aga_graph_new(struct aga_graph* graph, struct mil_ctx* mil) {
#ifdef AGA_DEVBUILD
	if(!graph) return ASYS_RESULT_BAD_PARAM;

	{
		mil_widget_t window, frame, area;

		window = mil_widget(
				mil, "Profile", MIL_WINDOW, mil->top,
				MIL_WIDTH, 1280,
				MIL_HEIGHT, 480,
				MIL_END);

		frame = mil_widget(mil, "frame", MIL_FRAME, window, MIL_END);

		area = mil_widget(
				mil, "gl_area", MIL_DRAWING_AREA, frame,
				/* TODO: Resizing. */
				/* MIL_DRAWING_AREA_RESIZE_CALLBACK, area_resize, */
				/* TODO: Graph controls. */
				/* MIL_DRAWING_AREA_INPUT_CALLBACK, aga_main_window_input, */
				MIL_END);

		graph->gl_area = area;
	}

	/* TODO: Controllable and make max raise to ceiling. */
	graph->segments = 50;
	graph->max = 2000;
	graph->period = 30;

	graph->running = asys_memory_allocate_zero(APRO_MAX, sizeof(apro_unit_t));
	if(!graph->running) return ASYS_RESULT_OOM;

	graph->histories = asys_memory_allocate_zero(
			graph->segments * APRO_MAX, sizeof(apro_unit_t));

	if(!graph->histories) return ASYS_RESULT_OOM;

	graph->heights = asys_memory_allocate_zero(graph->segments, sizeof(float));
	if(!graph->heights) return ASYS_RESULT_OOM;
#else
	(void) graph;
	(void) env;
	(void) main_data;
#endif

	return ASYS_RESULT_OK;
}

enum asys_result aga_graph_delete(struct aga_graph* graph) {
#ifdef AGA_DEVBUILD
	if(!graph) return ASYS_RESULT_BAD_PARAM;

	asys_memory_free(graph->histories);
	asys_memory_free(graph->heights);
	asys_memory_free(graph->running);
#else
	(void) graph;
	(void) env;
#endif

	return ASYS_RESULT_OK;
}

enum asys_result aga_graph_update(
		struct aga_graph* graph, struct mil_ctx* mil) {

#ifdef AGA_DEVBUILD
	static const float clear[] = { 0.4f, 0.4f, 0.4f, 1.0f };

	enum asys_result result;

	unsigned width, height;

	unsigned d = 0;
	unsigned x = 0;
	unsigned n = 0;

	if(!graph) return ASYS_RESULT_BAD_PARAM;
	if(!mil) return ASYS_RESULT_BAD_PARAM;

	result = mil_gl_context_widget(mil, graph->gl_area);
	asys_log_result(__FILE__, "mil_gl_context_widget", result);

	mil_widget_get_size(mil, graph->gl_area, &width, &height);

	result = aga_render_area(0, 0, width, height);
	asys_log_result(__FILE__, "aga_render_area", result);

	asys_log_result(__FILE__, "aga_render_clear", aga_render_clear(clear));

	graph->inter++;

	result = aga_graph_plot(graph, d++, 0, APRO_PRESWAP);
	if(result) return result;
	result = aga_graph_plot(graph, d++, 1, APRO_POLL);
	if(result) return result;
	result = aga_graph_plot(graph, d++, 1, APRO_SCRIPT_UPDATE);
	if(result) return result;
	result = aga_graph_plot(graph, d++, 2, APRO_SCRIPT_INSTCALL);
	if(result) return result;
	result = aga_graph_plot(graph, d++, 4, APRO_CEVAL_CODE_EVAL_RISING);
	if(result) return result;
	result = aga_graph_plot(graph, d++, 4, APRO_CEVAL_CODE_EVAL);
	if(result) return result;
	result = aga_graph_plot(graph, d++, 4, APRO_CEVAL_CODE_EVAL_FALLING);
	if(result) return result;
	result = aga_graph_plot(graph, d++, 1, APRO_RES_SWEEP);
	if(result) return result;
	result = aga_graph_plot(graph, d, 1, APRO_AUDIO_UPDATE);
	if(result) return result;

	result = aga_graph_plot(graph, x++, 12, APRO_SCRIPTGLUE_GETKEY);
	if(result) return result;
	result = aga_graph_plot(graph, x++, 12, APRO_SCRIPTGLUE_GETMOTION);
	if(result) return result;
	result = aga_graph_plot(graph, x++, 12, APRO_SCRIPTGLUE_SETCURSOR);
	if(result) return result;
	result = aga_graph_plot(graph, x++, 12, APRO_SCRIPTGLUE_SETCAM);
	if(result) return result;
	result = aga_graph_plot(graph, x++, 12, APRO_SCRIPTGLUE_TEXT);
	if(result) return result;
	result = aga_graph_plot(graph, x++, 12, APRO_SCRIPTGLUE_FOGPARAM);
	if(result) return result;
	result = aga_graph_plot(graph, x++, 12, APRO_SCRIPTGLUE_FOGCOL);
	if(result) return result;
	result = aga_graph_plot(graph, x++, 12, APRO_SCRIPTGLUE_CLEAR);
	if(result) return result;
	result = aga_graph_plot(graph, x++, 12, APRO_SCRIPTGLUE_MKTRANS);
	if(result) return result;
	result = aga_graph_plot(graph, x++, 12, APRO_SCRIPTGLUE_GETCONF);
	if(result) return result;
	result = aga_graph_plot(graph, x++, 12, APRO_SCRIPTGLUE_LOG);
	if(result) return result;
	result = aga_graph_plot(graph, x++, 12, APRO_SCRIPTGLUE_DIE);
	if(result) return result;
	result = aga_graph_plot(graph, x++, 12, APRO_SCRIPTGLUE_MKOBJ);
	if(result) return result;
	result = aga_graph_plot(graph, x++, 12, APRO_SCRIPTGLUE_INOBJ);
	if(result) return result;
	result = aga_graph_plot(graph, x++, 12, APRO_SCRIPTGLUE_PUTOBJ);
	if(result) return result;
	result = aga_graph_plot(graph, x++, 12, APRO_SCRIPTGLUE_KILLOBJ);
	if(result) return result;
	result = aga_graph_plot(graph, x++, 12, APRO_SCRIPTGLUE_OBJTRANS);
	if(result) return result;
	result = aga_graph_plot(graph, x++, 12, APRO_SCRIPTGLUE_OBJCONF);
	if(result) return result;
	result = aga_graph_plot(graph, x++, 12, APRO_SCRIPTGLUE_BITAND);
	if(result) return result;
	result = aga_graph_plot(graph, x++, 12, APRO_SCRIPTGLUE_BITSHL);
	if(result) return result;
	result = aga_graph_plot(graph, x++, 12, APRO_SCRIPTGLUE_RANDNORM);
	if(result) return result;
	result = aga_graph_plot(graph, x, 12, APRO_SCRIPTGLUE_PLAYSND);
	if(result) return result;

	result = aga_graph_plot(graph, n++, 20, APRO_PUTOBJ_RISING);
	if(result) return result;
	result = aga_graph_plot(graph, n++, 20, APRO_PUTOBJ_LIGHT);
	if(result) return result;
	result = aga_graph_plot(graph, n++, 20, APRO_PUTOBJ_CALL);
	if(result) return result;
	result = aga_graph_plot(graph, n, 20, APRO_PUTOBJ_FALLING);
	if(result) return result;

	if(graph->inter >= graph->period) {
		graph->inter = 0;
		asys_memory_zero(graph->running, APRO_MAX * sizeof(apro_unit_t));
	}

	mil_gl_swap(mil, graph->gl_area);

	return ASYS_RESULT_OK;
#else
	(void) graph;
	(void) env;

	return ASYS_RESULT_OK;
#endif
}

/* TODO: Controls to select+compare certain stats by themselves/highlighted. */
enum asys_result aga_graph_plot(
		struct aga_graph* graph, unsigned y, unsigned x, enum apro_section s) {

#ifdef AGA_DEVBUILD
	static const float width = 0.1f;

	float color[4];

	enum asys_result result;
	asys_bool_t shift;
	apro_unit_t* history;
	apro_unit_t us = apro_stamp_us(s);

	if(!graph) return ASYS_RESULT_BAD_PARAM;

	color[0] = (float) (s % 3) / 2.0f;
	color[1] = (float) (s % 7) / 6.0f;
	color[2] = (float) (s % 13) / 12.0f;
	color[3] = 1.0f;

	shift = graph->inter >= graph->period;
	history = &graph->histories[s * graph->segments];

	graph->running[s] += us;

	/* Graph. */
	{
		asys_size_t i;
		double height;

		for(i = 0; i < graph->segments; ++i) {
			if(shift) history[i] = history[i + 1];

			height = (double) history[i] / (double) graph->max;
			graph->heights[i] = (float) height;
		}

		if(shift) {
			history[graph->segments - 1] = graph->running[s] / graph->period;
			height = (double) us / (double) graph->max;
			graph->heights[graph->segments - 1] = (float) height;
		}

		result = aga_render_line_graph(
				graph->heights, graph->segments, width, color);

		if(result) return result;
	}

	/* Textual Overlay. */
	{
		float tx, ty;

		tx = 0.01f + (0.035f * (float) x);
		ty = 0.05f + (0.035f * (float) y);

		result = aga_render_text_format(
				tx, ty, color, "%s: %llu",
				apro_section_name(s), history[graph->segments - 1]);

		if(result) return result;
	}
#else
	(void) graph;
	(void) y;
	(void) x;
	(void) s;
#endif

	return ASYS_RESULT_OK;
}
