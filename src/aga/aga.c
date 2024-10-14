/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

/* TODO: Fuzz headless. */

#include <aga/window.h>
#include <aga/sound.h>
#include <aga/utility.h>
#include <aga/log.h>
#include <aga/error.h>
#include <aga/pack.h>
#include <aga/midi.h>
#include <aga/io.h>
#include <aga/render.h>
#include <aga/draw.h>
#include <aga/startup.h>
#include <aga/script.h>
#include <aga/build.h>
#include <aga/graph.h>

#include <apro.h>

static enum aga_result aga_put_default(void) {
	/*
	 * TODO: We can definitely work on making this more useful. Maybe
	 * 		 An interactive way to load a project once we have some
	 * 		 Semblance of UI?
	 */

	enum aga_result result;

	static const char str1[] = "No project loaded or no script files provided";
	static const char str2[] = "Did you forget `-f' or `-C'?";
	static const float text_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const float color[] = { 0.6f, 0.3f, 0.8f, 1.0f };

	if((result = aga_render_clear(color))) return result;

	result = aga_render_text_format(0.05f, 0.1f, text_color, str1);
	if(result) return result;

	return aga_render_text_format(0.05f, 0.2f, text_color, str2);
}

int main(int argc, char** argv) {
	enum aga_result result;

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

	aga_bool_t die = AGA_FALSE;
	aga_ulong_t dt = 0;

	const char* gl_version;

	/* TODO: CLI opt for this. */
	aga_bool_t do_prof = !!aga_getenv("AGA_DOPROF");
	struct aga_graph prof = { 0 };

	struct aga_script_userdata userdata;

	const char* logfiles[] = { 0 /* auto stdout */, "aga.log" };
	aga_log_new(logfiles, AGA_LEN(logfiles));

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

	aga_log(__FILE__, "Breathing in the chemicals...");

	result = aga_settings_new(&opts, argc, argv);
	aga_error_check_soft(__FILE__, "aga_settings_new", result);

#ifdef AGA_DEVBUILD
	if(opts.compile) {
		aga_error_check_soft(__FILE__, "aga_build", aga_build(&opts));
		aga_log(__FILE__, "Bye-bye!");
		return 0;
	}
#endif

	result = aga_resource_pack_new(opts.respack, &pack);
	aga_error_check_soft(__FILE__, "aga_resource_pack_new", result);

	result = aga_settings_parse_config(&opts, &pack);
	aga_error_check_soft(__FILE__, "aga_settings_parse_config", result);

	aga_log(__FILE__, "Initializing systems...");

	result = aga_window_device_new(&env, opts.display);
	aga_error_check(__FILE__, "aga_window_device_new", result);

	result = aga_keymap_new(&keymap, &env);
	aga_error_check(__FILE__, "aga_keymap_new", result);

	if(do_prof) {
		result = aga_graph_new(&prof, &env, argc, argv);
		if(result) do_prof = AGA_FALSE;
	}

	result = aga_window_new(
			opts.width, opts.height, opts.title,
			&env, &win, AGA_TRUE, argc, argv);
	aga_error_check(__FILE__, "aga_window_new", result);

	result = aga_renderer_string(&gl_version);
	aga_error_check_soft(__FILE__, "aga_renderer_string", result);
	aga_log(
			__FILE__, "Acquired GL context: %s",
			gl_version ? gl_version : "<error>");

	aga_error_check(__FILE__, "aga_draw_set", aga_draw_set(draw_flags));

#ifndef NDEBUG
# ifdef AGA_HAVE_SPAWN
	aga_error_check_soft(__FILE__, "aga_prerun_hook", aga_prerun_hook(&opts));
# endif
#endif

	if(opts.audio_enabled) {
		if((result = aga_sound_device_new(&snd, opts.audio_buffer))) {
			aga_error_check_soft(__FILE__, "aga_sound_device_new", result);
			/* TODO: Separate "unavailable snd/midi" and user defined. */
			opts.audio_enabled = AGA_FALSE;
		}
	}

	/* TODO: Work on MIDI. */
	/* TODO: This is only a hard error while it's WIP. */
	(void) midi;
	/*
	result = aga_midi_device_new(&midi);
	aga_error_check(__FILE__, "aga_midi_device_new", result);
	{
		struct aga_resource* mres;
		struct aga_midi m;

		result = aga_resource_new(&pack, "snd/sndtest.mid.raw", &mres);
		aga_error_check(__FILE__, "aga_resource_new", result);

		result = aga_midi_new(&midi, &m, mres->data, mres->size);
		aga_error_check(__FILE__, "aga_midi_new", result);

		result = aga_midi_play(&midi, &m);
		aga_error_check(__FILE__, "aga_midi_play", result);
	}*/

	if(!aga_streql(opts.version, AGA_VERSION)) {
		static const char err[] = "warn: Project version `%s' does not match "
								  "engine version `" AGA_VERSION "'";
		aga_log(__FILE__, err, opts.version);
	}

	aga_log(__FILE__, "Starting up the script engine...");

	result = aga_script_engine_new(
			&script_engine, opts.startup_script, &pack, opts.python_path,
			&userdata);
	aga_error_check_soft(__FILE__, "aga_script_engine_new", result);
	if(!result) {
		aga_log(__FILE__, "Instantiating game instance...");

		result = aga_script_engine_lookup(&script_engine, &class, "game");
		aga_error_check(__FILE__, "aga_script_engine_lookup", result);

		result = aga_script_instance_new(&class, &inst);
		aga_error_check(__FILE__, "aga_script_instance_new", result);

		/* TODO: The EH mode on this right now is terrible. */
		result = aga_script_instance_call(&script_engine, &inst, "create");
		aga_error_check_soft(__FILE__, "aga_script_instance_call", result);
	}

	aga_log(__FILE__, "Done!");

	while(!die) {
		result = aga_window_select(&env, &win);
		aga_error_check_soft(__FILE__, "aga_window_select", result);

		apro_stamp_start(APRO_PRESWAP);
		{
			apro_stamp_start(APRO_POLL);
			{
				pointer.dx = 0;
				pointer.dy = 0;

				result = aga_window_device_poll(
						&env, &keymap, &win, &pointer, &die, &buttons);
				aga_error_check_soft(
						__FILE__, "aga_window_device_poll", result);
			}
			apro_stamp_end(APRO_POLL);

			apro_stamp_start(APRO_SCRIPT_UPDATE);
			{
				if(class.class) {
					result = aga_script_instance_call(
							&script_engine, &inst, "update");
					aga_error_check_soft(
							__FILE__, "aga_script_instance_call", result);
				}
				else {
					result = aga_put_default();
					aga_error_check_soft(__FILE__, "aga_put_default", result);
				}
			}
			apro_stamp_end(APRO_SCRIPT_UPDATE);

			apro_stamp_start(APRO_RES_SWEEP);
			{
				result = aga_resource_pack_sweep(&pack);
				aga_error_check_soft(
						__FILE__, "aga_resource_pack_sweep", result);
			}
			apro_stamp_end(APRO_RES_SWEEP);
		}
		apro_stamp_end(APRO_PRESWAP);

		dt = apro_stamp_us(APRO_PRESWAP);

		if(do_prof) {
			result = aga_graph_update(&prof, &env);
			aga_error_check_soft(__FILE__, "aga_graph_update", result);
		}

		apro_clear();

		/* Window is already dead/dying if `die' is set. */
		if(!die) {
			result = aga_window_swap(&env, &win);
			aga_error_check_soft(__FILE__, "aga_window_swap", result);
		}
	}

	aga_log(__FILE__, "Tearing down...");

#ifdef __APPLE__
	/* Need to flush before shutdown to avoid NSGL dying */
	aga_error_check_soft(__FILE__, "aga_render_flush", aga_render_flush());
#endif

	if(class.class) {
		result = aga_script_instance_call(&script_engine, &inst, "close");
		aga_error_check_soft(__FILE__, "aga_script_instance_call", result);

		result = aga_script_instance_delete(&inst);
		aga_error_check_soft(__FILE__, "aga_script_instance_delete", result);
	}

	result = aga_script_engine_delete(&script_engine);
	aga_error_check_soft(__FILE__, "aga_script_engine_delete", result);

	if(opts.audio_enabled) {
		result = aga_sound_device_delete(&snd);
		aga_error_check_soft(__FILE__, "aga_sound_device_delete", result);
	}

	result = aga_config_delete(&opts.config);
	aga_error_check_soft(__FILE__, "aga_config_delete", result);

	result = aga_window_delete(&env, &win);
	aga_error_check_soft(__FILE__, "aga_window_delete", result);

	if(do_prof) {
		result = aga_graph_delete(&prof, &env);
		aga_error_check_soft(__FILE__, "aga_window_delete", result);
	}

	result = aga_keymap_delete(&keymap);
	aga_error_check_soft(__FILE__, "aga_keymap_delete", result);

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
	aga_error_check_soft(__FILE__, "aga_window_device_poll", result);
	 */

	result = aga_window_device_delete(&env);
	aga_error_check_soft(__FILE__, "aga_window_device_delete", result);

	result = aga_resource_pack_delete(&pack);
	aga_error_check_soft(__FILE__, "aga_resource_pack_delete", result);

	aga_log(__FILE__, "Bye-bye!");

	return 0;
}
