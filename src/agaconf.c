/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agaresult.h>

#include <agaerr.h>
#include <agaconf.h>
#include <agalog.h>
#include <agaio.h>
#include <agautil.h>
#include <agastd.h>

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

#define AGA_CONF_MAX_DEPTH (1024)

struct aga_sgml_structured {
	const HTStructuredClass* class;

	struct aga_conf_node* stack[1024];
	aga_size_t depth;
};

const char* aga_conf_debug_file = "<none>";

void SGML_character(HTStream* context, char c);

void SGML_free(HTStream* context);

static aga_bool_t aga_isblank(char c) {
	return c == ' ' || c == '\r' || c == '\t' || c == '\n';
}

static enum aga_result aga_sgml_push(
		struct aga_sgml_structured* s, struct aga_conf_node* node) {

	if(!s) return AGA_RESULT_BAD_PARAM;
	if(!node) return AGA_RESULT_BAD_PARAM;

	if(s->depth > AGA_CONF_MAX_DEPTH) return AGA_RESULT_OOM;

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
	struct aga_conf_node* node = me->stack[me->depth - 1];

	if(node->type == AGA_NONE) return;
	if(!node->data.string && aga_isblank(c)) return;

	node->data.string = aga_realloc(node->data.string, ++node->scratch + 1);
	if(!node->data.string) {
		aga_errno(__FILE__, "aga_realloc");
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

	struct aga_conf_node* parent = me->stack[me->depth - 1];
	struct aga_conf_node* node;
	enum aga_result result;

	aga_size_t sz = ++parent->len * sizeof(struct aga_conf_node);

	if(!(parent->children = aga_realloc(parent->children, sz))) {
		aga_errno(__FILE__, "aga_realloc");
		parent->len = 0;
		return;
	}

	node = &parent->children[parent->len - 1];
	memset(node, 0, sizeof(struct aga_conf_node));

	if((result = aga_sgml_push(me, node))) {
		aga_soft(__FILE__, "aga_sgml_push", result);
		return;
	}

	switch(element_number) {
		default: {
			aga_log(
					__FILE__, "warn: SGML_new: unknown element %i",
					element_number);
			return;
		}
		case AGA_NODE_ITEM: {
			if(!attribute_present[AGA_ITEM_NAME]) {
				aga_log(
						__FILE__,
						"warn: <item> element without name attrib `%s'",
						aga_conf_debug_file);
			}
			else {
				const char* value = attribute_value[AGA_ITEM_NAME];
				aga_size_t len = strlen(value);
				if(!(node->name = aga_malloc(len + 1))) {
					aga_errno(__FILE__, "aga_malloc");
					return;
				}
				memcpy(node->name, value, len + 1);
			}

			if(!attribute_present[AGA_ITEM_TYPE]) { node->type = AGA_NONE; }
			else {
				const char* typename = attribute_value[AGA_ITEM_TYPE];
				if(aga_streql(
						typename, "Integer")) { node->type = AGA_INTEGER; }
				else if(aga_streql(
						typename, "String")) { node->type = AGA_STRING; }
				else if(aga_streql(
						typename, "Float")) { node->type = AGA_FLOAT; }
				else if(aga_streql(
						typename, "None")) { node->type = AGA_NONE; }
				else {
					static const char fmt[] =
							"warn: <item> element has unknown type "
							"`%s' in `%s'";
					aga_log(__FILE__, fmt, typename, aga_conf_debug_file);
					node->type = AGA_NONE;
				}
			}
		}
		case AGA_NODE_ROOT: break; /* No attribs */
	}
}

void aga_sgml_end_element(struct aga_sgml_structured* me, int element_number) {
	aga_size_t i;

	struct aga_conf_node* node = me->stack[me->depth - 1];
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

void outofmem(const char* file, const char* func) {
	aga_log(file, "%s: out of memory", func);
	aga_abort();
}

enum aga_result aga_mkconf(
		void* fp, aga_size_t count, struct aga_conf_node* root) {

	enum aga_result result;

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

	memset(root, 0, sizeof(struct aga_conf_node));

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
			SGML_free(s);
			return aga_errno_path(__FILE__, "fgetc", aga_conf_debug_file);
		}

		SGML_character(s, (char) c);
	}

	SGML_free(s);

	return AGA_RESULT_OK;
}

