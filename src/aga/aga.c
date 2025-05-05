/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

/* TODO: Fuzz headless. */
/* TODO: Test build input types individually. */

#include <aga/sound.h>
#include <aga/pack.h>
#include <aga/midi.h>
#include <aga/render.h>
#include <aga/draw.h>
#include <aga/startup.h>
#include <aga/script.h>
#include <aga/build.h>
#include <aga/graph.h>
#include <aga/input.h>

#include <apro.h>

#include <asys/log.h>
#include <asys/error.h>
#include <asys/string.h>
#include <asys/main.h>

#include <mil/mil.h>
#include <mil/widget.h>
#include <mil/gl.h>
/* TODO: Move out input translation somewhere else. */
#include <mil/translate.h>

struct aga_mil_userdata {
	struct mil_drawing_area_input_storage input_storage;
	mil_widget_t gl_area;

	struct aga_script_engine* script_engine;
	struct aga_script_class* script_class;
	struct aga_script_instance* script_instance;

	struct aga_resource_pack* resource_pack;

	struct aga_sound_device* sound_device;

	struct aga_settings* settings;

	struct aga_graph* profile_graph;

	struct aga_input_pack* input;

	asys_bool_t frame_zero;
};

static enum asys_result aga_put_default(void) {
	/*
	 * TODO: We can definitely work on making this more useful. Maybe
	 * 		 An interactive way to load a project once we have some
	 * 		 Semblance of UI?
	 */

	enum asys_result result;

	static const char str1[] = "No project loaded or no script files provided";
	static const char str2[] = "Did you forget `-f' or `-C'?";
	static const float text_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const float color[] = { 0.6f, 0.3f, 0.8f, 1.0f };

	if ((result = aga_render_clear(color))) return result;

	result = aga_render_text_format(0.05f, 0.1f, text_color, str1);
	if(result) return result;

	return aga_render_text_format(0.05f, 0.2f, text_color, str2);
}

/*
 * TODO: Set dirty flag on pointer deltas and only reset if there was no input
 * 		 That frame.
 */
static void aga_main_window_input(
		mil_widget_t widget, struct mil_ctx* mil, void* data) {

	struct aga_mil_userdata* userdata = mil->user;

	struct mil_input_data input;

	(void) widget;

	mil_get_input_data(mil, data, &input);

	/*
	 * TODO: Just send the event to script land and deprecate our input procs.
	 */

	aga_translate_mil_input(&input, userdata->input);
}

static mil_widget_t aga_setup_main_window(struct mil_ctx* mil) {
	struct aga_mil_userdata* userdata = mil->user;

	mil_widget_t window, frame, area;

	window = mil_widget(
			mil, mil->settings.title, MIL_MAIN_WINDOW, mil->top, MIL_END);

	frame = mil_widget(mil, "frame", MIL_FRAME, window, MIL_END);

	userdata->input_storage.callback = aga_main_window_input;
	userdata->input_storage.ctx = mil;

	area = mil_widget(
			mil, "gl_area", MIL_DRAWING_AREA, frame,
			/* TODO: Resizing. */
			/* MIL_DRAWING_AREA_RESIZE_CALLBACK, area_resize, */
			MIL_DRAWING_AREA_INPUT_CALLBACK, &userdata->input_storage,
			MIL_END);

	return area;
}

static void aga_frame_zero(struct mil_ctx* mil) {
	struct aga_mil_userdata* userdata = mil->user;

	enum aga_draw_flags draw_flags = AGA_DRAW_BACKFACE | AGA_DRAW_FOG |
										AGA_DRAW_TEXTURE | AGA_DRAW_LIGHTING |
										AGA_DRAW_DEPTH | AGA_DRAW_FLAT;

	enum asys_result result;

	const char* gl_version;

	result = aga_renderer_string(&gl_version);
	asys_log_result(__FILE__, "aga_renderer_string", result);
	asys_log(
			__FILE__, "Acquired GL context: %s",
			gl_version ? gl_version : "<error>");

	asys_result_check(__FILE__, "aga_draw_set", aga_draw_set(draw_flags));

	result = mil_gl_load_font(mil, AGA_FONT_LIST_BASE);
	asys_log_result(__FILE__, "mil_gl_load_font", result);

	asys_log(__FILE__, "Instantiating game instance...");

	result = aga_script_engine_lookup(
			userdata->script_engine, userdata->script_class, "game");

	asys_result_check(__FILE__, "aga_script_engine_lookup", result);

	result = aga_script_instance_new(
			userdata->script_class, userdata->script_instance);

	asys_result_check(__FILE__, "aga_script_instance_new", result);

	result = aga_script_instance_call(
			userdata->script_engine, userdata->script_instance,
			AGA_SCRIPT_CREATE);

	asys_log_result(__FILE__, "aga_script_instance_call", result);
}

