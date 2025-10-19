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
#include <aga/build.h>
#include <aga/userdata.h>
#include <aga/input.h>

#include <asys/memory.h>
#include <asys/string.h>
#include <asys/stream.h>
#include <asys/log.h>

#include <mil/mil.h>
#include <mil/widget.h>
#include <mil/system.h>

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

	enum asys_result result;
	struct aga_resource_pack* pack = AGA_GET_USERDATA(env)->resource_pack;

	(void) env;
	(void) self;

	if(args) return aga_arg_error("killpack", "none");

	/*
	 * TODO: These should have extra safeties on them. Do they even need to
	 * 		 Be separate or can we just have a "reload" function?
	 */
	result = aga_resource_pack_delete(pack);
	if(aga_script_err(__FILE__, "aga_resource_pack_delete", result)) return 0;

	return py_object_incref(PY_NONE);
}

static struct py_object* agan_mkpack(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	enum asys_result result;
	struct aga_resource_pack* pack = AGA_GET_USERDATA(env)->resource_pack;
	struct aga_settings* opts = AGA_GET_USERDATA(env)->opts;

	(void) env;
	(void) self;

	if(args) return aga_arg_error("mkpack", "none");

	/*
	 * TODO: Make `aga.sgml' contain build file info as a fallback from CLI
	 * 		 For this.
	 */
	/*
	 * TODO: Implement partial/archive updates -- in dev builds could allow for
	 * 		 Leaving stale files as gaps in the pack and append new/moved
	 * 		 Resources to avoid needing to re-copy everything. Make an
	 * 		 Allowance in the pack header of empty space in dev builds.
	 */
	result = aga_build(opts);
	if(aga_script_err(__FILE__, "aga_build", result)) return 0;

	result = aga_resource_pack_new(opts->respack, pack, opts);
	if(aga_script_err(__FILE__, "aga_resource_pack_new", result)) return 0;

	return py_object_incref(PY_NONE);
}

static struct py_object* agan_dumpobj(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	enum asys_result result;

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
	if(aga_script_err(__FILE__, "agan_getobjconf", result)) return 0;

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
						node.children, elem, ASYS_LENGTH(elem), &n);

				if(aga_script_err(
						__FILE__, "aga_config_lookup_check", result)) {

					return 0;
				}

				if((o = py_list_get(l, j))->type != PY_TYPE_FLOAT) {
					py_error_set_badarg();
					return 0;
				}

				n->data.flt = py_float_get(o);
			}
		}
	}

	{
		struct asys_stream stream;

		result = asys_stream_new_write(&stream, path);
		if(aga_script_err(__FILE__, "asys_stream_new", result)) return 0;

		/* TODO: Leaky stream. */
		result = aga_config_dump(node.children, &stream);
		if(aga_script_err(__FILE__, "aga_config_dump", result)) return 0;

		result = asys_stream_delete(&stream);
		if(aga_script_err(__FILE__, "asys_stream_delete", result)) return 0;
	}

	/* TODO: Leaky conf.. */
	result = aga_config_delete(&node);
	if(aga_script_err(__FILE__, "aga_config_delete", result)) return 0;

	return py_object_incref(PY_NONE);
}

static struct py_object* agan_fdiag(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct mil_ctx* mil = AGA_GET_USERDATA(env)->mil;

	char* path = "";
	struct py_object* str;

	(void) env;
	(void) self;

	if(args) return aga_arg_error("fdiag", "none");

	/* TODO: Fix this! */
	/*
	if(aga_script_err(__FILE__, "aga_dialog_file", aga_dialog_file(&path))) {
		return 0;
	 */
	mil_widget(mil, "agan_fdiag", MIL_FILE_MODAL, mil->top, MIL_END);

	if(!(str = py_string_new(path))) {
		asys_memory_free(path);

		py_error_set_nomem();
		return 0;
	}

	/*asys_memory_free(path);*/

	return str;
}

static struct py_object* agan_setobj_render(
		struct py_env* env, struct py_object* self, struct py_object* args,
		const char* element, const char* name) {

	struct aga_resource_pack* respack = AGA_GET_USERDATA(env)->resource_pack;

	enum asys_result result;

	struct aga_config_node root;
	struct aga_config_node* node;

	struct py_object* objp;
	struct py_object* pathp;

	struct agan_object* obj;
	const char* path;

	(void) env;
	(void) self;

	/* setobj<>(int, string) */
	if(!aga_vararg_list(args, PY_TYPE_TUPLE, 2) ||
			!aga_arg(&objp, args, 0, PY_TYPE_INT) ||
			!aga_arg(&pathp, args, 1, PY_TYPE_STRING)) {

		return aga_arg_error(name, "int and string");
	}

	obj = (void*) py_int_get(objp);
	path = py_string_get(pathp);

	result = agan_getobjconf(obj, &root);
	if(aga_script_err(__FILE__, "agan_getobjconf", result)) return 0;

	/* TODO: We really need to work out this whole root/non-root fiasco. */
	result = aga_config_lookup_check(root.children, &element, 1, &node);
	if(aga_script_err(__FILE__, "aga_config_lookup_check", result)) return 0;

	/*
	 * TODO: We don't validate that the conf node is the correct type here nor
	 * 		 Above.
	 * TODO: Once `agan_getobjconf` retains the object tree this will persist
	 *		 Between calls.
	 */
	asys_memory_free(node->data.string);
	node->data.string = asys_string_duplicate(path);

	glDeleteLists(obj->drawlist, 1);
	(void) mil_gl_result(__FILE__, "glDeleteLists");

	if(agan_mkobj_model(env, obj, &root, respack, path)) {
		aga_script_err(__FILE__, "agan_mkobj_model", ASYS_RESULT_ERROR);
		return 0;
	}

	result = aga_config_delete(&root);
	(void) aga_script_err(__FILE__, "aga_config_delete", result);
	py_error_clear();

	return py_object_incref(PY_NONE);
}


