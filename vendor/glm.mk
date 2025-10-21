# SPDX-License-Identifier: X11
# Copyright (C) 2025 Emily "TTG" Banerjee <prs.ttg+aga@pm.me>

GLM = vendor$(SEP)glm$(SEP)
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
