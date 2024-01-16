/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agapack.h>
#include <agaio.h>
#include <agalog.h>
#include <agastd.h>

enum af_err aga_mkrespack(const char* path, struct aga_respack* pack) {
	AF_PARAM_CHK(path);
	AF_PARAM_CHK(pack);

	return AF_ERR_NONE;
}

enum af_err aga_killrespack(struct aga_respack* pack) {
	AF_PARAM_CHK(pack);

	return AF_ERR_NONE;
}

enum af_err aga_sweeprespack(struct aga_respack* pack) {
	AF_PARAM_CHK(pack);

	return AF_ERR_NONE;
}

enum af_err aga_mkres(
		struct aga_respack* pack, const char* path, struct aga_res** res) {

	AF_PARAM_CHK(path);
	AF_PARAM_CHK(pack);
	AF_PARAM_CHK(res);

	AF_VERIFY(*res = calloc(1, sizeof(struct aga_res)), AF_ERR_MEM);
	/* aga_log(__FILE__, "Loading resource `0x%p'...", *res); */

	(*res)->refcount = 1;

	return aga_mklargefile(path, &(*res)->data, &(*res)->size);
}

/* aga_log(__FILE__, "Killing resource `0x%p'...", *res); */
/*
static enum af_err aga_killres(struct aga_res* res) {
	AF_PARAM_CHK(res);

	AF_CHK(aga_killlargefile(res->data, res->size));

	free(res);

	return AF_ERR_NONE;
}
*/

enum af_err aga_acquireres(struct aga_res* res) {
	AF_PARAM_CHK(res);

	++res->refcount;

	return AF_ERR_NONE;
}

enum af_err aga_releaseres(struct aga_res* res) {
	AF_PARAM_CHK(res);

	if(res->refcount) --res->refcount;

	return AF_ERR_NONE;
}
