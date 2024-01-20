/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agastartup.h>
#include <agalog.h>
#include <agaerr.h>
#include <agaio.h>
#define AGA_WANT_UNIX
#include <agastd.h>

enum af_err aga_setopts(struct aga_opts* opts, int argc, char** argv) {
	enum af_err result;
	int v;

	const char* enabled[] = { "Audio", "Enabled" };
	const char* device[] = { "Audio", "Device" };
	const char* startup[] = { "Script", "Startup" };
	const char* path[] = { "Script", "Path" };
	const char* version[] = { "General", "Version" };
	const char* width[] = { "Display", "Width" };
	const char* height[] = { "Display", "Height" };
	const char* fov[] = { "Display", "FOV" };

	AF_PARAM_CHK(opts);
	AF_PARAM_CHK(argv);

	opts->config_file = "aga.sgml";
	opts->display = getenv("DISPLAY");
	opts->chdir = ".";
	opts->audio_dev = "/dev/dsp1";
	opts->startup_script = "script/main.py";
	opts->python_path = "vendor/python/lib:script:script/AGAScriptLib";
	/* TODO: Make this path not hardcoded! */
	opts->respack = "agapack.raw";
	opts->width = 640;
	opts->height = 480;
	opts->fov = 90.0f;
	opts->audio_enabled = AF_TRUE;
	opts->version = AGA_VERSION;

	af_memset(&opts->config, 0, sizeof(opts->config));

#ifdef AGA_HAVE_GETOPT
	{
		const char* help =
			"warn: usage: %s [-f config] [-A dsp] [-D display] [-C dir]";
		int o;
		while((o = 	getopt(argc, argv, "f:s:A:D:C:")) != -1) {
			switch(o) {
				default: {
					aga_log(__FILE__, help, argv[0]);
					goto break2;
				}
				case 'f': opts->config_file = optarg; break;
				case 'A': opts->audio_dev = optarg; break;
				case 'D': opts->display = optarg; break;
				case 'C': opts->chdir = optarg; break;
			}
		}
		break2:;
	}

	if(chdir(opts->chdir) == -1) {
		(void) aga_af_patherrno(__FILE__, "chdir", opts->chdir);
	}
#endif

	{
		void* fp;
		af_size_t size;

		/* TODO: Disentangle startup/opts/respack interdependency. */
		AF_CHK(aga_open(opts->config_file, &fp, &size));
		AF_CHK(aga_mkconf(fp, size, &opts->config));

		if(fclose(fp) == EOF) return aga_af_errno(__FILE__, "fclose");
	}

	aga_log(__FILE__, "Config loaded from `%s'", opts->config_file);

	result = aga_conftree(
		&opts->config, enabled, AF_ARRLEN(enabled), &v, AGA_INTEGER);
	if(result) aga_af_soft(__FILE__, "aga_conftree", result);

	result = aga_conftree(
			&opts->config, version, AF_ARRLEN(version), &opts->version,
			AGA_STRING);
	if(result) aga_af_soft(__FILE__, "aga_conftree", result);

	if(!opts->audio_dev) {
		result = aga_conftree(
			&opts->config, device, AF_ARRLEN(device), &opts->audio_dev, AGA_STRING);
		if(result) aga_af_soft(__FILE__, "aga_conftree", result);
	}

	result = aga_conftree(
		&opts->config, startup, AF_ARRLEN(startup),
		&opts->startup_script, AGA_STRING);
	if(result) aga_af_soft(__FILE__, "aga_conftree", result);

	result = aga_conftree(
		&opts->config, path, AF_ARRLEN(path), &opts->python_path, AGA_STRING);
	if(result) aga_af_soft(__FILE__, "aga_conftree", result);

	result = aga_conftree(
		&opts->config, width, AF_ARRLEN(width), &opts->width, AGA_INTEGER);
	if(result) aga_af_soft(__FILE__, "aga_conftree", result);

	result = aga_conftree(
		&opts->config, height, AF_ARRLEN(height), &opts->height, AGA_INTEGER);
	if(result) aga_af_soft(__FILE__, "aga_conftree", result);

	result = aga_conftree(
		&opts->config, fov, AF_ARRLEN(fov), &opts->fov, AGA_FLOAT);
	if(result) aga_af_soft(__FILE__, "aga_conftree", result);

	return AF_ERR_NONE;
}

#ifdef AGA_HAVE_SPAWN
enum af_err aga_prerun_hook(struct aga_opts* opts) {
	enum af_err result;

	const char* hook[] = { "Development", "PreHook" };
	char* program = getenv("SHELL");
	char* args[] = { 0 /* shell */, 0 /* -c */, 0 /* exec */, 0 };
	char* project_path;

# ifndef _DEBUG
	aga_log(__FILE__, "warn: Executing pre-run hook in non-debug build");
# endif

	AF_PARAM_CHK(opts);

	project_path = strrchr(opts->config_file, '/');

# ifdef _WINDOWS
	if(!program) program = "cmd";
	args[1] = "/c";
# else
	if(!program) program = "sh";
	args[1] = "-c";
# endif

	args[0] = program;

	AF_CHK(aga_conftree(
		&opts->config, hook, AF_ARRLEN(hook), &args[2], AGA_STRING));

	aga_log(__FILE__, "Executing project pre-run hook `%s'", args[2]);

	if(project_path) {
		af_size_t len = (af_size_t) (project_path - opts->config_file) + 1;
		AF_VERIFY(project_path = calloc(len + 1, sizeof(char)), AF_ERR_MEM);
		strncpy(project_path, opts->config_file, len);
	}

	result = aga_spawn_sync(program, args, project_path);
	if(result) aga_af_soft(__FILE__, "aga_spawn_sync", result);

	free(project_path);

	return AF_ERR_NONE;
}
#endif
