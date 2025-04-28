/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/startup.h>
#include <aga/pack.h>
#include <aga/diagnostic.h>

#include <asys/log.h>
#include <asys/getopt.h>
#include <asys/memory.h>
#include <asys/main.h>
#include <asys/string.h>

enum asys_result aga_settings_new(
		struct aga_settings* opts, struct asys_main_data* main_data) {

	if(!opts) return ASYS_RESULT_BAD_PARAM;
	if(!main_data) return ASYS_RESULT_BAD_PARAM;

#ifdef AGA_DEVBUILD
	opts->compile = ASYS_FALSE;
	opts->build_file = "agabuild.sgml";
	opts->no_stamp = ASYS_FALSE;
#endif
	opts->config_file = "aga.sgml";
	opts->display = 0;
	opts->chdir = ".";
	opts->audio_buffer = 1024;
	opts->startup_script = "script/main.py.raw";
	opts->python_path = "script";
	opts->respack = "agapack.raw";
	opts->width = 640;
	opts->height = 480;
	opts->title = "Aft Gang Aglay";
	opts->mipmap_default = ASYS_FALSE;
	opts->fov = 90.0f;
	opts->audio_enabled = ASYS_TRUE;
	opts->version = AGA_VERSION;
	opts->verbose = ASYS_FALSE;
	opts->profiler = ASYS_FALSE;

	asys_memory_zero(&opts->config, sizeof(struct aga_config_node));

	{
		static const char helpmsg[] =
			"warn: usage:\n"
			"\t%s [-f respack] [-A dsp] [-D display] [-C dir] [-p] [-v] [-h]"
#ifdef AGA_DEVBUILD
			"\n\t%s -c [-f buildfile] [-C dir] [-B] [-v] [-h]"
#endif
		;

		int o;
		while(1) {
			o = getopt(main_data->argc, main_data->argv, "hcf:A:D:C:vpB");
			if(o == -1) break;

			switch(o) {
				default:
#ifdef AGA_DEVBUILD
					;help:
#endif
				{
					const char* program = main_data->argv[0];
					asys_log(__FILE__, helpmsg, program, program);
					goto break2;
				}

				case 'c': {
#ifdef AGA_DEVBUILD
					if(optind != 2) goto help;

					opts->compile = ASYS_TRUE;
					break;
#else
					asys_result_fatal(
							__FILE__,
							"err: Cannot compile respack in non-devbuild",
							ASYS_RESULT_BAD_PARAM);
#endif
				}

				case 'f': {
#ifdef AGA_DEVBUILD
					if(opts->compile) opts->build_file = optarg;
					else
#endif
					opts->respack = optarg;

					break;
				}

				case 'A': {
#ifdef AGA_DEVBUILD
					if(opts->compile) goto help;
#endif

					asys_log(__FILE__, "")
					/*opts->audio_dev = optarg;*/
					break;
				}

				case 'p': {
#ifdef AGA_DEVBUILD
					if(opts->compile) goto help;
#endif

					opts->profiler = ASYS_TRUE;
					break;
				}

				case 'D': {
#ifdef AGA_DEVBUILD
					if(opts->compile) goto help;
#endif

					opts->display = optarg;
					break;
				}

				case 'C': {
					opts->chdir = optarg;
					break;
				}

				case 'v': {
					extern int WWW_TraceFlag; /* From libwww. */
					WWW_TraceFlag = 1;

					opts->verbose = ASYS_TRUE;

					break;
				}

				case 'B': {
#ifdef AGA_DEVBUILD
					if(!opts->compile) goto help;
#endif

					opts->no_stamp = ASYS_TRUE;

					break;
				}
			}
		}
		break2:;
	}

	/* TODO: Fix this. */
#ifdef AGA_HAVE_UNISTD
	if(chdir(opts->chdir) == -1) {
		(void) aga_error_system_path(__FILE__, "chdir", opts->chdir);
	}
#endif

	return ASYS_RESULT_OK;
}

/* TODO: Release startup -- delete config node, free script/pypath etc.. */

