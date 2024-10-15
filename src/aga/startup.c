/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/startup.h>
#include <aga/log.h>
#include <aga/pack.h>
#include <aga/error.h>
#include <aga/utility.h>
#include <aga/io.h>
#include <aga/diagnostic.h>

#define AGA_WANT_UNIX

#include <aga/std.h>

enum aga_result aga_settings_new(
		struct aga_settings* opts, int argc, char** argv) {

	if(!opts) return AGA_RESULT_BAD_PARAM;
	if(!argv) return AGA_RESULT_BAD_PARAM;

#ifdef AGA_DEVBUILD
	opts->compile = AGA_FALSE;
	opts->build_file = "agabuild.sgml";
#endif
	opts->config_file = "aga.sgml";
	opts->display = aga_getenv("DISPLAY");
	opts->chdir = ".";
	opts->audio_buffer = 1024;
	opts->startup_script = "script/main.py";
	opts->python_path = "script";
	opts->respack = "agapack.raw";
	opts->width = 640;
	opts->height = 480;
	opts->title = "Aft Gang Aglay";
	opts->mipmap_default = AGA_FALSE;
	opts->fov = 90.0f;
	opts->audio_enabled = AGA_TRUE;
	opts->version = AGA_VERSION;
	opts->verbose = AGA_FALSE;

	aga_bzero(&opts->config, sizeof(opts->config));

#ifdef AGA_HAVE_GETOPT
	{
		static const char helpmsg[] =
			"warn: usage:\n"
			"\t%s [-f respack] [-A dsp] [-D display] [-C dir] [-v] [-h]"
#ifdef AGA_DEVBUILD
			"\n\t%s -c [-f buildfile] [-C dir] [-v] [-h]"
#endif
		;

		int o;
		while((o = getopt(argc, argv, "hcf:s:A:D:C:v")) != -1) {
			switch(o) {
				default:
#ifdef AGA_DEVBUILD
					;help:
#endif
				{
					aga_log(__FILE__, helpmsg, argv[0], argv[0]);
					goto break2;
				}
#ifdef AGA_DEVBUILD
				case 'c': {
					if(optind != 2) goto help;

					opts->compile = AGA_TRUE;
					break;
				}
#endif
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

					/* TODO: Fix audio buffer options. */
					/*opts->audio_dev = optarg;*/
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

					opts->verbose = AGA_TRUE;
				}
			}
		}
		break2:;
	}

# ifdef AGA_HAVE_UNISTD
	if(chdir(opts->chdir) == -1) {
		(void) aga_error_system_path(__FILE__, "chdir", opts->chdir);
	}
# endif
#else
	(void) argc;
	(void) argv;
#endif

	return AGA_RESULT_OK;
}

