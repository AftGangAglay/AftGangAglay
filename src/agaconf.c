/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <afeirsa/afeirsa.h>

#include <agaconf.h>
#include <agacore.h>
#include <agaio.h>

/* Very nasty dependency to leak - keep it contained! */
#include <SGML.h>

/* For whatever reason these are missing from the header */
void SGML_write(HTStream* context, const char* str, int l);
void SGML_free(HTStream* context);

void test_free(HTStructured* me) {
	(void) me;
}

void test_abort(HTStructured* me, HTError e) {
	(void) me, (void) e;
	aga_fatal("SGML parsing failed");
}

void test_put_character(HTStructured* me, char ch) {
	(void) me;
	printf("PUTC: %c\n", ch);
}

void test_put_string(HTStructured* me, const char* str) {
	(void) me;
	printf("PUTS: %s\n", str);
}

void test_write(HTStructured* me, const char* str, int len) {
	(void) me, (void) len;
	printf("WRITE: %s\n", str);
}

void test_start_element(
		HTStructured* me, int element_number, const BOOL* attribute_present,
		const char** attribute_value) {

	(void) me;
	printf(
		"START ELEMENT: %i %s\n",
		element_number, attribute_present ? "TRUE" : "FALSE");

	if(attribute_present) {
		while(*attribute_value) {
			printf("\tATTRIB: %s\n", *attribute_value);
			attribute_value++;
		}
	}
}

void test_end_element(HTStructured* me, int element_number) {
	(void) me;
	printf("END ELEMENT: %i\n", element_number);
}

void test_put_entity(HTStructured* me, int entity_number) {
	(void) me;
	printf("PUT ENTITY: %i\n", entity_number);
}

enum af_err aga_test_sgml(const char* path) {
	HTStream* s;
	HTStructuredClass class = {
		"aga-test-parser",
		test_free,
		test_abort,
		test_put_character,
		test_put_string,
		test_write,
		test_start_element,
		test_end_element,
		test_put_entity
	};
	HTStructuredClass* classp = &class;
	SGML_dtd dtd;
	HTTag tags[] = {
		{ "conf", 0, 0, SGML_MIXED },
		{ "item", 0, 0, SGML_CDATA }
	};
	attr item_attributes[] = {
		{ "name" },
		{ "type" }
	};

	tags[1].attributes = item_attributes;
	tags[1].number_of_attributes = AF_ARRLEN(item_attributes);

	dtd.tags = tags;
	dtd.number_of_tags = AF_ARRLEN(tags);

	s = SGML_new(&dtd, (HTStructured*) &classp);

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
