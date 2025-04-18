# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2024 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

SGI = vendor$(SEP)sgi-demos97$(SEP)
GLM = $(SGI)examples$(SEP)samples$(SEP)shadow$(SEP)
GLM_INCLUDE = $(GLM)

GLM_SRC = $(GLM)glm.c
GLM_HDR = $(GLM)glm.h

GLM_OBJ = $(subst .c,$(OBJ),$(GLM_SRC))

GLM_OUT = vendor$(SEP)$(LIB)glm$(A)

$(GLM_OBJ): $(GLM_HDR)

$(GLM_OUT): $(GLM_OBJ)
	$(STATIC)

clean_glm:
	$(RM) $(GLM_OBJ) $(GLM_OUT)
