# Copyright (C) 2022-2023 <alpheratz99@protonmail.com>
# This program is free software.

VERSION = 0.1.5

PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

PKG_CONFIG = pkg-config

DEPENDENCIES = libpng

INCS = $(shell $(PKG_CONFIG) --cflags $(DEPENDENCIES)) -Iinclude
LIBS = $(shell $(PKG_CONFIG) --libs $(DEPENDENCIES)) -lm

CFLAGS = -std=c99 -pedantic -Wall -Wextra -Os $(INCS) -DVERSION=\"$(VERSION)\"
LDFLAGS = -s $(LIBS)

CC = cc
