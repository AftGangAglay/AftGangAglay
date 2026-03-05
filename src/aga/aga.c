/*
 * SPDX-License-Identifier: X11
 * Copyright (C) 2023-2025 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

/* TODO: Fuzz headless. */
/* TODO: Test build input types individually. */

#include <aga/userdata.h>
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
#include <asys/control.h>

#include <mil/mil.h>
#include <mil/widget.h>
#include <mil/gl.h>
#include <mil/translate.h>

#include <python/object.h>
#include <python/object/class.h>
#include <python/object/dict.h>
#include <python/evalops.h>
#include <python/errors.h>

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

void aga_main_window_input(
		mil_widget_t widget, struct mil_ctx* mil, void* data) {

	struct aga_mil_userdata* userdata = mil->user;

	struct mil_input_data input;

	(void) widget;

	mil_get_input_data(mil, data, &input);

	aga_translate_mil_input(&input, userdata->input);
}

static mil_widget_t aga_setup_main_window(struct mil_ctx* mil) {
	mil_widget_t window, area;

	window = mil_widget(
			mil, mil->settings.title, MIL_MAIN_WINDOW, mil->top, MIL_END);

	area = mil_widget(
			mil, "gl_area", MIL_DRAWING_AREA, window,
			/* TODO: Resizing. */
			/* MIL_DRAWING_AREA_RESIZE_CALLBACK, area_resize, */
			MIL_DRAWING_AREA_INPUT_CALLBACK, aga_main_window_input,
			MIL_END);

	return area;
}

static enum asys_result aga_setup_script(struct mil_ctx* mil) {
	struct aga_mil_userdata* userdata = mil->user;

	struct py_object* method;

	method = py_class_member_get_attr(userdata->script_instance, "create");
	if(!method) {
		asys_log(__FILE__, "warn: `game' class has no `create' method");
	}
	else {
		py_object_decref(py_call_function(
				userdata->script_engine->env, method, 0));

		py_object_decref(method);
		if(py_error_occurred()) {
			aga_script_engine_trace();
			return ASYS_RESULT_ERROR;
		}
	}

	userdata->script_update = py_class_member_get_attr(
			userdata->script_instance, "update");

	if(!userdata->script_update) {
		asys_log(__FILE__, "warn: `game' class has no `update' method");
		return ASYS_RESULT_MISSING_KEY;
	}

	return ASYS_RESULT_OK;
}

static void aga_frame_zero(struct mil_ctx* mil) {
	struct aga_mil_userdata* userdata = mil->user;

	enum aga_draw_flags draw_flags = AGA_DRAW_BACKFACE | AGA_DRAW_FOG |
										AGA_DRAW_TEXTURE | AGA_DRAW_LIGHTING |
										AGA_DRAW_DEPTH | AGA_DRAW_FLAT;

	enum asys_result result;

	const char* gl_version;

	if(!userdata->settings->headless) {
		result = aga_renderer_string(&gl_version);
		asys_log_result(__FILE__, "aga_renderer_string", result);
		asys_log(
				__FILE__, "Acquired GL context: %s",
				gl_version ? gl_version : "<error>");

		asys_result_check(__FILE__, "aga_draw_set", aga_draw_set(draw_flags));

		result = mil_gl_load_font(mil, userdata->gl_area, AGA_FONT_LIST_BASE);
		asys_log_result(__FILE__, "mil_gl_load_font", result);
	}

	if(userdata->script_engine->global) {
		result = aga_setup_script(mil);
		asys_log_result(__FILE__, "aga_instantiate_script", result);
	}
}

