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

/* Very nasty dependency to leak - keep it contained! */
#include <SGML.h>

/*
 * NOTE: These need to be in `strcmp' order (effectively alphabetical for us)
 */
enum aga_sgml_node {
	AGA_NODE_ITEM,
	AGA_NODE_ROOT,

	AGA_ELEMENT_COUNT
};

enum aga_sgml_item_attribs {
	AGA_ITEM_NAME,
	AGA_ITEM_TYPE,

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

aga_bool_t aga_isblank(char c) {
	return c == ' ' || c == '\r' || c == '\t' || c == '\n';
}

enum aga_result aga_sgml_push(
		struct aga_sgml_structured* s, struct aga_conf_node* node) {

	AGA_PARAM_CHK(s);
	AGA_PARAM_CHK(node);

	AGA_VERIFY(s->depth <= AGA_CONF_MAX_DEPTH, AGA_RESULT_OOM);

	s->stack[s->depth++] = node;

	return AGA_RESULT_OK;
}

void aga_sgml_nil(void) {}
void aga_sgml_putc(struct aga_sgml_structured* me, char c) {
	struct aga_conf_node* node = me->stack[me->depth - 1];
	void* newptr;

	if(node->type == AGA_NONE) return;
	if(!node->data.string && aga_isblank(c)) return;

	newptr = realloc(node->data.string, ++node->scratch + 1);
	if(!newptr) {
		aga_errno(__FILE__, "realloc");
		free(node->data.string);
		node->data.string = 0;
	}
	node->data.string = newptr;

	node->data.string[node->scratch - 1] = c;
	node->data.string[node->scratch] = 0;
}

void aga_sgml_start_element(
		struct aga_sgml_structured* me, int element_number,
		const BOOL* attribute_present, char** attribute_value) {

	struct aga_conf_node* parent = me->stack[me->depth - 1];
	struct aga_conf_node* node;
	void* newptr;
	enum aga_result result;

	aga_size_t sz = ++parent->len * sizeof(struct aga_conf_node);

	newptr = realloc(parent->children, sz);
	if(!newptr) {
		aga_errno(__FILE__, "realloc");
		free(parent->children);
		parent->children = 0;
		parent->len = 0;
		return;
	}
	parent->children = newptr;

	node = &parent->children[parent->len - 1];
	memset(node, 0, sizeof(struct aga_conf_node));

	if((result = aga_sgml_push(me, node))) {
		aga_soft(__FILE__, "aga_sgml_push", result);
		return;
	}

	switch(element_number) {
		default: {
			aga_log(
				__FILE__,
				"warn: SGML_new: unknown element %i", element_number);
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
				if(!(node->name = malloc(len + 1))) {
					aga_errno(__FILE__, "malloc");
					return;
				}
				memcpy(node->name, value, len + 1);
			}

			if(!attribute_present[AGA_ITEM_TYPE]) node->type = AGA_NONE;
			else {
				const char* typename = attribute_value[AGA_ITEM_TYPE];
				if(aga_streql(typename, "Integer")) node->type = AGA_INTEGER;
				else if(aga_streql(typename, "String")) node->type = AGA_STRING;
				else if(aga_streql(typename, "Float")) node->type = AGA_FLOAT;
				else {
					static const char fmt[] =
						"warn: <item> element has unknown type `%s' in `%s'";
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

		if(aga_isblank(*c)) *c = 0;
		else if(*c) break;
	}

	switch(node->type) {
		default: break;
		case AGA_INTEGER: {
			long res;
			if(!string) {
				node->data.integer = 0;
				break;
			}
			res = strtol(node->data.string, 0, 0);
			free(string);
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
			free(string);
			node->data.flt = res;
			break;
		}
	}

	me->depth--;
}

enum aga_result aga_mkconf(void* fp, aga_size_t count, struct aga_conf_node* root) {
	static HTStructuredClass class = {
		"",

		(void(*)(HTStructured*)) aga_sgml_nil,
		(void(*)(HTStructured*, HTError)) aga_sgml_nil,

		(void(*)(HTStructured*, char)) aga_sgml_putc,

		(void(*)(HTStructured*, const char*)) aga_sgml_nil,
		(void(*)(HTStructured*, const char*, int)) aga_sgml_nil,

		(void(*)(HTStructured*, int, const BOOLEAN*, const char**))
			aga_sgml_start_element,

		(void(*)(HTStructured*, int)) aga_sgml_end_element,

		(void(*)(HTStructured*, int)) aga_sgml_nil
	};

	HTStream* s;
	SGML_dtd dtd;
	HTTag tags[AGA_ELEMENT_COUNT] = { 0 };
	attr item_attributes[AGA_ITEM_ATTRIB_COUNT];
	struct aga_sgml_structured structured = { 0, { 0 }, 0 };
	aga_size_t i;

	memset(root, 0, sizeof(struct aga_conf_node));
	AGA_CHK(aga_sgml_push(&structured, root));

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

	/* TODO: Leaky error conditions as usual. */
	s = SGML_new(&dtd, (HTStructured*) &structured);

	for(i = 0; i < count; ++i) {
		int c = fgetc(fp);

		if(c == EOF) {
			return aga_patherrno(__FILE__, "fgetc", aga_conf_debug_file);
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

	if(node->type == AGA_STRING) free(node->data.string);

	free(node->name);
	free(node->children);
}

enum aga_result aga_killconf(struct aga_conf_node* root) {
	AGA_PARAM_CHK(root);

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
			default: {
                AGA_FALLTHROUGH;
				/* FALLTHRU */
            }
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

	AGA_PARAM_CHK(root);
	AGA_PARAM_CHK(names);
	AGA_PARAM_CHK(out);

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

enum aga_result aga_conftree_nonroot(
		struct aga_conf_node* root, const char** names, aga_size_t count,
		void* value, enum aga_conf_type type) {

	struct aga_conf_node* node;

	AGA_PARAM_CHK(root);
	AGA_PARAM_CHK(names);
	AGA_PARAM_CHK(value);

	AGA_CHK(aga_conftree_raw(root, names, count, &node));

	if(aga_confvar(node->name, node, type, value)) return AGA_RESULT_OK;
	else return AGA_RESULT_ERROR;
}

/* TODO: This should be removed and we should review our invocations. */
enum aga_result aga_conftree(
		struct aga_conf_node* root, const char** names, aga_size_t count,
		void* value, enum aga_conf_type type) {

	return aga_conftree_nonroot(root->children, names, count, value, type);
}