enum aga_result aga_settings_parse_config(
		struct aga_settings* opts, struct aga_resource_pack* pack) {

	enum aga_result result;
	void* fp;
	aga_size_t size;
	aga_slong_t v;
	double fv;

	static const char* enabled[] = { "Audio", "Enabled" };
	static const char* startup[] = { "Script", "Startup" };
	static const char* path[] = { "Script", "Path" };
	static const char* version[] = { "General", "Version" };
	static const char* title[] = { "General", "Title" };
	static const char* width[] = { "Display", "Width" };
	static const char* height[] = { "Display", "Height" };
	static const char* mipmap[] = { "Graphics", "MipmapDefault" };
	static const char* fov[] = { "Display", "FOV" };

	if(!opts) return AGA_RESULT_BAD_PARAM;
	if(!pack) return AGA_RESULT_BAD_PARAM;

	result = aga_resource_stream(pack, opts->config_file, &fp, &size);
	if(result) return result;

	aga_config_debug_file = opts->config_file;

	result = aga_config_new(fp, size, &opts->config);
	if(result) return result;

	result = aga_config_lookup(
			opts->config.children, enabled, AGA_LEN(enabled), &v, AGA_INTEGER,
			AGA_TRUE);
	aga_error_check_soft(__FILE__, "aga_config_lookup", result);
	if(!result) opts->audio_enabled = !!v;

	result = aga_config_lookup(
			opts->config.children, version, AGA_LEN(version), &opts->version,
			AGA_STRING, AGA_TRUE);
	aga_error_check_soft(__FILE__, "aga_config_lookup", result);

	result = aga_config_lookup(
			opts->config.children, title, AGA_LEN(title), &opts->title,
			AGA_STRING, AGA_TRUE);
	aga_error_check_soft(__FILE__, "aga_config_lookup", result);

	/*
	if(!opts->audio_dev) {
		result = aga_config_lookup(
				opts->config.children, device, AGA_LEN(device),
				&opts->audio_dev, AGA_STRING, AGA_TRUE);
		aga_error_check_soft(__FILE__, "aga_config_lookup", result);
	}
	 */

	result = aga_config_lookup(
			opts->config.children, startup, AGA_LEN(startup),
			&opts->startup_script, AGA_STRING, AGA_TRUE);
	aga_error_check_soft(__FILE__, "aga_config_lookup", result);

	result = aga_config_lookup(
			opts->config.children, path, AGA_LEN(path), &opts->python_path,
			AGA_STRING, AGA_TRUE);
	aga_error_check_soft(__FILE__, "aga_config_lookup", result);

	result = aga_config_lookup(
			opts->config.children, width, AGA_LEN(width), &v,
			AGA_INTEGER, AGA_TRUE);
	aga_error_check_soft(__FILE__, "aga_config_lookup", result);
	if(!result) opts->width = v;

	result = aga_config_lookup(
			opts->config.children, height, AGA_LEN(height), &v,
			AGA_INTEGER, AGA_TRUE);
	aga_error_check_soft(__FILE__, "aga_config_lookup", result);
	if(!result) opts->height = v;

	result = aga_config_lookup(
			opts->config.children, mipmap, AGA_LEN(mipmap), &v,
			AGA_INTEGER, AGA_TRUE);
	aga_error_check_soft(__FILE__, "aga_config_lookup", result);
	if(!result) opts->mipmap_default = !!v;

	result = aga_config_lookup(
			opts->config.children, fov, AGA_LEN(fov), &fv, AGA_FLOAT,
			AGA_TRUE);
	aga_error_check_soft(__FILE__, "aga_config_lookup", result);
	if(!result) opts->fov = (float) fv;

	return AGA_RESULT_OK;
}

#ifdef AGA_HAVE_SPAWN

enum aga_result aga_prerun_hook(struct aga_settings* opts) {
	enum aga_result result;

	const char* hook[] = { "Development", "PreHook" };
	char* program = aga_getenv("SHELL");
	char* args[] = { 0 /* shell */, 0 /* -c */, 0 /* exec */, 0 };
	char* project_path;

	AGA_DEPRECATED("Development/PreHook", "agabuild.sgml");

# ifndef AGA_DEVBUILD
	aga_log(__FILE__, "warn: Executing pre-run hook in non-debug build");
# endif

	if(!opts) return AGA_RESULT_BAD_PARAM;

	project_path = strrchr(opts->config_file, '/');

# ifdef _WIN32
	if(!program) program = "cmd.exe";
	args[1] = "/c";
# else
	if(!program) program = "sh";
	args[1] = "-c";
# endif

	args[0] = program;

	result = aga_config_lookup(
			opts->config.children, hook, AGA_LEN(hook), &args[2], AGA_STRING,
			AGA_TRUE);
	if(result) return result;

	aga_log(__FILE__, "Executing project pre-run hook `%s'", args[2]);

	if(project_path) {
		aga_size_t len = (aga_size_t) (project_path - opts->config_file) + 1;

		project_path = aga_calloc(len + 1, sizeof(char));
		if(!project_path) return AGA_RESULT_OOM;

		strncpy(project_path, opts->config_file, len);
	}

	result = aga_process_spawn(program, args, project_path);
	aga_error_check_soft(__FILE__, "aga_process_spawn", result);

	aga_free(project_path);

	return AGA_RESULT_OK;
}

#endif
