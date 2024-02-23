/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agascript.h>
#include <agastd.h>
#include <agascripthelp.h>

#include <agan/agan.h>

/*
 * Defines our nativeptr type and a few misc. functions Python wants.
 * Also contains a few scriptglue helpers declared in `agapyinc'.
 */

/* NOTE: This is cursed beyond cursed but old C code do be like that. */
#define main fake_main
#include <pythonmain.c>
#undef main
#include <config.c>

#ifdef _DEBUG
/* The extra debug info this enables is a bit too verbose. */
int debugging = 0;
#endif

static void agan_killnativeptr(aga_pyobject_t obj) { free(obj); }

const typeobject agan_nativeptr_type = {
    OB_HEAD_INIT(&Typetype)
    0,
    "nativeptr", sizeof(struct agan_nativeptr), 0,
    agan_killnativeptr, 0, 0, 0, 0, 0, 0, 0, 0
};

aga_pyobject_t agan_mknativeptr(void* ptr) {
    aga_pyobject_t o = newobject((void*) &agan_nativeptr_type);
	((struct agan_nativeptr*) o)->ptr = ptr;
	return o;
}

FILE* pyopen_r(const char* path) {
    return aga_open_r(path);
}

void pyclose(FILE* fp) {
    aga_close(fp);
}

aga_bool_t aga_script_float(aga_pyobject_t o, float* f) {
	*f = (float) getfloatvalue(o);
	return err_occurred();
}

aga_bool_t aga_script_int(aga_pyobject_t o, int* i) {
	*i = getintvalue(o);
	return err_occurred();
}

aga_bool_t aga_script_string(aga_pyobject_t o, char** s) {
	*s = getstringvalue(o);
	return err_occurred();
}

aga_bool_t aga_script_bool(aga_pyobject_t o, aga_bool_t* b) {
	*b = !!getintvalue(o);
	return err_occurred();
}

aga_bool_t aga_list_set(aga_pyobject_t list, aga_size_t n, aga_pyobject_t v) {
	return setlistitem(list, (int) n, v) == -1;
}

aga_bool_t aga_list_get(aga_pyobject_t list, aga_size_t n, aga_pyobject_t* v) {
	return !(*v = getlistitem(list, (int) n));
}
