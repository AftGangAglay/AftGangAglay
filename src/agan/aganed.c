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
struct py_object* agan_killpack(
		struct py_object* self, struct py_object* args) {

	struct aga_respack* pack;

	(void) self;
	(void) args;

	if(!(pack = aga_getscriptptr(AGA_SCRIPT_PACK))) return 0;

	if(aga_script_err("aga_killrespack", aga_killrespack(pack))) return 0;

	return py_object_incref(PY_NONE);
}

struct py_object* agan_mkpack(
		struct py_object* self, struct py_object* args) {

	struct aga_respack* pack;
	struct aga_opts* opts;

	(void) self;
	(void) args;

	if(!(pack = aga_getscriptptr(AGA_SCRIPT_PACK))) return 0;
	if(!(opts = aga_getscriptptr(AGA_SCRIPT_OPTS))) return 0;

	if(aga_script_err("aga_mkrespack", aga_mkrespack(opts->respack, pack))) {
		return 0;
	}

	return py_object_incref(PY_NONE);
}

struct py_object* agan_dumpobj(
		struct py_object* self, struct py_object* args) {

	enum aga_result result;

	struct py_object* objp;
	struct py_object* pathp;

	struct agan_object* obj;
	struct aga_conf_node node;
	const char* path;

	(void) self;

	if(!aga_arg_list(args, PY_TYPE_TUPLE) ||
	   !aga_arg(&objp, args, 0, PY_TYPE_INT) ||
	   !aga_arg(&pathp, args, 1, PY_TYPE_STRING)) {

		return aga_arg_error("dumpobj", "int and string");
	}

	if(aga_script_int(objp, (py_value_t*) &obj)) return 0;
	if(aga_script_string(pathp, &path)) return 0;

	result = agan_getobjconf(obj, &node);
	if(aga_script_err("agan_getobjconf", result)) return 0;

	/* TODO: This is copied from `mkobj_trans'. */
	{
		aga_size_t i, j;

		const char* elem[2];

		for(i = 0; i < 3; ++i) {
			struct py_object* l;

			elem[0] = agan_conf_components[i];

			l = py_dict_lookup(obj->transform, agan_trans_components[i]);
			if(!l) return 0;

			for(j = 0; j < 3; ++j) {
				struct aga_conf_node* n;
				struct py_object* o;
				double f;

				elem[1] = agan_xyz[j];

				result = aga_conftree_raw(
						node.children, elem, AGA_LEN(elem), &n);
				if(aga_script_err("aga_conftree_raw", result)) return 0;

				if(aga_list_get(l, j, &o)) return 0;
				if(aga_script_float(o, &f)) return 0;

				n->data.flt = f;
			}
		}
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

enum aga_result agan_ed_register(void) {
#define _(name) { #name, agan_##name }
	static const struct py_methodlist methods[] = {
			_(killpack), _(mkpack), _(dumpobj),

			{ 0, 0 }
	};
#undef _

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
	if(!(ed = py_module_new_methods("_ed", methods))) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}
#else
	if(!(ed = py_int_new(0))) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}
#endif

	if(py_dict_insert(agan_dict, "ed", ed) == -1) {
		aga_script_trace();
		return AGA_RESULT_ERROR;
	}

	return AGA_RESULT_OK;
}
