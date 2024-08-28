/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023, 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#ifndef AGA_SCRIPT_H
#define AGA_SCRIPT_H

#include <aga/environment.h>
#include <aga/result.h>

#define AGA_SCRIPT_KEYMAP ("keymap")
#define AGA_SCRIPT_POINTER ("pointer")
#define AGA_SCRIPT_SETTINGS ("opts")
#define AGA_SCRIPT_SOUND_DEVICE ("sound_device")
#define AGA_SCRIPT_DIE ("die")
#define AGA_SCRIPT_WINDOW_DEVICE ("window_device")
#define AGA_SCRIPT_WINDOW ("window")
#define AGA_SCRIPT_RESOURCE_PACK ("resource_pack")
#define AGA_SCRIPT_BUTTONS ("buttons")

struct aga_resource;
struct aga_resource_pack;

struct aga_script_class {
	void* class;
};

struct aga_script_instance {
	struct aga_script_class* class;
	void* object;
};

struct aga_script_engine {
	struct py* py;
	struct py_env* env; /* TODO: This is temporary. */

	void* global;
	void* agan;
};

enum aga_result aga_script_engine_new(
		struct aga_script_engine*, const char*, struct aga_resource_pack*,
		const char*);

enum aga_result aga_script_engine_delete(struct aga_script_engine*);

/* TODO: Generalised object storage/lookup abstraction. */
enum aga_result aga_script_engine_lookup(
		struct aga_script_engine*, struct aga_script_class*, const char*);

void aga_script_engine_trace(void);

void* aga_script_pointer_new(void*);
void* aga_script_pointer_get(void*);

enum aga_result aga_script_engine_set_pointer(
		struct aga_script_engine*, const char*, void*);

enum aga_result aga_script_instance_new(
		struct aga_script_class*, struct aga_script_instance*);

enum aga_result aga_script_instance_delete(struct aga_script_instance*);

enum aga_result aga_script_instance_call(
		struct aga_script_engine*, struct aga_script_instance*, const char*);

#endif
