/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/result.h>

#include <aga/error.h>
#include <aga/config.h>
#include <aga/log.h>
#include <aga/io.h>
#include <aga/utility.h>
#include <aga/std.h>

/* Very nasty dependency to leak - keep it contained! */
#include <SGML.h>

/*
 * NOTE: These need to be in `strcmp' order (effectively alphabetical for us)
 */
enum aga_sgml_node {
	AGA_NODE_ITEM, AGA_NODE_ROOT,

	AGA_ELEMENT_COUNT
};

enum aga_sgml_item_attribs {
	AGA_ITEM_NAME, AGA_ITEM_TYPE,

	AGA_ITEM_ATTRIB_COUNT
};

#define AGA_CONFIG_MAX_DEPTH (1024)

struct aga_sgml_structured {
	const HTStructuredClass* class;

	struct aga_config_node* stack[1024];
	aga_size_t depth;
};

const char* aga_config_debug_file = "<none>";

void SGML_character(HTStream* context, char c);

void SGML_free(HTStream* context);

static aga_bool_t aga_isblank(char c) {
	return c == ' ' || c == '\r' || c == '\t' || c == '\n';
}

static enum aga_result aga_sgml_push(
		struct aga_sgml_structured* s, struct aga_config_node* node) {

	if(!s) return AGA_RESULT_BAD_PARAM;
	if(!node) return AGA_RESULT_BAD_PARAM;

	if(s->depth > AGA_CONFIG_MAX_DEPTH) return AGA_RESULT_OOM;

	s->stack[s->depth++] = node;

	return AGA_RESULT_OK;
}

static void aga_sgml_free(struct aga_sgml_structured* me) {
	(void) me;
}

static void aga_sgml_abort(struct aga_sgml_structured* me, HTError e) {
	(void) me;
	(void) e;
}

static void aga_sgml_putc(struct aga_sgml_structured* me, char c) {
	struct aga_config_node* node = me->stack[me->depth - 1];

	if(node->type == AGA_NONE) return;
	if(!node->data.string && aga_isblank(c)) return;

	node->data.string = aga_realloc(node->data.string, ++node->scratch + 1);
	if(!node->data.string) {
		aga_error_system(__FILE__, "aga_realloc");
		return;
	}

	node->data.string[node->scratch - 1] = c;
	node->data.string[node->scratch] = 0;
}

static void aga_sgml_puts(HTStructured* me, const char* str) {
	(void) me;
	(void) str;
}

static void aga_sgml_write(HTStructured* me, const char* str, unsigned len) {
	(void) me;
	(void) str;
	(void) len;
}

void aga_sgml_start_element(
		struct aga_sgml_structured* me, int element_number,
		const HTBool* attribute_present, char** attribute_value) {

	struct aga_config_node* parent = me->stack[me->depth - 1];
	struct aga_config_node* node;
	enum aga_result result;

	aga_size_t sz = ++parent->len * sizeof(struct aga_config_node);

	if(!(parent->children = aga_realloc(parent->children, sz))) {
		aga_error_system(__FILE__, "aga_realloc");
		parent->len = 0;
		return;
	}

	node = &parent->children[parent->len - 1];
	memset(node, 0, sizeof(struct aga_config_node));

	if((result = aga_sgml_push(me, node))) {
		aga_error_check_soft(__FILE__, "aga_sgml_push", result);
		return;
	}

	switch(element_number) {
		default: {
			aga_log(
					__FILE__, "warn: SGML_new: unknown element %i in `%s'",
					element_number, aga_config_debug_file);
			return;
		}
		case AGA_NODE_ITEM: {
			if(!attribute_present[AGA_ITEM_NAME]) node->name = 0;
			else {
				const char* value = attribute_value[AGA_ITEM_NAME];
				if(!(node->name = aga_strdup(value))) {
					/* TODO: Bad -- ignored OOM! */
					aga_error_system(__FILE__, "aga_strdup");
					return;
				}
			}

			if(!attribute_present[AGA_ITEM_TYPE]) node->type = AGA_NONE;
			else {
				const char* typename = attribute_value[AGA_ITEM_TYPE];
				if(aga_streql(typename, "Integer")) node->type = AGA_INTEGER;
				else if(aga_streql(typename, "String")) {
					node->type = AGA_STRING;
				}
				else if(aga_streql(typename, "Float")) node->type = AGA_FLOAT;
				else if(aga_streql(typename, "None")) node->type = AGA_NONE;
				else {
					static const char fmt[] =
							"warn: <item> element has unknown type "
							"`%s' in `%s'";
					aga_log(__FILE__, fmt, typename, aga_config_debug_file);
					node->type = AGA_NONE;
				}
			}
		}
		case AGA_NODE_ROOT: break; /* No attribs */
	}
}

