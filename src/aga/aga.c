/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

/* TODO: Fuzz headless. */
/* TODO: Isolate agabuild file input types and test individually. */

#include <aga/window.h>
#include <aga/sound.h>
#include <aga/pack.h>
#include <aga/midi.h>
#include <aga/render.h>
#include <aga/draw.h>
#include <aga/startup.h>
#include <aga/script.h>
#include <aga/build.h>
#include <aga/graph.h>

#include <apro.h>

#include <asys/log.h>
#include <asys/error.h>
#include <asys/string.h>
#include <asys/main.h>

static enum asys_result aga_put_default(void) {
	/*
	 * TODO: We can definitely work on making this more useful. Maybe
	 * 		 An interactive way to load a project once we have some
	 * 		 Semblance of UI?
	 */

	enum asys_result result;

	static const char str1[] = "No project loaded or no script files provided";
	static const char str2[] = "Did you forget `-f' or `-C'?";
	static const float text_color[] = {1.0f, 1.0f, 1.0f, 1.0f};
	static const float color[] = {0.6f, 0.3f, 0.8f, 1.0f};

	if ((result = aga_render_clear(color))) return result;

	result = aga_render_text_format(0.05f, 0.1f, text_color, str1);
	if (result) return result;

	return aga_render_text_format(0.05f, 0.2f, text_color, str2);
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

	struct aga_window_device env;
	struct aga_window win;
	struct aga_keymap keymap;
	struct aga_pointer pointer;
	struct aga_buttons buttons = { 0 };

	struct aga_script_engine script_engine;
	struct aga_script_class class = { 0 };
	struct aga_script_instance inst;

	enum aga_draw_flags draw_flags = AGA_DRAW_BACKFACE | AGA_DRAW_FOG |
								   AGA_DRAW_TEXTURE | AGA_DRAW_LIGHTING |
								   AGA_DRAW_DEPTH | AGA_DRAW_FLAT;

	asys_bool_t die = ASYS_FALSE;
	apro_unit_t dt = 0;

	const char* gl_version;

	/* TODO: CLI opt for this. */
	/* TODO: Fix this */
	asys_bool_t do_prof = ASYS_FALSE; /* !!aga_getenv("AGA_DOPROF") */
	struct aga_graph prof = { 0 };

	struct aga_script_userdata userdata;

	userdata.keymap = &keymap;
	userdata.pointer = &pointer;
	userdata.opts = &opts;
	userdata.sound_device = &snd;
	userdata.die = &die;
	userdata.window_device = &env;
	userdata.window = &win;
	userdata.resource_pack = &pack;
	userdata.buttons = &buttons;
	userdata.dt = &dt;

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

	result = aga_resource_pack_new(opts.respack, &pack);
	asys_log_result(__FILE__, "aga_resource_pack_new", result);

	/*
	 * TODO: Trace each resource load (from `pack.c' not here) in verbose mode.
	 */
	/*{
		asys_size_t i;

		for(i = 0; i < pack.count; ++i) {
			const char* name = pack.resources[i].config->name;

			asys_log(__FILE__, "%s", name);
		}

		result = aga_resource_pack_sweep(&pack);
		asys_log_result(__FILE__, "aga_resource_pack_sweep", result);
	}*/

	result = aga_settings_parse_config(&opts, &pack);
	asys_log_result(__FILE__, "aga_settings_parse_config", result);

	asys_log(__FILE__, "Initializing systems...");

	result = aga_window_device_new(&env, opts.display);
	asys_result_check(__FILE__, "aga_window_device_new", result);

	result = aga_keymap_new(&keymap, &env);
	asys_result_check(__FILE__, "aga_keymap_new", result);

	if(do_prof) {
		result = aga_graph_new(&prof, &env, main_data);
		if(result) do_prof = ASYS_FALSE;
	}

	result = aga_window_new(
			opts.width, opts.height, opts.title, &env, &win, ASYS_TRUE,
			main_data);

