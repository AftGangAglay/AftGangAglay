PYTHON_ROOT = vendor/python
PYTHON_SOURCEROOT = $(PYTHON_ROOT)/src

PYTHON_IFLAGS = -isystem $(PYTHON_SOURCEROOT)

LIBPYTHON_SOURCES = acceler.c bltinmodule.c ceval.c classobject.c \
               compile.c dictobject.c errors.c fgetsintr.c \
               fileobject.c floatobject.c frameobject.c \
               funcobject.c graminit.c grammar1.c import.c \
               intobject.c intrcheck.c listnode.c listobject.c \
               mathmodule.c methodobject.c modsupport.c \
               moduleobject.c node.c object.c parser.c \
               parsetok.c posixmodule.c regexp.c regexpmodule.c \
               strdup.c \
               stringobject.c structmember.c \
               sysmodule.c timemodule.c tokenizer.c traceback.c \
               tupleobject.c typeobject.c
LIBPYTHON_SOURCES := $(addprefix $(PYTHON_SOURCEROOT)/,$(LIBPYTHON_SOURCES))
LIBPYTHON_OBJECTS = $(LIBPYTHON_SOURCES:.c=.o)

LIBPYTHON = vendor/libpython.a

LIBPYTHON_CFLAGS = -w -DSYSV
LIBPYTHON_CFLAGS += -Wno-incompatible-function-pointer-types
ifdef WINDOWS
	LIBPYTHON_CFLAGS += -D_WINDOWS -DNO_LSTAT
endif

PYTHONGEN_SOURCES = acceler.c fgetsintr.c grammar1.c \
               intrcheck.c listnode.c node.c parser.c \
               parsetok.c strdup.c tokenizer.c bitset.c \
               firstsets.c grammar.c metagrammar.c pgen.c \
               pgenmain.c printgrammar.c
PYTHONGEN_SOURCES := $(addprefix $(PYTHON_SOURCEROOT)/,$(PYTHONGEN_SOURCES))
PYTHONGEN_OBJECTS = $(PYTHONGEN_SOURCES:.c=.o)

PYTHONGEN = $(PYTHON_SOURCEROOT)/pgenmain

$(LIBPYTHON): CFLAGS += $(LIBPYTHON_CFLAGS)
$(LIBPYTHON): LDFLAGS = -lm
$(LIBPYTHON): $(LIBPYTHON_OBJECTS)

$(LIBPYTHON_OBJECTS): $(LIBPYTHON_SOURCES)

$(PYTHONGEN): LDFLAGS = -lm
$(PYTHONGEN): $(PYTHONGEN_OBJECTS)

$(PYTHONGEN_OBJECTS): $(PYTHONGEN_SOURCES)

graminit.c graminit.h: $(PYTHON_SOURCEROOT)/Grammar $(PYTHONGEN)
	-$(CROSS_TOOL) $(PYTHONGEN) Grammar

all: $(LIBPYTHON)

.PHONY: clean_python
clean_python:
	rm -f $(LIBPYTHON)
	rm -f $(PYTHONGEN)
	rm -f $(PYTHONGEN_OBJECTS)
	rm -f $(LIBPYTHON_OBJECTS)	
