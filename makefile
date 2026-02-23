HOST ?= m68k-amigaos

CC     = $(HOST)-gcc
AR     = $(HOST)-ar
RANLIB = $(HOST)-ranlib

OPTIMIZE = -Os -fno-common
INCLUDES = -I./include
WARNINGS = -Wall -Wwrite-strings -Werror
CFLAGS = $(OPTIMIZE) $(INCLUDES) $(WARNINGS)

SRCS = $(addprefix src/, \
       changefileposition64.c changefilesize64.c getfileposition64.c getfilesize64.c \
       changefileposition.c changefilesize.c getfileposition.c getfilesize.c \
       dopkt64.c)

ifneq (,$(SYSROOT))
	CFLAGS := --sysroot=$(SYSROOT) $(CFLAGS)
endif

ifeq ($(HOST),m68k-amigaos)
	CPU = 68000
	CFLAGS := -noixemul -fomit-frame-pointer $(CFLAGS)
else
	CPU = $(patsubst %-aros,%,$(HOST))
endif

OBJS = $(subst src/,obj/$(CPU)/,$(SRCS:.c=.o))

.PHONY: all
all: bin/$(CPU)/libdos64.a

bin/$(CPU)/libdos64.a: $(OBJS)
	@mkdir -p $(dir $@)
	$(AR) -crv $@ $^

obj/$(CPU)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -rf bin obj

