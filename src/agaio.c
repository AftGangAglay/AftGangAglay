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

	if(!(f = fopen(path, "r"))) aga_errno_chk("fopen");
	if(fseek(f, 0, SEEK_END) == -1) aga_errno_chk("fseek");
	if((off = ftell(f)) == -1) aga_errno_chk("ftell");
	rewind(f);
	AF_VERIFY(*ptr = malloc((af_size_t) off), AF_ERR_MEM);
	if((*size = fread(*ptr, sizeof(af_uchar_t), off, f)) != (af_size_t) off) {
		if(ferror(f)) aga_errno_chk("fread");
	}
	if(fclose(f) == EOF) aga_errno_chk("fclose");

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

	if((fd = open(path, O_RDONLY)) == -1) aga_errno_chk("open");
	if(fstat(fd, &statbuf) == -1) aga_errno_chk("fstat");
	*size = statbuf.st_size;

	*ptr = mmap(0, *size, PROT_READ, MAP_PRIVATE, fd, 0);
	if(*ptr == (void*) -1) aga_errno_chk("mmap");

	if(close(fd) == -1) aga_errno_chk("close");

	return AF_ERR_NONE;
}

enum af_err aga_funmap(af_uchar_t* ptr, af_size_t size) {
	AF_PARAM_CHK(ptr);

	if(munmap(ptr, size) == -1) aga_errno_chk("munmap");

	return AF_ERR_NONE;
}
#endif
