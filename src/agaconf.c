/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <afeirsa/afeirsa.h>

#include <agacore.h>
#include <agaconf.h>
#include <agalog.h>
#include <agaio.h>

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
	const char* file;

	struct aga_conf_node* stack[1024];
	af_size_t depth;
};

/* For whatever reason these are missing from the header */
void SGML_write(HTStream* context, const char* str, int l);
void SGML_free(HTStream* context);

enum af_err aga_sgml_push(
		struct aga_sgml_structured* s, struct aga_conf_node* node) {

	AF_PARAM_CHK(s);
	AF_PARAM_CHK(node);

	AF_VERIFY(s->depth <= AGA_CONF_MAX_DEPTH, AF_ERR_MEM); /* AF_ERR_STACK */

	s->stack[s->depth++] = node;

	return AF_ERR_NONE;
}

void aga_sgml_nil(void) {}
void aga_sgml_putc(struct aga_sgml_structured* me, char c) {
	struct aga_conf_node* node = me->stack[me->depth - 1];
	void* newptr;

	if(node->type == AGA_NONE) return;
	if(c == ' ' || c == '\r' || c == '\t' || c == '\n') return;

	newptr = realloc(node->data.string, ++node->scratch + 1);
	if(!newptr) {
		aga_af_errno(__FILE__, "realloc");
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
	enum af_err result;

	af_size_t sz = ++parent->len * sizeof(struct aga_conf_node);

	newptr = realloc(parent->children, sz);
	if(!newptr) {
		aga_af_errno(__FILE__, "realloc");
		free(parent->children);
		parent->children = 0;
		parent->len = 0;
		return;
	}
	parent->children = newptr;

	node = &parent->children[parent->len - 1];
	af_memset(node, 0, sizeof(struct aga_conf_node));

	if((result = aga_sgml_push(me, node))) {
		aga_af_soft(__FILE__, "aga_sgml_push", result);
		/*
		 * TODO: This can be a soft misuse of memory as we will leave haps in
		 * 		 The child list.
		 */
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
					"warn: <item> element without name attrib in `%s'\n",
					me->file);
			}
			else {
				const char* value = attribute_value[AGA_ITEM_NAME];
				af_size_t len = af_strlen(value);
				if(!(node->name = malloc(len + 1))) {
					aga_af_errno(__FILE__, "malloc");
					return;
				}
				af_memcpy(node->name, value, len + 1);
			}

			if(!attribute_present[AGA_ITEM_TYPE]) node->type = AGA_NONE;
			else {
				const char* typename = attribute_value[AGA_ITEM_TYPE];
				if(af_streql(typename, "Integer")) node->type = AGA_INTEGER;
				else if(af_streql(typename, "String")) node->type = AGA_STRING;
				else if(af_streql(typename, "Float")) node->type = AGA_FLOAT;
				else {
					static const char fmt[] =
						"warn: <item> element has unknown type `%s' in `%s'\n";
					aga_log(__FILE__, fmt, typename, me->file);
					node->type = AGA_NONE;
				}
			}
		}
		case AGA_NODE_ROOT: break; /* No attribs */
	}
}

void aga_sgml_end_element(struct aga_sgml_structured* me, int element_number) {
	struct aga_conf_node* node = me->stack[me->depth - 1];
	char* string = node->data.string;

	(void) element_number;

	/* TODO: lstrip+rstrip string data to get rid of all the guff */

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

enum af_err aga_mkconf(const char* path, struct aga_conf_node* root) {
	HTStream* s;
	HTStructuredClass class = {
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
	SGML_dtd dtd;
	HTTag tags[AGA_ELEMENT_COUNT] = { 0 };
	attr item_attributes[AGA_ITEM_ATTRIB_COUNT];
	struct aga_sgml_structured structured = {
		0, 0, { 0 }, 0
	};
	structured.file = path;
	AF_CHK(aga_sgml_push(&structured, root));

	structured.class = &class;

	/*
	 * NOTE: If your SGML is misbehaving - give this a try.
	 * 		 Should we have a way to set this at runtime?
	WWW_TraceFlag = 1;
	 */

	tags[AGA_NODE_ROOT].name = "root";
	tags[AGA_NODE_ROOT].contents = SGML_ELEMENT;

	tags[AGA_NODE_ITEM].name = "item";
	tags[AGA_NODE_ITEM].contents = SGML_CDATA;
	tags[AGA_NODE_ITEM].attributes = item_attributes;
	tags[AGA_NODE_ITEM].number_of_attributes = AF_ARRLEN(item_attributes);

	item_attributes[AGA_ITEM_NAME].name = "name";
	item_attributes[AGA_ITEM_TYPE].name = "type";

	dtd.tags = tags;
	dtd.number_of_tags = AF_ARRLEN(tags);

	s = SGML_new(&dtd, (HTStructured*) &structured);

	{
		af_uchar_t* buf;
		af_size_t size;
		AF_CHK(AGA_MK_LARGE_FILE_STRATEGY(path, &buf, &size));

		SGML_write(s, (char*) buf, (int) size);

		AF_CHK(AGA_KILL_LARGE_FILE_STRATEGY(buf, size));
	}

	SGML_free(s);

	return AF_ERR_NONE;
}

void aga_free_node(struct aga_conf_node* node) {
	af_size_t i;

	for(i = 0; i < node->len; ++i) {
		aga_free_node(&node->children[i]);
	}

	if(node->type == AGA_STRING) free(node->data.string);

	free(node->name);
	free(node->children);
}

enum af_err aga_killconf(struct aga_conf_node* root) {
	AF_PARAM_CHK(root);

	aga_free_node(root);

	return AF_ERR_NONE;
}