void aga_sgml_end_element(struct aga_sgml_structured* me, int element_number) {
	aga_size_t i;

	struct aga_config_node* node = me->stack[me->depth - 1];
	char* string = node->data.string;

	(void) element_number;

	for(i = 0; string && i <= node->scratch; ++i) {
		aga_size_t n = node->scratch - i;
		char* c = &string[n];

		if(aga_isblank(*c)) { *c = 0; }
		else if(*c) break;
	}

	switch(node->type) {
		default: break;
		case AGA_INTEGER: {
			aga_slong_t res;
			if(!string) {
				node->data.integer = 0;
				break;
			}

			/* TODO: `strtoll` not C89. */
			res = strtol(node->data.string, 0, 0);
			aga_free(string);
			node->data.integer = res;
			break;
		}
		case AGA_FLOAT: {
			double res;
			if(!string) {
				node->data.flt = 0.0;
				break;
			}
			res = strtod(node->data.string, 0);
			aga_free(string);
			node->data.flt = res;
			break;
		}
	}

	me->depth--;
}


static void aga_sgml_put_entity(HTStructured* me, int n) {
	(void) me;
	(void) n;
}

void HTOOM(const char* file, const char* func) {
	aga_log(file, "%s: out of memory", func);
	aga_error_abort();
}

enum aga_result aga_config_new(
		void* fp, aga_size_t count, struct aga_config_node* root) {

	enum aga_result result;

	/* TODO: Introduce shorthand `<str>' etc. tags and `bool' type. */
	static HTStructuredClass class = {
			"",

			(void (*)(HTStructured*)) aga_sgml_free,
			(void (*)(HTStructured*, HTError)) aga_sgml_abort,

			(void (*)(HTStructured*, char)) aga_sgml_putc,

			(void (*)(HTStructured*, const char*)) aga_sgml_puts,
			(void (*)(HTStructured*, const char*, unsigned)) aga_sgml_write,

			(void (*)(
					HTStructured*, int, const HTBool*,
					const char**)) aga_sgml_start_element,

			(void (*)(HTStructured*, int)) aga_sgml_end_element,

			(void (*)(HTStructured*, int)) aga_sgml_put_entity };

	HTStream* s;
	SGML_dtd dtd;
	HTTag tags[AGA_ELEMENT_COUNT] = { 0 };
	attr item_attributes[AGA_ITEM_ATTRIB_COUNT];
	struct aga_sgml_structured structured = { 0, { 0 }, 0 };
	aga_size_t i;

	memset(root, 0, sizeof(struct aga_config_node));

	result = aga_sgml_push(&structured, root);
	if(result) return result;

	structured.class = &class;

	tags[AGA_NODE_ROOT].name = "root";
	tags[AGA_NODE_ROOT].contents = SGML_ELEMENT;

	tags[AGA_NODE_ITEM].name = "item";
	tags[AGA_NODE_ITEM].contents = SGML_CDATA;
	tags[AGA_NODE_ITEM].attributes = item_attributes;
	tags[AGA_NODE_ITEM].number_of_attributes = AGA_LEN(item_attributes);

	item_attributes[AGA_ITEM_NAME].name = "name";
	item_attributes[AGA_ITEM_TYPE].name = "type";

	dtd.tags = tags;
	dtd.number_of_tags = AGA_LEN(tags);

	s = SGML_new(&dtd, (HTStructured*) &structured);

	for(i = 0; i < count; ++i) {
		int c = fgetc(fp);

		if(c == EOF) {
			if(feof(fp)) break;

			SGML_free(s);

			return aga_error_system_path(
					__FILE__, "fgetc", aga_config_debug_file);
		}

		SGML_character(s, (char) c);
	}

	SGML_free(s);

	return AGA_RESULT_OK;
}

void aga_free_node(struct aga_config_node* node) {
	aga_size_t i;

	for(i = 0; i < node->len; ++i) {
		aga_free_node(&node->children[i]);
	}

	if(node->type == AGA_STRING) aga_free(node->data.string);

	aga_free(node->name);
	aga_free(node->children);
}

enum aga_result aga_config_delete(struct aga_config_node* root) {
	if(!root) return AGA_RESULT_BAD_PARAM;

	aga_free_node(root);

	return AGA_RESULT_OK;
}

aga_bool_t aga_config_variable(
		const char* name, struct aga_config_node* node,
		enum aga_config_node_type type, void* value) {

	if(!name || !node || !value) return AGA_FALSE;

	if(aga_streql(node->name, name)) {
		if(node->type != type) {
			aga_log(__FILE__, "warn: wrong type for field `%s'", name);
			return AGA_TRUE;
		}
		switch(type) {
			default:; AGA_FALLTHROUGH;
			/* FALLTHROUGH */
			case AGA_NONE: break;
			case AGA_STRING: {
				*(char**) value = node->data.string;
				break;
			}
			case AGA_INTEGER: {
				*(aga_slong_t*) value = node->data.integer;
				break;
			}
			case AGA_FLOAT: {
				*(double*) value = node->data.flt;
				break;
			}
		}
		return AGA_TRUE;
	}

	return AGA_FALSE;
}

