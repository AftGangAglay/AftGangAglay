/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agascript.h>
#include <agapyinc.h>

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

/*
 * NOTE: We need a bit of global state here to get engine system contexts etc.
 * 		 Into script land because this version of Python's state is spread
 * 		 Across every continent.
 */
aga_pyobject_t agan_dict = 0;

const typeobject aga_nativeptr_type = {
    OB_HEAD_INIT(&Typetype)
    0,
    "nativeptr", sizeof(struct aga_nativeptr), 0,
    aga_nativeptr_dealloc, 0, 0, 0, 0, 0, 0, 0, 0
};

static void aga_nativeptr_dealloc(aga_pyobject_t _) { (void) _; }

aga_pyobject_t newnativeptrobject(void) {
    return (aga_pyobject_t)
            NEWOBJ(struct aga_nativeptr, (typeobject*) &aga_nativeptr_type);
}

FILE* pyopen_r(const char* path) {
    return aga_open_r(path);
}

void pyclose(FILE* fp) {
    aga_close(fp);
}
