/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <aga/config.h>

#include <asys/result.h>
#include <asys/log.h>
#include <asys/string.h>
#include <asys/memory.h>
#include <asys/stream.h>
#include <asys/error.h>

/* Very nasty dependency to leak - keep it contained! */
#include <SGML.h>

enum aga_sgml_node {
	AGA_NODE_ITEM,
	AGA_NODE_ROOT
};

enum aga_sgml_item_attribute {
	AGA_ITEM_NAME,
	AGA_ITEM_TYPE
};

#define AGA_CONFIG_MAX_DEPTH (1024)

#define AGA_CONFIG_INTEGER_DEFAULT (ASYS_MAKE_NATIVE_LONG(0))
#define AGA_CONFIG_FLOAT_DEFAULT (0.0)

struct aga_sgml_structured {
	const HTStructuredClass* class;

	struct aga_config_node* stack[1024];
	asys_size_t depth;
};

void SGML_character(HTStream*, char);
void SGML_free(HTStream*);

static void aga_sgml_free(struct aga_sgml_structured*);
static void aga_sgml_abort(struct aga_sgml_structured*, HTError);
static void aga_sgml_put_character(struct aga_sgml_structured*, char);
static void aga_sgml_put_string(HTStructured*, const char*);
static void aga_sgml_write(HTStructured*, const char*, unsigned);
static void aga_sgml_start_element(
		struct aga_sgml_structured*, int, const HTBool*, char**);

static void aga_sgml_end_element(struct aga_sgml_structured*, int);
static void aga_sgml_put_entity(HTStructured*, int);

static attr aga_global_sgml_item_attributes[] = {
		{ "name" }, /* AGA_ITEM_NAME */
		{ "type" } /* AGA_ITEM_TYPE */
};

/* TODO: Introduce shorthand `<str>' etc. tags and `bool' type. */
/* NOTE: Must be in `strcmp' order by-name. Effectively alphabetical order. */
static HTTag aga_global_sgml_tags[] = {
		/* AGA_NODE_ITEM */
		{
				"item",
				aga_global_sgml_item_attributes,
				ASYS_LENGTH(aga_global_sgml_item_attributes),
				SGML_CDATA
		},

		/* AGA_NODE_ROOT */
		{
				"root",
				0,
				0,
				SGML_ELEMENT
		}
};

static SGML_dtd aga_global_sgml_dtd = {
		aga_global_sgml_tags, ASYS_LENGTH(aga_global_sgml_tags), 0, 0
};

static HTStructuredClass aga_global_sgml_class[1] = {
		{
				"aga-sgml",

				(HTStructuredFree) aga_sgml_free,
				(HTStructuredAbort) aga_sgml_abort,

				(HTStructuredPutCharacter) aga_sgml_put_character,
				(HTStructuredPutString) aga_sgml_put_string,

				(HTStructuredWrite) aga_sgml_write,

				(HTStructuredStartElement) aga_sgml_start_element,
				(HTStructuredEndElement) aga_sgml_end_element,

				(HTStructuredPutEntity) aga_sgml_put_entity
		}
};

static struct aga_sgml_structured aga_global_sgml_structured = {
		aga_global_sgml_class,
		{ 0 },
		0
};

static enum asys_result aga_sgml_push(
		struct aga_sgml_structured* s, struct aga_config_node* node) {

	if(!s) return ASYS_RESULT_BAD_PARAM;
	if(!node) return ASYS_RESULT_BAD_PARAM;

	if(s->depth > AGA_CONFIG_MAX_DEPTH) return ASYS_RESULT_OOM;

	s->stack[s->depth++] = node;

	/*asys_log(
			__FILE__,
			"Pushing config node `%p' at depth `" ASYS_NATIVE_ULONG_FORMAT "'",
			node, s->depth);*/

	return ASYS_RESULT_OK;
}

static void aga_sgml_free(struct aga_sgml_structured* me) {
	(void) me;
}

static void aga_sgml_abort(struct aga_sgml_structured* me, HTError e) {
	(void) me;
	(void) e;
}

static void aga_sgml_put_character(struct aga_sgml_structured* me, char c) {
	struct aga_config_node* node;

	if(!me->depth) {
		asys_log(
				__FILE__,
				"warn: aga_sgml_put_character: Read `%c' with empty node "
				"stack", c);

		return;
	}

	node = me->stack[me->depth - 1];

	/*asys_log(__FILE__, "aga_sgml_put_character: %c", c);*/

	if(node->type == AGA_NONE) return;
	if(!node->data.string && asys_character_is_blank(c)) return;

	node->data.string = asys_memory_reallocate_safe(
			node->data.string, ++node->scratch + 1);

	if(!node->data.string) return;

	node->data.string[node->scratch - 1] = c;
	node->data.string[node->scratch] = 0;
}