static void aga_update(struct mil_ctx* mil) {
	struct aga_mil_userdata* userdata = mil->user;

	enum asys_result result;
	unsigned width, height;

	/* TODO: Fix more formal ref/obj tracing for devbuilds. */

	/* Fixup legacy input mapping click vs. down. */
	{
		static struct aga_buttons buttons = { 0 };

		asys_size_t i;
		for(i = 0; i < ASYS_LENGTH(buttons.states); ++i) {
			enum aga_button_state* state = &userdata->input->buttons.states[i];
			if(*state == AGA_BUTTON_CLICK &&
					buttons.states[i] == AGA_BUTTON_CLICK) {

				*state = AGA_BUTTON_DOWN;
			}

			buttons.states[i] = *state;
		}
	}

	if(!userdata->settings->headless) {
		result = mil_gl_context_widget(mil, userdata->gl_area);
		asys_log_result(__FILE__, "mil_gl_context_widget", result);

		mil_widget_get_size(mil, userdata->gl_area, &width, &height);

		aga_wrap_pointer(
				mil, userdata->gl_area, &userdata->input->pointer,
				(int) width, (int) height);
	}

	if(userdata->frame_zero) {
		aga_frame_zero(mil);
		userdata->frame_zero = ASYS_FALSE;
	}

	if(!userdata->settings->headless) {
		result = aga_render_area(0, 0, width, height);
		asys_log_result(__FILE__, "aga_render_area", result);
	}

	apro_stamp_start(APRO_PRESWAP);
	{
		apro_stamp_start(APRO_SCRIPT_UPDATE);
		{
			if(userdata->script_update) {
				py_object_decref(py_call_function(
						userdata->script_engine->env, userdata->script_update,
						0));

				if(py_error_occurred()) aga_script_engine_trace();
			}
			else {
				result = aga_put_default();
				asys_log_result(__FILE__, "aga_put_default", result);
			}
		}
		apro_stamp_end(APRO_SCRIPT_UPDATE);

		/*
		 * TODO: Delay this to only trigger every N frames -- user
		 *		 Configurable.
		 */
		apro_stamp_start(APRO_RES_SWEEP);
		{
			result = aga_resource_pack_sweep(userdata->resource_pack);
			asys_log_result(
					__FILE__, "aga_resource_pack_sweep", result);

			if(!userdata->settings->headless) {
				result = aga_sound_device_sweep(userdata->sound_device);
				asys_log_result(
						__FILE__, "aga_sound_device_sweep", result);
			}
		}
		apro_stamp_end(APRO_RES_SWEEP);
	}
	apro_stamp_end(APRO_PRESWAP);

	apro_stamp_start(APRO_AUDIO_UPDATE);
	if(userdata->settings->audio_enabled && !userdata->settings->headless) {
		result = aga_sound_device_update(userdata->sound_device);
		asys_log_result(
				__FILE__, "aga_sound_device_update", result);
	}
	apro_stamp_end(APRO_AUDIO_UPDATE);

	/* TODO: This needs to be fixed. */
	/* dt = (asys_size_t) apro_stamp_us(APRO_PRESWAP); */

	if(userdata->settings->profiler && !userdata->settings->headless) {
		result = aga_graph_update(userdata->profile_graph, mil);
		asys_log_result(__FILE__, "aga_graph_update", result);
	}

	apro_clear();

	if(!userdata->settings->headless) {
		result = mil_gl_swap(mil, userdata->gl_area);
		asys_log_result(__FILE__, "mil_gl_swap", result);
	}

	if(*userdata->die && !userdata->settings->headless) mil_stop(mil);
}

static enum asys_result aga_class_script_instance(
		struct py_object* global, const char* name,
		struct py_object** instance) {

	struct py_object* class = py_dict_lookup(global, name);
	if(!class) {
		asys_log(__FILE__, "err: Script is missing `%s' class", name);
		asys_log_result(__FILE__, "py_dict_lookup", ASYS_RESULT_MISSING_KEY);
		return ASYS_RESULT_MISSING_KEY;
	}

	*instance = py_class_member_new(class);
	py_object_decref(class);
	if(!*instance) {
		asys_log_result(__FILE__, "py_class_member_new", ASYS_RESULT_OOM);
		return ASYS_RESULT_OOM;
	}

	return ASYS_RESULT_OK;
}