void aga_free_node(struct aga_conf_node* node) {
	aga_size_t i;

	for(i = 0; i < node->len; ++i) {
		aga_free_node(&node->children[i]);
	}

	if(node->type == AGA_STRING) aga_free(node->data.string);

	aga_free(node->name);
	aga_free(node->children);
}

enum aga_result aga_killconf(struct aga_conf_node* root) {
	if(!root) return AGA_RESULT_BAD_PARAM;

	aga_free_node(root);

	return AGA_RESULT_OK;
}

aga_bool_t aga_confvar(
		const char* name, struct aga_conf_node* node, enum aga_conf_type type,
		void* value) {

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
				*(int*) value = (int) node->data.integer;
				break;
			}
			case AGA_FLOAT: {
				*(float*) value = (float) node->data.flt;
				break;
			}
		}
		return AGA_TRUE;
	}

	return AGA_FALSE;
}

enum aga_result aga_conftree_raw(
		struct aga_conf_node* root, const char** names, aga_size_t count,
		struct aga_conf_node** out) {

	aga_size_t i;

	if(!root) return AGA_RESULT_BAD_PARAM;
	if(!names) return AGA_RESULT_BAD_PARAM;
	if(!out) return AGA_RESULT_BAD_PARAM;

	if(count == 0) {
		*out = root;
		return AGA_RESULT_OK;
	}

	for(i = 0; i < root->len; ++i) {
		struct aga_conf_node* node = &root->children[i];
		if(aga_streql(*names, node->name)) {
			enum aga_result result = aga_conftree_raw(
					node, names + 1, count - 1, out);
			if(!result) return result;
		}
	}

	return AGA_RESULT_ERROR;
}

enum aga_result aga_conftree(
		struct aga_conf_node* root, const char** names, aga_size_t count,
		void* value, enum aga_conf_type type) {

	enum aga_result result;
	struct aga_conf_node* node;

	if(!root) return AGA_RESULT_BAD_PARAM;
	if(!names) return AGA_RESULT_BAD_PARAM;
	if(!value) return AGA_RESULT_BAD_PARAM;

	result = aga_conftree_raw(root, names, count, &node);
	if(result) return result;

	if(aga_confvar(node->name, node, type, value)) { return AGA_RESULT_OK; }
	else { return AGA_RESULT_ERROR; }
}

static enum aga_result aga_dumpf(void* fp, const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	if(vfprintf(fp, fmt, ap) < 0) {
		va_end(ap);
		return aga_errno(__FILE__, "vfprintf");
	}

	va_end(ap);
	return AGA_RESULT_OK;
}

/*
 * TODO: Make a no-write mode to avoid importability to immutable storage
 * 		 Systems/"Edit builds". (?)
 */
static enum aga_result aga_dumptree_int(
		struct aga_conf_node* node, void* fp, aga_size_t depth) {

	enum aga_result result;
	aga_size_t i;

	for(i = 0; i < node->len; ++i) {
		static const char fmt[] = "<item name=\"%s\" type=\"%s\">\n";
		static const char* tnames[] = { "None", "String", "Integer", "Float" };

		struct aga_conf_node* n = &node->children[i];
		const char* tname = tnames[n->type];
		const char* nname = n->name ? n->name : "(none)";

		if((result = aga_fputn('\t', depth, fp))) return result;
		if((result = aga_dumpf(fp, fmt, nname, tname))) return result;
		if(n->type) {
			if((result = aga_fputn('\t', depth + 1, fp))) return result;
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
		if((result = aga_fputn('\t', depth, fp))) return result;
		if((result = aga_dumpf(fp, "</item>\n"))) return result;
	}

	return AGA_RESULT_OK;
}

/*
 * TODO: Make generic tree traversal callback-based function to decrease code
 * 		 Duplication in here.
 */
enum aga_result aga_dumptree(struct aga_conf_node* root, void* fp) {
	enum aga_result result;

	if((result = aga_dumpf(fp, "<root>\n"))) return result;
	if((result = aga_dumptree_int(root, fp, 1))) return result;
	if((result = aga_dumpf(fp, "</root>"))) return result;

	return AGA_RESULT_OK;
}