enum asys_result aga_settings_parse_config(
		struct aga_settings* opts, struct aga_resource_pack* pack) {

	static const char* enabled[] = { "Audio", "Enabled" };
	static const char* startup[] = { "Script", "Startup" };
	static const char* path[] = { "Script", "Path" };
	static const char* version[] = { "General", "Version" };
	static const char* title[] = { "General", "Title" };
	static const char* width[] = { "Display", "Width" };
	static const char* height[] = { "Display", "Height" };
	static const char* mipmap[] = { "Graphics", "MipmapDefault" };
	static const char* fov[] = { "Display", "FOV" };

	static asys_float_format_buffer_t double_format;

	enum asys_result result;
	struct asys_stream* stream;

	asys_size_t size;
	aga_config_int_t v;
	double fv;

	if(!opts) return ASYS_RESULT_BAD_PARAM;
	if(!pack) return ASYS_RESULT_BAD_PARAM;

	result = aga_resource_stream(pack, opts->config_file, &stream, &size);
	if(result) return result;

	result = aga_config_new(stream, size, &opts->config);
	if(result) return result;

	result = aga_config_lookup(
			opts->config.children, enabled, ASYS_LENGTH(enabled),
			&v, AGA_INTEGER, ASYS_TRUE);

	asys_log_result(__FILE__, "aga_config_lookup", result);
	if(!result) opts->audio_enabled = !!v;

	result = aga_config_lookup(
			opts->config.children, version, ASYS_LENGTH(version),
			&opts->version, AGA_STRING, ASYS_TRUE);

	asys_log_result(__FILE__, "aga_config_lookup", result);

	result = aga_config_lookup(
			opts->config.children, title, ASYS_LENGTH(title),
			&opts->title, AGA_STRING, ASYS_TRUE);

	asys_log_result(__FILE__, "aga_config_lookup", result);

	/*
	if(!opts->audio_dev) {
		result = aga_config_lookup(
				opts->config.children, device, ASYS_LENGTH(device),
				&opts->audio_dev, AGA_STRING, ASYS_TRUE);
		asys_log_result(__FILE__, "aga_config_lookup", result);
	}
	 */

	result = aga_config_lookup(
			opts->config.children, startup, ASYS_LENGTH(startup),
			&opts->startup_script, AGA_PATH, ASYS_TRUE);

	asys_log_result(__FILE__, "aga_config_lookup", result);

	result = aga_config_lookup(
			opts->config.children, path, ASYS_LENGTH(path),
			&opts->python_path, AGA_PATH, ASYS_TRUE);

	asys_log_result(__FILE__, "aga_config_lookup", result);

	result = aga_config_lookup(
			opts->config.children, width, ASYS_LENGTH(width),
			&v, AGA_INTEGER, ASYS_TRUE);

	asys_log_result(__FILE__, "aga_config_lookup", result);
	if(!result) opts->width = (asys_size_t) v;

	result = aga_config_lookup(
			opts->config.children, height, ASYS_LENGTH(height),
			&v, AGA_INTEGER, ASYS_TRUE);

	asys_log_result(__FILE__, "aga_config_lookup", result);
	if(!result) opts->height = (asys_size_t) v;

	result = aga_config_lookup(
			opts->config.children, mipmap, ASYS_LENGTH(mipmap),
			&v, AGA_INTEGER, ASYS_TRUE);

	asys_log_result(__FILE__, "aga_config_lookup", result);
	if(!result) opts->mipmap_default = !!v;

	result = aga_config_lookup(
			opts->config.children, fov, ASYS_LENGTH(fov),
			&fv, AGA_FLOAT, ASYS_TRUE);

	asys_log_result(__FILE__, "aga_config_lookup", result);
	if(!result) opts->fov = (float) fv;

	/* TODO: Put this in a separate function. */

	asys_log(__FILE__, "Loaded startup options:");
#ifdef AGA_BUILD
	asys_log(__FILE__, "\tCompile?: %s", asys_bool_to_string(opts->compile));
	asys_log(__FILE__, "\tBuild: %s", opts->build_file);
#endif
	asys_log(__FILE__, "\tTitle: %s", asys_string_optional(opts->title));
	asys_log(
			__FILE__, "\tConfig: %s", asys_string_optional(opts->config_file));

	asys_log(__FILE__, "\tDisplay: %s", asys_string_optional(opts->display));
	asys_log(__FILE__, "\tDirectory: %s", asys_string_optional(opts->chdir));
	asys_log(__FILE__, "\tVersion: %s", asys_string_optional(opts->version));
	asys_log(
			__FILE__, "\tResource Pack: %s",
			asys_string_optional(opts->respack));

	asys_log(
			__FILE__, "\tAudio Buffer: " ASYS_NATIVE_ULONG_FORMAT,
			opts->audio_buffer);

	asys_log(
			__FILE__, "\tAudio?: %s",
			asys_bool_to_string(opts->audio_enabled));

	asys_log(
			__FILE__, "\tStartup Script: %s",
			asys_string_optional(opts->startup_script));

	asys_log(
			__FILE__, "\tPython Path: %s",
			asys_string_optional(opts->python_path));

	asys_log(
			__FILE__, "\tWindow Size: [ " ASYS_NATIVE_ULONG_FORMAT ", "
			ASYS_NATIVE_ULONG_FORMAT " ]", opts->width, opts->height);

	asys_log(
			__FILE__, "\tMipmap?: %s",
			asys_bool_to_string(opts->mipmap_default));

	double_format[0] = 0;
	result = asys_float_to_string(opts->fov, &double_format);
	asys_log_result(__FILE__, "asys_float_to_string", result);
	asys_log(__FILE__, "\tFOV: %s", double_format);

	asys_log(__FILE__, "\tVerbose?: %s", asys_bool_to_string(opts->verbose));

	asys_log(__FILE__, "\tProfiler?: %s", asys_bool_to_string(opts->profiler));

	/* TODO: Config dump. */

	return ASYS_RESULT_OK;
}
