/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agastartup.h>
#include <agalog.h>
#include <agapack.h>
#include <agaerr.h>
#include <agaio.h>
#define AGA_WANT_UNIX
#include <agastd.h>

enum aga_result aga_setopts(struct aga_opts* opts, int argc, char** argv) {
	AGA_PARAM_CHK(opts);
	AGA_PARAM_CHK(argv);

	opts->config_file = "aga.sgml";
	opts->display = getenv("DISPLAY");
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

	memset(&opts->config, 0, sizeof(opts->config));

#ifdef AGA_HAVE_GETOPT
	{
		const char* help =
			"warn: usage: %s [-f respack] [-A dsp] [-D display] [-C dir] "
			"[-v]";
		int o;
		while((o = 	getopt(argc, argv, "f:s:A:D:C:v")) != -1) {
			switch(o) {
				default: {
					aga_log(__FILE__, help, argv[0]);
					goto break2;
				}
				case 'f': opts->respack = optarg; break;
				case 'A': opts->audio_dev = optarg; break;
				case 'D': opts->display = optarg; break;
				case 'C': opts->chdir = optarg; break;
				case 'v': {
					extern int WWW_TraceFlag; /* From libwww. */
					WWW_TraceFlag = 1;

#ifdef _DEBUG
					extern int debugging; /* From python. */
					debugging = 1;
#endif

					opts->verbose = AGA_TRUE;
				}
			}
		}
		break2:;
	}

	if(chdir(opts->chdir) == -1) {
		(void) aga_patherrno(__FILE__, "chdir", opts->chdir);
	}
#endif

	return AGA_RESULT_OK;
}

enum aga_result aga_setconf(struct aga_opts* opts, struct aga_respack* pack) {
	enum aga_result result;
	void* fp;
	aga_size_t size;
	int v;

	const char* enabled[] = { "Audio", "Enabled" };
	const char* device[] = { "Audio", "Device" };
	const char* startup[] = { "Script", "Startup" };
	const char* path[] = { "Script", "Path" };
	const char* version[] = { "General", "Version" };
	const char* width[] = { "Display", "Width" };
	const char* height[] = { "Display", "Height" };
	const char* fov[] = { "Display", "FOV" };

	AGA_PARAM_CHK(opts);
	AGA_PARAM_CHK(pack);

	AGA_CHK(aga_resfptr(pack, opts->config_file, &fp, &size));
	aga_conf_debug_file = opts->config_file;
	AGA_CHK(aga_mkconf(fp, size, &opts->config));

	result = aga_conftree(
		&opts->config, enabled, AGA_LEN(enabled), &v, AGA_INTEGER);
	if(result) aga_soft(__FILE__, "aga_conftree", result);

	result = aga_conftree(
		&opts->config, version, AGA_LEN(version), &opts->version,
		AGA_STRING);
	if(result) aga_soft(__FILE__, "aga_conftree", result);

	if(!opts->audio_dev) {
		result = aga_conftree(
			&opts->config, device, AGA_LEN(device), &opts->audio_dev,
			AGA_STRING);
		if(result) aga_soft(__FILE__, "aga_conftree", result);
	}

	result = aga_conftree(
		&opts->config, startup, AGA_LEN(startup), &opts->startup_script,
		AGA_STRING);
	if(result) aga_soft(__FILE__, "aga_conftree", result);

	result = aga_conftree(
		&opts->config, path, AGA_LEN(path), &opts->python_path, AGA_STRING);
	if(result) aga_soft(__FILE__, "aga_conftree", result);

	result = aga_conftree(
		&opts->config, width, AGA_LEN(width), &opts->width, AGA_INTEGER);
	if(result) aga_soft(__FILE__, "aga_conftree", result);

	result = aga_conftree(
		&opts->config, height, AGA_LEN(height), &opts->height, AGA_INTEGER);
	if(result) aga_soft(__FILE__, "aga_conftree", result);

	result = aga_conftree(
		&opts->config, fov, AGA_LEN(fov), &opts->fov, AGA_FLOAT);
	if(result) aga_soft(__FILE__, "aga_conftree", result);

	return AGA_RESULT_OK;
}

#ifdef AGA_HAVE_SPAWN
enum aga_result aga_prerun_hook(struct aga_opts* opts) {
	enum aga_result result;

	const char* hook[] = { "Development", "PreHook" };
	char* program = getenv("SHELL");
	char* args[] = { 0 /* shell */, 0 /* -c */, 0 /* exec */, 0 };
	char* project_path;

# ifndef _DEBUG
	aga_log(__FILE__, "warn: Executing pre-run hook in non-debug build");
# endif

	AGA_PARAM_CHK(opts);

	project_path = strrchr(opts->config_file, '/');

# ifdef _WIN32
	if(!program) program = "cmd.exe";
	args[1] = "/c";
# else
	if(!program) program = "sh";
	args[1] = "-c";
# endif

	args[0] = program;

	AGA_CHK(aga_conftree(
		&opts->config, hook, AGA_LEN(hook), &args[2], AGA_STRING));

	aga_log(__FILE__, "Executing project pre-run hook `%s'", args[2]);

	if(project_path) {
		aga_size_t len = (aga_size_t) (project_path - opts->config_file) + 1;
		AGA_VERIFY(project_path = calloc(len + 1, sizeof(char)), AGA_RESULT_OOM);
		strncpy(project_path, opts->config_file, len);
	}

	result = aga_spawn_sync(program, args, project_path);
	if(result) aga_soft(__FILE__, "aga_spawn_sync", result);

	free(project_path);

	return AGA_RESULT_OK;
}
#endif