static void aga_sgml_put_string(HTStructured* me, const char* str) {
	(void) me;
	(void) str;
}

static void aga_sgml_write(HTStructured* me, const char* str, unsigned len) {
	(void) me;
	(void) str;
	(void) len;
}

static void aga_sgml_start_element(
		struct aga_sgml_structured* me, int element_number,
		const HTBool* attribute_present, char** attribute_value) {

	struct aga_config_node* parent;
	struct aga_config_node* node;
	enum asys_result result;

	asys_size_t sz;

	if(!me->depth) {
		asys_log(
				__FILE__,
				"warn: aga_sgml_start_element: Start element type `%s' with "
				"empty node stack", aga_global_sgml_tags[element_number].name);

		return;
	}

	parent = me->stack[me->depth - 1];
	sz = ++parent->len * sizeof(struct aga_config_node);

	/*asys_log(
			__FILE__, "aga_sgml_start_element: %d -> %s",
			element_number, aga_global_sgml_tags[element_number].name);*/

	parent->children = asys_memory_reallocate_safe(parent->children, sz);
	if(!parent->children) {
		parent->len = 0;
		return;
	}

	node = &parent->children[parent->len - 1];
	asys_memory_zero(node, sizeof(struct aga_config_node));

	if((result = aga_sgml_push(me, node))) {
		asys_log_result(__FILE__, "aga_sgml_push", result);
		return;
	}

	switch(element_number) {
		default: {
			asys_log(
					__FILE__, "warn: SGML_new: unknown element %i",
					element_number);

			return;
		}
		case AGA_NODE_ITEM: {
			if(!attribute_present[AGA_ITEM_NAME]) node->name = 0;
			else {
				const char* value = attribute_value[AGA_ITEM_NAME];
				/*
				 * TODO: Clear new alloc on OOM (?). Can a shrinking realloc
				 * 		 Fail?
				 */
				if(!(node->name = asys_string_duplicate(value))) return;
			}

			if(!attribute_present[AGA_ITEM_TYPE]) node->type = AGA_NONE;
			else {
				const char* typename = attribute_value[AGA_ITEM_TYPE];

				if(asys_string_equal(typename, "Integer")) {
					node->type = AGA_INTEGER;
				}
				else if(asys_string_equal(typename, "String")) {
					node->type = AGA_STRING;
				}
				else if(asys_string_equal(typename, "Float")) {
					node->type = AGA_FLOAT;
				}
				else if(asys_string_equal(typename, "None")) {
					node->type = AGA_NONE;
				}
				else {
					asys_log(
							__FILE__,
							"warn: <item> element has unknown type `%s'",
							typename);

					node->type = AGA_NONE;
				}
			}
		}
		case AGA_NODE_ROOT: break; /* No attribs */
	}
}

static void aga_sgml_end_element(
		struct aga_sgml_structured* me, int element_number) {

	asys_size_t i;

	struct aga_config_node* node;
	char* string;

	if(!me->depth) {
		asys_log(
				__FILE__,
				"warn: aga_sgml_end_element: End element type `%s' with empty "
				"node stack", aga_global_sgml_tags[element_number]);

		return;
	}

	node = me->stack[me->depth - 1];
	string = node->data.string;

	/*asys_log(
			__FILE__, "aga_sgml_end_element: %d -> %s",
			element_number, aga_global_sgml_tags[element_number].name);*/

	for(i = 0; string && i <= node->scratch; ++i) {
		asys_size_t n = node->scratch - i;
		char* c = &string[n];

		if(asys_character_is_blank(*c)) *c = 0;
		else if(*c) break;
	}

	switch(node->type) {
		default: break;
		case AGA_INTEGER: {
			aga_config_int_t res;
			if(!string) {
				asys_log(
						__FILE__,
						"warn: No data for `Integer' config node `%s', "
						"using default value `" ASYS_NATIVE_LONG_FORMAT "'...",
						node->name, AGA_CONFIG_INTEGER_DEFAULT);

				node->data.integer = AGA_CONFIG_INTEGER_DEFAULT;
				break;
			}

			res = asys_string_to_native_long(node->data.string, 0);
			asys_memory_free(string);
			node->data.integer = res;

			break;
		}
		case AGA_FLOAT: {
			double res;

			if(!string) {
				asys_log(
						__FILE__,
						"warn: No data for `Float' config node `%s', "
						"using default value...",
						node->name);

				node->data.flt = AGA_CONFIG_FLOAT_DEFAULT;
				break;
			}

			res = asys_string_to_double(node->data.string, 0);
			asys_memory_free(string);
			node->data.flt = res;

			break;
		}
	}

	me->depth--;

	/*asys_log(
			__FILE__,
			"Popping config node `%p' at depth `" ASYS_NATIVE_ULONG_FORMAT "'",
			node, me->depth);*/
}


