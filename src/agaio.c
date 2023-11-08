/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2023 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>
 */

#include <agacore.h>
#include <agaio.h>
#include <agastd.h>

#include <afeirsa/afeirsa.h>

enum af_err aga_read(const char* path, af_uchar_t** ptr, af_size_t* size) {
	FILE* f;
	long off;

	AF_PARAM_CHK(path);
	AF_PARAM_CHK(ptr);

	if(!(f = fopen(path, "r"))) {
		return aga_af_patherrno(__FILE__, "fopen", path);
	}
	if(fseek(f, 0, SEEK_END) == -1) return aga_af_errno(__FILE__, "fseek");
	if((off = ftell(f)) == -1) return aga_af_errno(__FILE__, "ftell");
	rewind(f);
	AF_VERIFY(*ptr = malloc((af_size_t) off), AF_ERR_MEM);
	if((*size = fread(*ptr, sizeof(af_uchar_t), off, f)) != (af_size_t) off) {
		if(ferror(f)) return aga_af_errno(__FILE__, "fread");
	}
	if(fclose(f) == EOF) return aga_af_errno(__FILE__, "fclose");

	return AF_ERR_NONE;
}

#ifdef AGA_HAVE_MAP
# include <unistd.h>
# include <sys/mman.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/fcntl.h>

enum af_err aga_fmap(const char* path, af_uchar_t** ptr, af_size_t* size) {
	struct stat statbuf;
	int fd;

	AF_PARAM_CHK(path);
	AF_PARAM_CHK(ptr);

	if((fd = open(path, O_RDONLY)) == -1) {
		return aga_af_patherrno(__FILE__, "open", path);
	}
	if(fstat(fd, &statbuf) == -1) {
		return aga_af_patherrno(__FILE__, "stat", path);
	}
	*size = statbuf.st_size;

	*ptr = mmap(0, *size, PROT_READ, MAP_PRIVATE, fd, 0);
	if(*ptr == (void*) -1) {
		return aga_af_patherrno(__FILE__, "mmap", path);
	}

	if(close(fd) == -1) {
		return aga_af_patherrno(__FILE__, "close", path);
	}

	return AF_ERR_NONE;
}

enum af_err aga_funmap(af_uchar_t* ptr, af_size_t size) {
	AF_PARAM_CHK(ptr);

	if(munmap(ptr, size) == -1) return aga_af_errno(__FILE__, "munmap");

	return AF_ERR_NONE;
}
#endif