static void aga_update(struct mil_ctx* mil) {
	struct aga_mil_userdata* userdata = mil->user;

	enum asys_result result;
	unsigned width, height;

	/* TODO: Fix more formal ref/obj tracing for devbuilds. */

	result = mil_gl_context_widget(mil, userdata->gl_area);
	asys_log_result(__FILE__, "mil_gl_context_widget", result);

	mil_widget_get_size(mil, userdata->gl_area, &width, &height);

	aga_wrap_pointer(
			mil, userdata->gl_area, &userdata->input->pointer,
			(int) width, (int) height);

	if(userdata->frame_zero) {
		aga_frame_zero(mil);
		userdata->frame_zero = ASYS_FALSE;
	}

	result = aga_render_area(0, 0, width, height);
	asys_log_result(__FILE__, "aga_render_area", result);

	apro_stamp_start(APRO_PRESWAP);
	{
		apro_stamp_start(APRO_SCRIPT_UPDATE);
		{
			if(userdata->script_class->class) {
				result = aga_script_instance_call(
						userdata->script_engine, userdata->script_instance,
						AGA_SCRIPT_UPDATE);

				asys_log_result(
						__FILE__, "aga_script_instance_call", result);
			}
			else {
				result = aga_put_default();
				asys_log_result(__FILE__, "aga_put_default", result);
			}
		}
		apro_stamp_end(APRO_SCRIPT_UPDATE);

		apro_stamp_start(APRO_RES_SWEEP);
		{
			result = aga_resource_pack_sweep(userdata->resource_pack);
			asys_log_result(
					__FILE__, "aga_resource_pack_sweep", result);
		}
		apro_stamp_end(APRO_RES_SWEEP);
	}
	apro_stamp_end(APRO_PRESWAP);

	apro_stamp_start(APRO_AUDIO_UPDATE);
	if(userdata->settings->audio_enabled) {
		result = aga_sound_device_update(userdata->sound_device);
		asys_log_result(
				__FILE__, "aga_sound_device_update", result);
	}
	apro_stamp_end(APRO_AUDIO_UPDATE);

	/* TODO: This needs to be fixed. */
	/* dt = (asys_size_t) apro_stamp_us(APRO_PRESWAP); */

	if(userdata->settings->profiler) {
		result = aga_graph_update(userdata->profile_graph, mil);
		asys_log_result(__FILE__, "aga_graph_update", result);
	}

	apro_clear();

	mil_gl_swap(mil, userdata->gl_area);
}

/*
 * TODO: We appear to have a memory leak (at least on Windows) which consumes
 * 		 Hundreds of MiBs in seconds. Probably leaking a script engine
 * 		 Reference.
 */
enum asys_result asys_main(struct asys_main_data* main_data) {
	enum asys_result result;

	struct aga_settings opts;

	struct aga_resource_pack pack;

	struct aga_sound_device snd;
	struct aga_midi_device midi;

	struct aga_mil_userdata mil_userdata;

	struct mil_settings mil_opts;
	struct mil_ctx mil;

	struct aga_input_pack input = { 0 };

	struct aga_script_engine script_engine;
	struct aga_script_class class = { 0 };
	struct aga_script_instance inst;

	asys_bool_t die = ASYS_FALSE;
	apro_unit_t dt = 0;

	struct aga_graph prof = { 0 };

	struct aga_script_userdata script_userdata;

	script_userdata.opts = &opts;
	script_userdata.sound_device = &snd;
	script_userdata.resource_pack = &pack;
	script_userdata.die = &die;
	script_userdata.dt = &dt;
	script_userdata.input = &input;
	script_userdata.mil = &mil;

	mil.user = &mil_userdata;

	mil_userdata.script_engine = &script_engine;
	mil_userdata.script_class = &class;
	mil_userdata.script_instance = &inst;
	mil_userdata.resource_pack = &pack;
	mil_userdata.settings = &opts;
	mil_userdata.sound_device = &snd;
	mil_userdata.profile_graph = &prof;
	mil_userdata.frame_zero = ASYS_TRUE;
	mil_userdata.input = &input;

