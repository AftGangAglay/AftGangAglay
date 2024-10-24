/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agan/editor.h>
#include <agan/object.h>

#include <aga/script.h>
#include <aga/pack.h>
#include <aga/startup.h>
#include <aga/config.h>
#include <aga/error.h>
#include <aga/utility.h>
#include <aga/window.h>

#include <aga/log.h>
#define AGA_WANT_UNIX
#include <aga/std.h>

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
 * TODO: `AGA_DEVBUILD' is inconsistent as to whether they eliminate the
 * 		 Function and its decls or just disable their functionality and force
 * 		 An error or guaranteed safe return.
 */
#ifdef AGA_DEVBUILD
/*
 * TODO: Tear down and reload script land (or just user scripts) once we
 * 		 Consolidate Python state more to allow it.
 * 		 Either implement serialisation so the application editor class can
 * 		 Maintain position/other state or have explicit functions in-engine for
 * 		 Storing camera state etc..
 */
static struct py_object* agan_killpack(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	enum aga_result result;
	struct aga_resource_pack* pack = AGA_GET_USERDATA(env)->resource_pack;

	(void) env;
	(void) self;

	if(args) return aga_arg_error("killpack", "none");

	/*
	 * TODO: These should have extra safeties on them. Do they even need to
	 * 		 Be separate or can we just have a "reload" function?
	 */
	result = aga_resource_pack_delete(pack);
	if(aga_script_err("aga_resource_pack_delete", result)) return 0;

	return py_object_incref(PY_NONE);
}

static struct py_object* agan_mkpack(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	enum aga_result result;
	struct aga_resource_pack* pack = AGA_GET_USERDATA(env)->resource_pack;
	struct aga_settings* opts = AGA_GET_USERDATA(env)->opts;

	(void) env;
	(void) self;

	if(args) return aga_arg_error("mkpack", "none");

	result = aga_resource_pack_new(opts->respack, pack);
	if(aga_script_err("aga_resource_pack_new", result)) return 0;

	return py_object_incref(PY_NONE);
}

static struct py_object* agan_dumpobj(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	enum aga_result result;

	struct py_object* objp;
	struct py_object* pathp;

	struct agan_object* obj;
	struct aga_config_node node;
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
		unsigned i, j;

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
				struct aga_config_node* n;
				struct py_object* o;

				elem[1] = agan_xyz[j];

				result = aga_config_lookup_check(
						node.children, elem, AGA_LEN(elem), &n);
				if(aga_script_err("aga_config_lookup_check", result)) return 0;

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

		struct aga_config_node* n;

		result = aga_config_lookup_check(node.children, &model, 1, &n);
		if(aga_script_err("aga_config_lookup_check", result)) return 0;

		aga_free(n->data.string);
		n->data.string = aga_strdup(obj->modelpath);
	}

	{
		FILE* f;
		if(!(f = fopen(path, "w"))) {
			aga_script_err("fopen", aga_error_system_path(__FILE__, "fopen", path));
			return 0;
		}

		/* TODO: Leaky stream. */
		result = aga_config_dump(node.children, f);
		if(aga_script_err("aga_config_dump", result)) return 0;

		if(fclose(f) == EOF) {
			aga_script_err("fclose", aga_error_system(__FILE__, "fclose"));
			return 0;
		}
	}

	/* TODO: Leaky conf.. */
	result = aga_config_delete(&node);
	if(aga_script_err("aga_config_delete", result)) return 0;

	return py_object_incref(PY_NONE);
}

static struct py_object* agan_fdiag(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	char* path;
	struct py_object* str;

	(void) env;
	(void) self;

	if(args) return aga_arg_error("fdiag", "none");

	if(aga_script_err("aga_dialog_file", aga_dialog_file(&path))) return 0;

	if(!(str = py_string_new(path))) {
		aga_free(path);

		py_error_set_nomem();
		return 0;
	}

	aga_free(path);

	return str;
}

static struct py_object* agan_setobjmdl(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	static const char* model = "Model";

	enum aga_result result;

	struct aga_config_node root;
	struct aga_config_node* node;

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

	obj = aga_script_pointer_get(objp);
	path = py_string_get(pathp);

	result = agan_getobjconf(obj, &root);
	if(aga_script_err("agan_getobjconf", result)) return 0;

	/* TODO: We really need to work out this whole root/non-root fiasco. */
	result = aga_config_lookup_check(root.children, &model, 1, &node);
	if(aga_script_err("aga_config_lookup_check", result)) return 0;

	/*
	 * TODO: We don't validate that the conf node is the correct type here nor
	 * 		 Above.
	 */
	aga_free(node->data.string);
	node->data.string = aga_strdup(path);

	/* TODO: Soft reload object model here. Delete old drawlist etc. */

	return py_object_incref(PY_NONE);
}
#endif

enum aga_result agan_ed_register(struct py_env* env) {
#ifdef AGA_DEVBUILD
# define aga_(name) { #name, agan_##name }
	static const struct py_methodlist methods[] = {
			aga_(killpack), aga_(mkpack), aga_(dumpobj), aga_(fdiag),
			aga_(setobjmdl),

			{ 0, 0 }
	};
# undef aga_
#endif

	struct py_object* ed;

#ifdef AGA_DEVBUILD
	/*
	 * User is not meant to access `ed' directly as a module -- but as an attr
	 * of `agan'.
	 */
	if(!(ed = py_module_new_methods(env, "_ed", methods))) {
		aga_script_engine_trace();
		return AGA_RESULT_ERROR;
	}
#else
	(void) env;

	if(!(ed = py_int_new(0))) {
		aga_script_engine_trace();
		return AGA_RESULT_ERROR;
	}
#endif

	if(py_dict_insert(agan_dict, "ed", ed) == -1) return AGA_RESULT_ERROR;

	return AGA_RESULT_OK;
}