static void aga_sgml_put_entity(HTStructured* me, int n) {
	(void) me;
	(void) n;
}

void HTOOM(const char* file, const char* func) {
	asys_result_fatal(file, func, ASYS_RESULT_OOM);
}

enum asys_result aga_config_new(
		struct asys_stream* stream, asys_size_t count,
		struct aga_config_node* root) {

	enum asys_result result;

	HTStream* s;
	asys_size_t i;

#if !defined(AGA_DEVBUILD) && !defined(NDEBUG)
	if(count == AGA_CONFIG_EOF) {
		asys_log(__FILE__, "`AGA_CONFIG_EOF' is only available in dev builds");
		return ASYS_RESULT_BAD_PARAM;
	}
#endif

	asys_memory_zero(root, sizeof(struct aga_config_node));

	result = aga_sgml_push(&aga_global_sgml_structured, root);
	if(result) return result;

	s = SGML_new(&aga_global_sgml_dtd, (void*) &aga_global_sgml_structured);

	for(i = 0; i < count; ++i) {
		char c;

		if((result = asys_stream_read(stream, 0, &c, 1))) {
			if(result == ASYS_RESULT_EOF) break;

			SGML_free(s);
			return result;
		}

		SGML_character(s, (char) c);
	}

	SGML_free(s);

	return ASYS_RESULT_OK;
}

void aga_free_node(struct aga_config_node* node) {
	asys_size_t i;

	if(!node) return;

	for(i = 0; i < node->len; ++i) {
		aga_free_node(&node->children[i]);
	}

	if(node->type == AGA_STRING) asys_memory_free(node->data.string);

	asys_memory_free(node->name);
	asys_memory_free(node->children);
}

enum asys_result aga_config_delete(struct aga_config_node* root) {
	if(!root) return ASYS_RESULT_BAD_PARAM;

	aga_free_node(root);

	return ASYS_RESULT_OK;
}

asys_bool_t aga_config_variable(
		const char* name, struct aga_config_node* node,
		enum aga_config_node_type type, void* value) {

	if(!name || !node || !value) return ASYS_FALSE;

	if(asys_string_equal(node->name, name)) {
		if(type == AGA_PATH) {
			char* path;
			asys_size_t i;

			if(node->type != AGA_STRING) goto bad_type;

			*(char**) value = path = asys_string_duplicate(node->data.string);

#ifdef ASYS_WIN32
			for(i = 0; path[i]; ++i) {
				if(path[i] == '/') path[i] = '\\';
			}
#else
			(void) i;
#endif

			return ASYS_TRUE;
		}
		if(node->type != type) {
			bad_type: {
				/* TODO: This should default the output value. */
				asys_log(__FILE__, "warn: wrong type for field `%s'", name);
				return ASYS_TRUE;
			};
		}
		switch(type) {
			default:; ASYS_FALLTHROUGH;
			/* FALLTHROUGH */
			case AGA_NONE: break;
			case AGA_STRING: {
				*(char**) value = node->data.string;
				break;
			}
			case AGA_INTEGER: {
				*(aga_config_int_t*) value = node->data.integer;
				break;
			}
			/*
			 * TODO: Could we add a float-free operation mode (with configured
			 * 		 Fixed point scaling).
			 */
			case AGA_FLOAT: {
				*(double*) value = node->data.flt;
				break;
			}
		}

		return ASYS_TRUE;
	}

	return ASYS_FALSE;
}

enum asys_result aga_config_lookup_raw(
		struct aga_config_node* root, const char** names, asys_size_t count,
		struct aga_config_node** out) {

	asys_size_t i;

	if(!root) return ASYS_RESULT_BAD_PARAM;
	if(!names) return ASYS_RESULT_BAD_PARAM;
	if(!out) return ASYS_RESULT_BAD_PARAM;

	if(count == 0) {
		*out = root;
		return ASYS_RESULT_OK;
	}

	for(i = 0; i < root->len; ++i) {
		struct aga_config_node* node = &root->children[i];

		if(!node || !node->name) continue;

		if(asys_string_equal(*names, node->name)) {
			enum asys_result result = aga_config_lookup_raw(
					node, names + 1, count - 1, out);

			if(!result) return result;
		}
	}

	return ASYS_RESULT_MISSING_KEY;
}

