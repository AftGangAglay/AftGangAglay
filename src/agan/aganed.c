/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agan/aganed.h>
#include <agan/aganobj.h>

#include <agascripthelp.h>
#include <agascript.h>
#include <agapack.h>
#include <agastartup.h>
#include <agaconf.h>
#include <agaerr.h>
#include <agautil.h>
#include <agawin.h>

#include <agalog.h>
#define AGA_WANT_UNIX
#include <agastd.h>

/*
 * "Editor" functions are isolated here as they should not be callable in
 * Distribution. No well-behaved AGA application should be attempting to write
 * Directly to the filesystem -- AGA applications are able to run off of
 * Read-only media and attempting to do direct writes can counteract this
 * Inherent behaviour of an otherwise portable AGA application.
 *
 * There is also a certain degree to which this discourages malicious programs
 * From being written with AGA -- although in practice a bad actor could simply
 * Ship a build with "editor" enabled, or use something more sensible than AGA
 * To write their malware.
 *
 * Finally -- the usage patterns for some of these are particular and unsafe,
 * The program state after `killpack' is somewhat ill-defined and as such it
 * Would be unwise to ship an application using it; even if the developer is
 * Careful to avoid holding onto resource references etc.. These functions are
 * Too bug prone to enter distribution.
 */

/*
 * TODO: Tear down and reload script land (or just user scripts) once we
 * 		 Consolidate Python state more to allow it.
 */
static struct py_object* agan_killpack(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct aga_respack* pack;

	(void) env;
	(void) self;
	(void) args;

	if(!(pack = aga_getscriptptr(AGA_SCRIPT_PACK))) return 0;

	if(aga_script_err("aga_killrespack", aga_killrespack(pack))) return 0;

	return py_object_incref(PY_NONE);
}

static struct py_object* agan_mkpack(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct aga_respack* pack;
	struct aga_opts* opts;

	(void) env;
	(void) self;
	(void) args;

	if(!(pack = aga_getscriptptr(AGA_SCRIPT_PACK))) return 0;
	if(!(opts = aga_getscriptptr(AGA_SCRIPT_OPTS))) return 0;

	if(aga_script_err("aga_mkrespack", aga_mkrespack(opts->respack, pack))) {
		return 0;
	}

	return py_object_incref(PY_NONE);
}

static struct py_object* agan_dumpobj(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	enum aga_result result;

	struct py_object* objp;
	struct py_object* pathp;

	struct agan_object* obj;
	struct aga_conf_node node;
	const char* path;

	(void) env;
	(void) self;

	/* dumpobj(int, string) */
	if(!aga_vararg_list(args, PY_TYPE_TUPLE, 2) ||
		!aga_arg(&objp, args, 0, PY_TYPE_INT) ||
		!aga_arg(&pathp, args, 1, PY_TYPE_STRING)) {

		return aga_arg_error("dumpobj", "int and string");
	}

	obj = (void*) py_int_get(objp);
	path = py_string_get(pathp);

	result = agan_getobjconf(obj, &node);
	if(aga_script_err("agan_getobjconf", result)) return 0;

	/* Update conf tree with current transform data. */
	/* TODO: This is copied from `mkobj_trans'. */
	{
		aga_size_t i, j;

		const char* elem[2];

		for(i = 0; i < 3; ++i) {
			struct py_object* l;

			elem[0] = agan_conf_components[i];

			l = py_dict_lookup(obj->transform, agan_trans_components[i]);
			if(!l) {
				py_error_set_key();
				return 0;
			}

			for(j = 0; j < 3; ++j) {
				struct aga_conf_node* n;
				struct py_object* o;

				elem[1] = agan_xyz[j];

				result = aga_conftree_raw(
						node.children, elem, AGA_LEN(elem), &n);
				if(aga_script_err("aga_conftree_raw", result)) return 0;

				if((o = py_list_get(l, j))->type != PY_TYPE_FLOAT) {
					py_error_set_badarg();
					return 0;
				}

				n->data.flt = py_float_get(o);
			}
		}
	}

	/* Update conf tree with current model path. */
	{
		static const char* model = "Model";

		struct aga_conf_node* n;

		result = aga_conftree_raw(node.children, &model, 1, &n);
		if(aga_script_err("aga_conftree_raw", result)) return 0;

		aga_free(n->data.string);
		n->data.string = aga_strdup(obj->modelpath);
	}

	{
		FILE* outp = fopen(path, "wb+");
		if(!outp) {
			aga_script_err("fopen", aga_errno_path(__FILE__, "fopen", path));
			return 0;
		}

		/* TODO: Leaky stream. */
		result = aga_dumptree(node.children, outp);
		if(aga_script_err("aga_dumptree", result)) return 0;

		if(fclose(outp) == EOF) {
			aga_script_err("fopen", aga_errno(__FILE__, "fclose"));
			return 0;
		}
	}

	/* TODO: Leaky conf.. */
	result = aga_killconf(&node);
	if(aga_script_err("aga_killconf", result)) return 0;

	return py_object_incref(PY_NONE);
}

static struct py_object* agan_fdiag(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	char* path;
	struct py_object* str;

	(void) env;
	(void) self;
	(void) args;

	if(aga_script_err("aga_filediag", aga_filediag(&path))) return 0;

	str = py_string_new(path);
	aga_free(path);

	return str;
}

static struct py_object* agan_setobjmdl(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	static const char* model = "Model";

	enum aga_result result;

	struct aga_conf_node root;
	struct aga_conf_node* node;

	struct py_object* objp;
	struct py_object* pathp;

	struct agan_object* obj;
	const char* path;

	(void) env;
	(void) self;

	/* setobjmdl(int, string) */
	if(!aga_vararg_list(args, PY_TYPE_TUPLE, 2) ||
		!aga_arg(&objp, args, 0, PY_TYPE_INT) ||
		!aga_arg(&pathp, args, 1, PY_TYPE_STRING)) {

		return aga_arg_error("setobjmdl", "int and string");
	}

	obj = aga_script_getptr(objp);
	path = py_string_get(pathp);

	result = agan_getobjconf(obj, &root);
	if(aga_script_err("agan_getobjconf", result)) return 0;

	/* TODO: We really need to work out this whole root/non-root fiasco. */
	result = aga_conftree_raw(root.children, &model, 1, &node);
	if(aga_script_err("aga_conftree_raw", result)) return 0;

	/*
	 * TODO: We don't validate that the conf node is the correct type here nor
	 * 		 Above.
	 */
	aga_free(node->data.string);
	node->data.string = aga_strdup(path);

	/* TODO: Soft reload object model here. Delete old drawlist etc. */

	return py_object_incref(PY_NONE);
}

enum aga_result agan_ed_register(struct py_env* env) {
#define aga_(name) { #name, agan_##name }
	static const struct py_methodlist methods[] = {
			aga_(killpack), aga_(mkpack), aga_(dumpobj), aga_(fdiag),
			aga_(setobjmdl),

			{ 0, 0 }
	};
#undef aga_

	struct py_object* ed;

	/*
	 * TODO: Make a better distinction between debug/non-debug/distribution --
	 * 		 This should also extend to profiler presence and noverify aswell
	 * 		 Once this is enacted.
	 */
#ifdef _DEBUG
	/*
	 * User is not meant to access `ed' directly as a module -- but as an attr
	 * of `agan'.
	 */
	if(!(ed = py_module_new_methods(env, "_ed", methods))) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}
#else
	if(!(ed = py_int_new(0))) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}
#endif

	if(py_dict_insert(agan_dict, "ed", ed) == -1) return AGA_RESULT_ERROR;

	return AGA_RESULT_OK;
}