	asys_log(__FILE__, "Breathing in the chemicals...");

	result = aga_settings_new(&opts, main_data);
	asys_log_result(__FILE__, "aga_settings_new", result);

#ifdef AGA_DEVBUILD
	if(opts.compile) {
		result = aga_build(&opts);
		asys_log_result(__FILE__, "aga_build", result);
		asys_log(__FILE__, "Bye-bye!");
		return result;
	}
#endif

	result = aga_resource_pack_new(opts.respack, &pack, &opts);
	asys_log_result(__FILE__, "aga_resource_pack_new", result);

	result = aga_settings_parse_config(&opts, &pack);
	asys_log_result(__FILE__, "aga_settings_parse_config", result);

	asys_log(__FILE__, "Initializing systems...");

	mil_opts.title = opts.title;
	mil_opts.min_width = opts.width;
	mil_opts.min_height = opts.height;

	mil.update = aga_update;

	mil_ctx_new(&mil, &mil_opts, &main_data->argc, main_data->argv);

	mil_userdata.gl_area = aga_setup_main_window(&mil);
	script_userdata.gl_area = mil_userdata.gl_area;

	if(opts.profiler) {
		result = aga_graph_new(&prof, &mil);
		if(result) {
			asys_result_check(__FILE__, "aga_graph_new", result);
			opts.profiler = ASYS_FALSE;
		}
	}

	if(opts.audio_enabled) {
		if((result = aga_sound_device_new(&snd, opts.audio_buffer))) {
			asys_log_result(__FILE__, "aga_sound_device_new", result);
			/* TODO: Separate "unavailable snd/midi" and user defined. */
			opts.audio_enabled = ASYS_FALSE;
		}
	}

	/* TODO: Work on MIDI. */
	/* TODO: This is only a hard error while it's WIP. */
	(void) midi;
	/*
	result = aga_midi_device_new(&midi);
	asys_result_check(__FILE__, "aga_midi_device_new", result);
	{
		struct aga_resource* mres;
		struct aga_midi m;

		result = aga_resource_new(&pack, "snd/sndtest.mid.raw", &mres);
		asys_result_check(__FILE__, "aga_resource_new", result);

		result = aga_midi_new(&midi, &m, mres->data, mres->size);
		asys_result_check(__FILE__, "aga_midi_new", result);

		result = aga_midi_play(&midi, &m);
		asys_result_check(__FILE__, "aga_midi_play", result);
	}*/

	if(!asys_string_equal(opts.version, AGA_VERSION)) {
		asys_log(
				__FILE__,
				"warn: Project version `%s' does not match engine version `"
				AGA_VERSION "'",
				opts.version);
	}

	asys_log(__FILE__, "Starting up the script engine...");

	result = aga_script_engine_new(
			&script_engine, opts.startup_script, &pack, opts.python_path,
			&script_userdata);

	asys_log_result(__FILE__, "aga_script_engine_new", result);

	asys_log(__FILE__, "Done!");

	mil_start(&mil);

	asys_log(__FILE__, "Tearing down...");

	mil_ctx_delete(&mil);

	/* TODO: Add `asys' Apple OSX/OS9 detection. */
#ifdef __APPLE__
	/* Need to flush before shutdown to avoid NSGL dying */
	asys_log_result(__FILE__, "aga_render_flush", aga_render_flush());
#endif

	if(class.class) {
		result = aga_script_instance_call(
				&script_engine, &inst, AGA_SCRIPT_CLOSE);

		asys_log_result(__FILE__, "aga_script_instance_call", result);

		result = aga_script_instance_delete(&inst);
		asys_log_result(__FILE__, "aga_script_instance_delete", result);
	}

	result = aga_script_engine_delete(&script_engine);
	asys_log_result(__FILE__, "aga_script_engine_delete", result);

	if(opts.audio_enabled) {
		result = aga_sound_device_delete(&snd);
		asys_log_result(__FILE__, "aga_sound_device_delete", result);
	}

	result = aga_config_delete(&opts.config);
	asys_log_result(__FILE__, "aga_config_delete", result);

	if(opts.profiler) {
		result = aga_graph_delete(&prof);
		asys_log_result(__FILE__, "aga_window_delete", result);
	}

	result = aga_resource_pack_delete(&pack);
	asys_log_result(__FILE__, "aga_resource_pack_delete", result);

	asys_log(__FILE__, "Bye-bye!");

	return ASYS_RESULT_OK;
}
