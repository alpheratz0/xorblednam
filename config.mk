# Copyright (C) 2022 <alpheratz99@protonmail.com>
# This program is free software.

VERSION   = 0.1.4

CC        = cc
CFLAGS    = -std=c99 -pedantic -Wall -Wextra -Os -DVERSION=\"$(VERSION)\"
LDLIBS    = -lpng -lm
LDFLAGS   = -s

PREFIX    = /usr/local
MANPREFIX = $(PREFIX)/share/man
