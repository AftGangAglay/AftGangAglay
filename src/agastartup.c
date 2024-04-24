/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agastartup.h>
#include <agalog.h>
#include <agapack.h>
#include <agaerr.h>
#include <agautil.h>
#include <agaio.h>

#define AGA_WANT_UNIX

#include <agastd.h>

enum aga_result aga_setopts(struct aga_opts* opts, int argc, char** argv) {
	if(!opts) return AGA_RESULT_BAD_PARAM;
	if(!argv) return AGA_RESULT_BAD_PARAM;

	opts->config_file = "aga.sgml";
	opts->display = aga_getenv("DISPLAY");
	opts->chdir = ".";
	opts->audio_dev = "/dev/dsp1";
	opts->startup_script = "script/main.py";
	opts->python_path = "script";
	opts->respack = "agapack.raw";
	opts->width = 640;
	opts->height = 480;
	opts->fov = 90.0f;
	opts->audio_enabled = AGA_TRUE;
	opts->version = AGA_VERSION;
	opts->verbose = AGA_FALSE;

	aga_memset(&opts->config, 0, sizeof(opts->config));

#ifdef AGA_HAVE_GETOPT
	{
		static const char help[] =
			"warn: usage: %s [-f respack] [-A dsp] [-D display] [-C dir] [-v]";
		int o;
		while((o = getopt(argc, argv, "f:s:A:D:C:v")) != -1) {
			switch(o) {
				default: {
					aga_log(__FILE__, help, argv[0]);
					goto break2;
				}
				case 'f': {
					opts->respack = optarg;
					break;
				}
				case 'A': {
					opts->audio_dev = optarg;
					break;
				}
				case 'D': {
					opts->display = optarg;
					break;
				}
				case 'C': {
					opts->chdir = optarg;
					break;
				}
				case 'v': {
					extern int WWW_TraceFlag; /* From libwww. */

#ifdef _DEBUG
					extern int debugging; /* From python. */
					debugging = 1;
#endif

					WWW_TraceFlag = 1;

					opts->verbose = AGA_TRUE;
				}
			}
		}
		break2:;
	}

# ifdef AGA_HAVE_UNISTD
	if(chdir(opts->chdir) == -1) {
		(void) aga_errno_path(__FILE__, "chdir", opts->chdir);
	}
# endif
#else
	(void) argc;
	(void) argv;
#endif

	return AGA_RESULT_OK;
}

enum aga_result aga_setconf(struct aga_opts* opts, struct aga_respack* pack) {
	enum aga_result result;
	void* fp;
	aga_size_t size;
	int v;

	static const char* enabled[] = { "Audio", "Enabled" };
	static const char* device[] = { "Audio", "Device" };
	static const char* startup[] = { "Script", "Startup" };
	static const char* path[] = { "Script", "Path" };
	static const char* version[] = { "General", "Version" };
	static const char* width[] = { "Display", "Width" };
	static const char* height[] = { "Display", "Height" };
	static const char* fov[] = { "Display", "FOV" };

	if(!opts) return AGA_RESULT_BAD_PARAM;
	if(!pack) return AGA_RESULT_BAD_PARAM;

	result = aga_resfptr(pack, opts->config_file, &fp, &size);
	if(result) return result;

	aga_conf_debug_file = opts->config_file;

	result = aga_mkconf(fp, size, &opts->config);
	if(result) return result;

	result = aga_conftree(
			opts->config.children, enabled, AGA_LEN(enabled), &v, AGA_INTEGER);
	aga_soft(__FILE__, "aga_conftree", result);

	result = aga_conftree(
			opts->config.children, version, AGA_LEN(version), &opts->version,
			AGA_STRING);
	aga_soft(__FILE__, "aga_conftree", result);

	if(!opts->audio_dev) {
		result = aga_conftree(
				opts->config.children, device, AGA_LEN(device),
				&opts->audio_dev, AGA_STRING);
		aga_soft(__FILE__, "aga_conftree", result);
	}

	result = aga_conftree(
			opts->config.children, startup, AGA_LEN(startup),
			&opts->startup_script, AGA_STRING);
	aga_soft(__FILE__, "aga_conftree", result);

	result = aga_conftree(
			opts->config.children, path, AGA_LEN(path), &opts->python_path,
			AGA_STRING);
	aga_soft(__FILE__, "aga_conftree", result);

	result = aga_conftree(
			opts->config.children, width, AGA_LEN(width), &opts->width,
			AGA_INTEGER);
	aga_soft(__FILE__, "aga_conftree", result);

	result = aga_conftree(
			opts->config.children, height, AGA_LEN(height), &opts->height,
			AGA_INTEGER);
	aga_soft(__FILE__, "aga_conftree", result);

	result = aga_conftree(
			opts->config.children, fov, AGA_LEN(fov), &opts->fov, AGA_FLOAT);
	aga_soft(__FILE__, "aga_conftree", result);

	return AGA_RESULT_OK;
}

#ifdef AGA_HAVE_SPAWN

enum aga_result aga_prerun_hook(struct aga_opts* opts) {
	enum aga_result result;

	const char* hook[] = { "Development", "PreHook" };
	char* program = aga_getenv("SHELL");
	char* args[] = { 0 /* shell */, 0 /* -c */, 0 /* exec */, 0 };
	char* project_path;

# ifndef _DEBUG
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

	result = aga_conftree(
			opts->config.children, hook, AGA_LEN(hook), &args[2], AGA_STRING);
	if(result) return result;

	aga_log(__FILE__, "Executing project pre-run hook `%s'", args[2]);

	if(project_path) {
		aga_size_t len = (aga_size_t) (project_path - opts->config_file) + 1;

		project_path = aga_calloc(len + 1, sizeof(char));
		if(!project_path) return AGA_RESULT_OOM;

		strncpy(project_path, opts->config_file, len);
	}

	result = aga_spawn_sync(program, args, project_path);
	aga_soft(__FILE__, "aga_spawn_sync", result);

	aga_free(project_path);

	return AGA_RESULT_OK;
}

#endif