enum aga_result aga_config_lookup_raw(
		struct aga_config_node* root, const char** names, aga_size_t count,
		struct aga_config_node** out) {

	aga_size_t i;

	if(!root) return AGA_RESULT_BAD_PARAM;
	if(!names) return AGA_RESULT_BAD_PARAM;
	if(!out) return AGA_RESULT_BAD_PARAM;

	if(count == 0) {
		*out = root;
		return AGA_RESULT_OK;
	}

	for(i = 0; i < root->len; ++i) {
		struct aga_config_node* node = &root->children[i];

		if(!node || !node->name) continue;

		if(aga_streql(*names, node->name)) {
			enum aga_result result = aga_config_lookup_raw(
					node, names + 1, count - 1, out);
			if(!result) return result;
		}
	}

	return AGA_RESULT_MISSING_KEY;
}

static void aga_conftree_debug_name(
		const char** names, aga_size_t count, aga_fixed_buf_t* buf) {

	aga_size_t i;

	for(i = 0; i < count; ++i) {
		strcat(*buf, names[i]);
		if(i < count - 1) strcat(*buf, "/");
	}
}

enum aga_result aga_config_lookup(
		struct aga_config_node* root, const char** names, aga_size_t count,
		void* value, enum aga_config_node_type type, aga_bool_t err) {

	enum aga_result result;
	struct aga_config_node* node;

	if(!root) return AGA_RESULT_BAD_PARAM;
	if(!names) return AGA_RESULT_BAD_PARAM;
	if(!value) return AGA_RESULT_BAD_PARAM;

	if(err) result = aga_config_lookup_check(root, names, count, &node);
	else result = aga_config_lookup_raw(root, names, count, &node);
	if(result) return result;

	if(aga_config_variable(node->name, node, type, value)) return AGA_RESULT_OK;
	else return AGA_RESULT_BAD_TYPE;
}

enum aga_result aga_config_lookup_check(
		struct aga_config_node* root, const char** names, aga_size_t count,
		struct aga_config_node** out) {

	enum aga_result result = aga_config_lookup_raw(root, names, count, out);
	if(result) {
		aga_fixed_buf_t buf = { 0 };
		aga_conftree_debug_name(names, count, &buf);

		aga_log(__FILE__, "err: Key `%s' not found in conf tree", buf);
	}

	return result;
}

static enum aga_result aga_dumpf(void* fp, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	if(vfprintf(fp, fmt, ap) < 0) {
		va_end(ap);
		return aga_error_system(__FILE__, "vfprintf");
	}

	va_end(ap);
	return AGA_RESULT_OK;
}

static enum aga_result aga_dumptree_int(
		struct aga_config_node* node, void* fp, aga_size_t depth) {

#ifdef AGA_DEVBUILD
	enum aga_result result;
	aga_size_t i;

	for(i = 0; i < node->len; ++i) {
		static const char fmt[] = "<item name=\"%s\" type=\"%s\">\n";
		static const char* tnames[] = { "None", "String", "Integer", "Float" };

		struct aga_config_node* n = &node->children[i];
		const char* tname = tnames[n->type];
		const char* nname = n->name ? n->name : "(none)";

		if((result = aga_file_print_characters('\t', depth, fp))) return result;
		if((result = aga_dumpf(fp, fmt, nname, tname))) return result;
		if(n->type) {
			if((result = aga_file_print_characters('\t', depth + 1, fp))) return result;
			switch(n->type) {
				default: break;
				case AGA_STRING: {
					const char* s = n->data.string ? n->data.string : "";
					result = aga_dumpf(fp, "%s\n", s);
					if(result) return result;
					break;
				}
				case AGA_INTEGER: {
					result = aga_dumpf(fp, "%lld\n", n->data.integer);
					if(result) return result;
					break;
				}
				case AGA_FLOAT: {
					result = aga_dumpf(fp, "%lf\n", n->data.flt);
					if(result) return result;
					break;
				}
			}
		}
		if((result = aga_dumptree_int(n, fp, depth + 1))) return result;
		if((result = aga_file_print_characters('\t', depth, fp))) return result;
		if((result = aga_dumpf(fp, "</item>\n"))) return result;
	}

	return AGA_RESULT_OK;
#else
	(void) node;
	(void) fp;
	(void) depth;

	aga_log(
			__FILE__,
			"err: Serialising conf trees is only supported in dev builds");

	return AGA_RESULT_BAD_OP;
#endif
}

/*
 * TODO: Make generic tree traversal callback-based function to decrease code
 * 		 Duplication in here.
 */
enum aga_result aga_config_dump(struct aga_config_node* root, void* fp) {
	enum aga_result result;

	if((result = aga_dumpf(fp, "<root>\n"))) return result;
	if((result = aga_dumptree_int(root, fp, 1))) return result;
	if((result = aga_dumpf(fp, "</root>"))) return result;

	return AGA_RESULT_OK;
}