static void aga_config_debug_name(
		const char** names, asys_size_t count, asys_fixed_buffer_t* buffer) {

	asys_size_t i;

	for(i = 0; i < count; ++i) {
		asys_string_concatenate(*buffer, names[i]);

		if(i < count - 1) asys_string_concatenate(*buffer, "/");
	}
}

enum asys_result aga_config_lookup(
		struct aga_config_node* root, const char** names, asys_size_t count,
		void* value, enum aga_config_node_type type, asys_bool_t do_error) {

	enum asys_result result;
	struct aga_config_node* node;

	if(!root) return ASYS_RESULT_BAD_PARAM;
	if(!names) return ASYS_RESULT_BAD_PARAM;
	if(!value) return ASYS_RESULT_BAD_PARAM;

	if(do_error) result = aga_config_lookup_check(root, names, count, &node);
	else result = aga_config_lookup_raw(root, names, count, &node);

	if(result) return result;

	if(aga_config_variable(node->name, node, type, value)) return ASYS_RESULT_OK;
	else return ASYS_RESULT_BAD_TYPE;
}

enum asys_result aga_config_lookup_check(
		struct aga_config_node* root, const char** names, asys_size_t count,
		struct aga_config_node** out) {

	enum asys_result result = aga_config_lookup_raw(root, names, count, out);
	if(result) {
		asys_fixed_buffer_t buffer = { 0 };
		aga_config_debug_name(names, count, &buffer);

		asys_log(__FILE__, "err: Key `%s' not found in config tree", buffer);
	}

	return result;
}

#ifdef AGA_DEVBUILD
static enum asys_result aga_config_dump_tree(
		struct aga_config_node* node, struct asys_stream* stream,
		asys_size_t depth) {

	static const char item[] = "<item name=\"%s\" type=\"%s\">\n";
	static const char* type_names[] = { "None", "String", "Integer", "Float" };

	enum asys_result result;
	asys_size_t i;

	for(i = 0; i < node->len; ++i) {
		struct aga_config_node* child = &node->children[i];
		const char* type = type_names[child->type];
		const char* name = child->name ? child->name : "(none)";

		result = asys_stream_write_characters(stream, '\t', depth);
		if(result) return result;

		result = asys_stream_write_format(stream, item, name, type);
		if(result) return result;

		if(child->type) {
			result = asys_stream_write_characters(stream, '\t', depth + 1);
			if(result) return result;

			switch(child->type) {
				default: break;
				case AGA_STRING: {
					const char* string;
					string = child->data.string ? child->data.string : "";

					result = asys_stream_write_format(stream, "%s\n", string);
					if(result) return result;

					break;
				}
				case AGA_INTEGER: {
					result = asys_stream_write_format(
							stream, ASYS_NATIVE_LONG_FORMAT "\n",
							child->data.integer);

					if(result) return result;

					break;
				}
				case AGA_FLOAT: {
					result = asys_stream_write_format(
							stream, "%lf\n", child->data.flt);

					if(result) return result;

					break;
				}
			}
		}

		result = aga_config_dump_tree(child, stream, depth + 1);
		if(result) return result;

		result = asys_stream_write_characters(stream, '\t', depth);
		if(result) return result;

		result = asys_stream_write_format(stream, "</item>\n");
		if(result) return result;
	}

	return ASYS_RESULT_OK;
}
#endif

/*
 * TODO: Make generic tree traversal callback-based function to decrease code
 * 		 Duplication in here.
 */
enum asys_result aga_config_dump(
		struct aga_config_node* root, struct asys_stream* stream) {

#ifdef AGA_DEVBUILD
	enum asys_result result;

	if(!root) return ASYS_RESULT_BAD_PARAM;
	if(!stream) return ASYS_RESULT_BAD_PARAM;

	result = asys_stream_write_format(stream, "<root>\n");
	if(result) return result;

	if((result = aga_config_dump_tree(root, stream, 1))) return result;

	result = asys_stream_write_format(stream, "</root>\n");
	if(result) return result;

	return ASYS_RESULT_OK;
#else
	(void) root;
	(void) stream;

	return ASYS_RESULT_NOT_IMPLEMENTED;
#endif
}