static struct py_object* agan_setobjmdl(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	return agan_setobj_render(env, self, args, "Model", "setobjmdl");
}

static struct py_object* agan_setobjtex(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	return agan_setobj_render(env, self, args, "Texture", "setobjtex");
}

static void agan_push_button_activate(
		mil_widget_t widget, struct mil_ctx* mil, void* data) {

	struct aga_mil_userdata* userdata = mil->user;

	(void) data;

	if(!userdata->ui_activate) return;

	py_object_decref(py_call_function(
			userdata->script_engine->env, userdata->ui_activate,
			py_int_new((py_value_t) widget)));

	if(py_error_occurred()) aga_script_engine_trace();
}

static asys_bool_t agan_widget_dimension(
		struct py_object* args, asys_size_t n,
		py_value_t* width, py_value_t* height, const char* error) {

	struct py_object* width_object;
	struct py_object* height_object;

	if(py_varobject_size(args) < n + 2 ||
			!aga_arg(&width_object, args, n, PY_TYPE_INT) ||
			!aga_arg(&height_object, args, n + 1, PY_TYPE_INT)) {

		aga_arg_error("widget", error);
		return ASYS_FALSE;
	}

	*width = py_int_get(width_object);
	*height = py_int_get(height_object);

	return ASYS_TRUE;
}

static struct py_object* agan_widget(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct mil_ctx* mil = AGA_GET_USERDATA(env)->mil;
	struct aga_settings* opts = AGA_GET_USERDATA(env)->opts;
	struct aga_mil_userdata* userdata = mil->user;

	const char* name;
	struct py_object* name_object;

	mil_widget_t parent;
	struct py_object* parent_object;

	enum mil_class class;
	struct py_object* class_object;

	mil_widget_t widget;
	struct py_object* return_widget;

	py_value_t width, height;

	mil_callback_t activate = agan_push_button_activate;

	(void) self;

	/* widget(string, int, int, ...) */
	if(!aga_arg_list(args, PY_TYPE_TUPLE) ||
			!aga_arg(&name_object, args, 0, PY_TYPE_STRING) ||
			!aga_arg(&parent_object, args, 1, PY_TYPE_INT) ||
			!aga_arg(&class_object, args, 2, PY_TYPE_INT)) {

		return aga_arg_error("widget", "string, int, int...");
	}

	name = py_string_get(name_object);

	parent = (mil_widget_t) py_int_get(parent_object);
	if(!parent) parent = mil->top;

	class = py_int_get(class_object);

	switch(class) {
		case MIL_AUTO: {
			if(!agan_widget_dimension(
					args, 3, &width, &height, "string, int, int, int, int")) {

				return 0;
			}

			widget = mil_widget(
					mil, name, class, parent,
					MIL_WIDTH, width,
					MIL_HEIGHT, height,
					MIL_END);

			break;
		}

		case MIL_DRAWING_AREA: {
			 widget = userdata->gl_area = mil_widget(
					mil, name, class, parent,
					/* TODO: Resizing. */
					/* MIL_DRAWING_AREA_RESIZE_CALLBACK, area_resize, */
					MIL_DRAWING_AREA_INPUT_CALLBACK, aga_main_window_input,
					MIL_WIDTH, opts->width,
					MIL_HEIGHT, opts->height,
					MIL_END);

			break;
		}

		case MIL_ICON: {
			activate = 0;
			ASYS_FALLTHROUGH;
		}
		/* FALLTHROUGH */
		case MIL_ICON_BUTTON: {
			ASYS_FALLTHROUGH;
		}
		/* FALLTHROUGH */
		case MIL_LABEL: {
			ASYS_FALLTHROUGH;
		}
		/* FALLTHROUGH */
		case MIL_PUSH_BUTTON: {
			const char* icon = 0;
			struct py_object* icon_object;

			if(py_varobject_size(args) > 3) {
				if(aga_arg(&icon_object, args, 3, PY_TYPE_STRING)) {
					icon = py_string_get(icon_object);
				}
			}

			widget = mil_widget(
					mil, name, class, parent,
					MIL_ACTIVATE_CALLBACK, activate,
					MIL_LABEL_ICON, icon,
					MIL_END);

			break;
		}

		case MIL_ICON_CONTAINER: {
			enum mil_icon_layout layout;
			struct py_object* layout_object;

			if(py_varobject_size(args) < 4 ||
					!aga_arg(&layout_object, args, 3, PY_TYPE_INT)) {

				return aga_arg_error(
						"widget", "string, int, int, int, int, int");
			}

			if(!agan_widget_dimension(
					args, 4, &width, &height,
					"string, int, int, int, int, int")) {

				return 0;
			}

			layout = py_int_get(layout_object);

			widget = mil_widget(
					mil, name, class, parent,
					MIL_ICON_LAYOUT, layout,
					MIL_WIDTH, width,
					MIL_HEIGHT, height,
					MIL_END);

			break;
		}

		case MIL_CASCADE_BUTTON: {
			mil_widget_t pulldown;
			struct py_object* pulldown_object;

			if(!aga_arg(&pulldown_object, args, 3, PY_TYPE_INT)) {
				return aga_arg_error("widget", "string, int, int, int");
			}

			pulldown = (mil_widget_t) py_int_get(pulldown_object);

			widget = mil_widget(
					mil, name, class, parent,
					MIL_CASCADE_MENU, pulldown,
					MIL_END);

			break;
		}

		default: {
			widget = mil_widget(mil, name, class, parent, MIL_END);
			break;
		}
	}

	return_widget = py_int_new((py_value_t) widget);
	if(!return_widget) {
		py_error_set_nomem();
		return 0;
	}

	return return_widget;
}