static void aga_interrupt_handler(void* userdata) {
	asys_clear_user_interrupt_handler();

	*(asys_bool_t*) userdata = ASYS_TRUE;

	asys_set_user_interrupt_handler(aga_interrupt_handler, userdata);
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

	struct aga_mil_userdata mil_userdata = { 0 };

	struct mil_settings mil_opts;
	struct mil_ctx mil;

	struct aga_input_pack input = { 0 };

	struct aga_script_engine script_engine;

	asys_bool_t die = ASYS_FALSE;
	apro_unit_t dt = 0;

	struct aga_graph prof = { 0 };

	struct aga_script_userdata script_userdata = { 0 };

	script_userdata.opts = &opts;
	script_userdata.sound_device = &snd;
	script_userdata.resource_pack = &pack;
	script_userdata.die = &die;
	script_userdata.dt = &dt;
	script_userdata.input = &input;
	script_userdata.mil = &mil;

	mil.user = &mil_userdata;

	mil_userdata.script_engine = &script_engine;
	mil_userdata.resource_pack = &pack;
	mil_userdata.settings = &opts;
	mil_userdata.sound_device = &snd;
	mil_userdata.profile_graph = &prof;
	mil_userdata.frame_zero = ASYS_TRUE;
	mil_userdata.input = &input;
	mil_userdata.die = &die;

	asys_log(__FILE__, "Breathing in the chemicals...");

	result = aga_settings_new(&opts, main_data);
	asys_log_result(__FILE__, "aga_settings_new", result);

#ifdef AGA_DEVBUILD
	/* TODO: Resource pack viewer/viz/decompilation */
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

	if(!opts.headless) {
		result = mil_ctx_new(&mil, &mil_opts, main_data);
		if(result) return result;
	}

	if(opts.audio_enabled && !opts.headless) {
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

    if(script_engine.global) {
        asys_log(__FILE__, "Instantiating game instance...");
        result = aga_class_script_instance(
                script_engine.global, "game", &mil_userdata.script_instance);

        asys_log_result(__FILE__, "aga_class_script_instance", result);

#ifdef AGA_DEVBUILD
		if(!opts.headless) {
    		result = aga_class_script_instance(
	                script_engine.global, "ui", &mil_userdata.ui_instance);

	        asys_log_result(__FILE__, "aga_class_script_instance", result);

	        if(!result) {
	            mil_userdata.ui_activate = py_class_member_get_attr(
	                    mil_userdata.ui_instance, "activate");

	            if(!mil_userdata.ui_activate) {
	                asys_log(
	                		__FILE__,
							"warn: `ui' class has no `activate' method");
	            }
	        }
		}
#endif
    }

	if(!opts.headless) {
#ifdef AGA_DEVBUILD
		struct py_object* method;
		asys_bool_t can_ui;
#endif

		asys_log(__FILE__, "Creating main window...");

#ifdef AGA_DEVBUILD
		method = py_class_member_get_attr(mil_userdata.ui_instance, "create");

		can_ui = mil_userdata.ui_instance && method;
		if(can_ui) {
			py_object_decref(py_call_function(
					script_engine.env, method, mil_userdata.script_instance));

			py_object_decref(method);

			if(py_error_occurred()) aga_script_engine_trace();
		}
		else if(!method) {
			asys_log(
					__FILE__,
					"warn: `ui' class has no `create' method, using default "
					"main window layout");
		}
#endif

		if(!mil_userdata.gl_area) {
			mil_userdata.gl_area = aga_setup_main_window(&mil);
		}

		script_userdata.gl_area = mil_userdata.gl_area;
	}

	/* TODO: Headless profiler. */
	if(opts.profiler && !opts.headless) {
		result = aga_graph_new(&prof, &mil);
		if(result) {
			asys_result_check(__FILE__, "aga_graph_new", result);
			opts.profiler = ASYS_FALSE;
		}
	}

	asys_log_result(__FILE__, "aga_script_engine_new", result);

	asys_set_user_interrupt_handler(aga_interrupt_handler, &die);

	asys_log(__FILE__, "Done!");

	/* TODO: Store result. */
	if(!opts.headless) {
		mil_start(&mil);
	}
	else {
		while(!die) {
			aga_update(&mil);
		}
	}

	asys_log(__FILE__, "Tearing down...");

	if(!opts.headless) {
		mil_ctx_delete(&mil);
	}

	/* TODO: Add `asys' Apple OSX/OS9 detection. */
	/* TODO: Move this out to mil. */
#ifdef __APPLE__
	if(!opts.headless) {
		/* Need to flush before shutdown to avoid NSGL dying */
		asys_log_result(__FILE__, "aga_render_flush", aga_render_flush());
	}
#endif

	if(mil_userdata.script_instance) {
		struct py_object* method;

		py_object_decref(mil_userdata.script_update);

		method = py_class_member_get_attr(
				mil_userdata.script_instance, "close");

		if(!method) {
			asys_log(__FILE__, "warn: `game' class has no `close' method");
		}
		else {
			py_object_decref(py_call_function(script_engine.env, method, 0));
			py_object_decref(method);

			if(py_error_occurred()) aga_script_engine_trace();
		}
	}

	result = aga_script_engine_delete(&script_engine);
	asys_log_result(__FILE__, "aga_script_engine_delete", result);

	if(opts.audio_enabled && !opts.headless) {
		result = aga_sound_device_delete(&snd);
		asys_log_result(__FILE__, "aga_sound_device_delete", result);
	}

	result = aga_config_delete(&opts.config);
	asys_log_result(__FILE__, "aga_config_delete", result);

	if(opts.profiler && !opts.headless) {
		result = aga_graph_delete(&prof);
		asys_log_result(__FILE__, "aga_window_delete", result);
	}

	result = aga_resource_pack_delete(&pack);
	asys_log_result(__FILE__, "aga_resource_pack_delete", result);

	asys_log(__FILE__, "Bye-bye!");

	return ASYS_RESULT_OK;
}
