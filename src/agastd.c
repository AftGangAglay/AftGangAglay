/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agastd.h>
#include <agapack.h>
#include <agaerr.h>

FILE* aga_open_r(const char* path) {
    void* fp;
    af_size_t i;

    for(i = 0; i < aga_global_pack->len; ++i) {
        struct aga_res* res = &aga_global_pack->db[i];
        if(af_streql(path, res->conf->name)) {
            enum af_err result;

            result = aga_resseek(res, &fp);
            if(result) {
                aga_af_soft(__FILE__, "aga_resseek", result);
                return 0;
            }

            return fp;
        }
    }

    if(!(fp = fopen(path, "rb"))) aga_af_patherrno(__FILE__, "fopen", path);
    return fp;
}

void aga_close(FILE* fp) {
    if(!fp) return;
    if(fp == aga_global_pack->fp) return;

    if(fclose(fp) == EOF) aga_af_errno(__FILE__, "fclose");
}