static struct py_object* agan_widgetsz(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	struct mil_ctx* mil = AGA_GET_USERDATA(env)->mil;

	mil_widget_t widget;
	struct py_object* widget_object;

	py_value_t width;
	struct py_object* width_object;

	py_value_t height;
	struct py_object* height_object;

	(void) self;

	/* widgetsz(int, int, int) */
	if(!aga_arg_list(args, PY_TYPE_TUPLE) ||
			!aga_arg(&widget_object, args, 0, PY_TYPE_INT) ||
			!aga_arg(&width_object, args, 1, PY_TYPE_INT) ||
			!aga_arg(&height_object, args, 2, PY_TYPE_INT)) {

		return aga_arg_error("widget", "int, int, int");
	}

	widget = (mil_widget_t) py_int_get(widget_object);
	width = py_int_get(width_object);
	height = py_int_get(height_object);

	mil_widget_set_size(mil, widget, width, height);

	return py_object_incref(PY_NONE);
}

static struct py_object* agan_build(
		struct py_env* env, struct py_object* self, struct py_object* args) {

	enum asys_result result;

	struct aga_settings* opts = AGA_GET_USERDATA(env)->opts;

	(void) self;
	(void) args;

	/* TODO: Allow configuring of build settings via. optional args. */
	result = aga_build(opts);
	if(aga_script_err(__FILE__, "aga_build", result)) return 0;

	return py_object_incref(PY_NONE);
}
#endif

enum asys_result agan_ed_register(struct py_env* env) {
	struct py_object* ed;

#ifdef AGA_DEVBUILD
# define aga_(name) { #name, agan_##name }
	static const struct py_methodlist methods[] = {
			aga_(killpack), aga_(mkpack), aga_(dumpobj), aga_(fdiag),
			aga_(setobjmdl), aga_(widget), aga_(widgetsz), aga_(build),
			aga_(setobjtex),

			{ 0, 0 }
	};
# undef aga_

	enum asys_result result;

	/*
	 * User is not meant to access `ed' directly as a module -- but as an attr
	 * of `agan'.
	 */
	if(!(ed = py_module_new_methods(env, "_ed", methods))) {
		aga_script_engine_trace();
		return ASYS_RESULT_ERROR;
	}

# define aga_(class) \
		do { \
			if((result = aga_module_insert_int(ed, #class, MIL_##class))) { \
				py_object_decref(ed); \
				return result; \
			} \
		} while(0)

	aga_(PUSH_BUTTON);
	aga_(CASCADE_BUTTON);
	aga_(ICON_BUTTON);
	aga_(MAIN_WINDOW);
	aga_(WINDOW);
	aga_(PANED);
	aga_(MENUBAR);
	aga_(ICON_CONTAINER);
	aga_(AUTO);
	aga_(PULLDOWN);
	aga_(LABEL);
	aga_(FRAME);
	aga_(DRAWING_AREA);
	aga_(ICON);
	aga_(SEPARATOR);

	aga_(ICON_TREE);
	aga_(ICON_TABLE);
	aga_(ICON_GRID);
# undef aga_
#else
	(void) env;

	if(!(ed = py_int_new(0))) {
		aga_script_engine_trace();
		return ASYS_RESULT_ERROR;
	}
#endif

	if(py_dict_insert(agan_dict, "ed", ed) == -1) return ASYS_RESULT_ERROR;

	return ASYS_RESULT_OK;
}