	asys_result_check(__FILE__, "aga_window_new", result);

	result = aga_renderer_string(&gl_version);
	asys_log_result(__FILE__, "aga_renderer_string", result);
	asys_log(
			__FILE__, "Acquired GL context: %s",
			gl_version ? gl_version : "<error>");

	asys_result_check(__FILE__, "aga_draw_set", aga_draw_set(draw_flags));

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
			&userdata);

	asys_log_result(__FILE__, "aga_script_engine_new", result);
	if(!result) {
		asys_log(__FILE__, "Instantiating game instance...");

		result = aga_script_engine_lookup(&script_engine, &class, "game");
		asys_result_check(__FILE__, "aga_script_engine_lookup", result);

		result = aga_script_instance_new(&class, &inst);
		asys_result_check(__FILE__, "aga_script_instance_new", result);

		result = aga_script_instance_call(
				&script_engine, &inst, AGA_SCRIPT_CREATE);

		asys_log_result(__FILE__, "aga_script_instance_call", result);
	}

	asys_log(__FILE__, "Done!");

	while(!die) {
		/* TODO: Fix more formal ref/obj tracing for devbuilds. */

		result = aga_window_select(&env, &win);
		asys_log_result(__FILE__, "aga_window_select", result);

		apro_stamp_start(APRO_PRESWAP);
		{
			apro_stamp_start(APRO_POLL);
			{
				pointer.dx = 0;
				pointer.dy = 0;

				result = aga_window_device_poll(
						&env, &keymap, &win, &pointer, &die, &buttons);

				asys_log_result(
						__FILE__, "aga_window_device_poll", result);
			}
			apro_stamp_end(APRO_POLL);

			apro_stamp_start(APRO_SCRIPT_UPDATE);
			{
				if(class.class) {
					result = aga_script_instance_call(
							&script_engine, &inst, AGA_SCRIPT_UPDATE);

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
				result = aga_resource_pack_sweep(&pack);
				asys_log_result(
						__FILE__, "aga_resource_pack_sweep", result);
			}
			apro_stamp_end(APRO_RES_SWEEP);
		}
		apro_stamp_end(APRO_PRESWAP);

		/* TODO: This doesn't work under devbuilds. */
		dt = (asys_size_t) apro_stamp_us(APRO_PRESWAP);

		if(do_prof) {
			result = aga_graph_update(&prof, &env);
			asys_log_result(__FILE__, "aga_graph_update", result);
		}

		apro_clear();

		/* Window is already dead/dying if `die' is set. */
		if(!die) {
			result = aga_window_swap(&env, &win);
			asys_log_result(__FILE__, "aga_window_swap", result);
		}
	}

	asys_log(__FILE__, "Tearing down...");

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

	result = aga_window_delete(&env, &win);
	asys_log_result(__FILE__, "aga_window_delete", result);

	if(do_prof) {
		result = aga_graph_delete(&prof, &env);
		asys_log_result(__FILE__, "aga_window_delete", result);
	}

	result = aga_keymap_delete(&keymap);
	asys_log_result(__FILE__, "aga_keymap_delete", result);

	/*
	 * NOTE: Windows needs to process final Window messages for `WM_DESTROY'
	 * 		 Before teardown.
	 */
	/*
	 * TODO: Currently broken under multiwindow -- need to explicitly poll a
	 * 		 Window that hasn't been closed.
	 */
	/*
	result = aga_window_device_poll(
			&env, &keymap, &win, &pointer, &die, &buttons);
	asys_log_result(__FILE__, "aga_window_device_poll", result);
	 */

	result = aga_window_device_delete(&env);
	asys_log_result(__FILE__, "aga_window_device_delete", result);

	result = aga_resource_pack_delete(&pack);
	asys_log_result(__FILE__, "aga_resource_pack_delete", result);

	asys_log(__FILE__, "Bye-bye!");

	return ASYS_RESULT_OK;
}
