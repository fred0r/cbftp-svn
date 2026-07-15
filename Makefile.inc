# uncomment and edit this line to link with a static openssl
#STATIC_SSL_PATH := /path/to/openssl

# relative path to the ui to use
UI_PATH := src/ui

# path where the data file will be stored
DATA_FILE := "\"~/.cbftp/data\""

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
  ifndef STATIC_SSL_PATH
    HOMEBREW_SSL_PATH := $(shell find /opt/homebrew/Cellar -name libssl.a | xargs dirname | xargs dirname | tail -n 1)
    ifneq ($(HOMEBREW_SSL_PATH),)
      STATIC_SSL_PATH := $(HOMEBREW_SSL_PATH)
      STATIC_SSL_LINK := $(STATIC_SSL_PATH)/lib/libssl.a $(STATIC_SSL_PATH)/lib/libcrypto.a
    else
      $(error Could not find any OpenSSL installed in /opt/homebrew/Cellar. Please specify your path to OpenSSL by editing STATIC_SSL_PATH in Makefile.inc)
    endif
  endif
endif

ifdef STATIC_SSL_PATH
  STATIC_SSL_INCLUDE := -I$(STATIC_SSL_PATH)/include
  ifndef STATIC_SSL_LINK
    STATIC_SSL_LINK := $(STATIC_SSL_PATH)/libssl.a $(STATIC_SSL_PATH)/libcrypto.a
  endif
  SSL_LINKFLAGS := $(STATIC_SSL_LINK) -ldl
else
  SSL_LINKFLAGS := -lssl -lcrypto
endif

ROOT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

HAS_SVN := $(shell svn info $(ROOT_DIR) > /dev/null 2>&1; echo $$?)
REDIST_VERSION := $(shell cat $(ROOT_DIR)/.version 2>/dev/null)
GIT_HASH := $(shell git -C $(ROOT_DIR) rev-parse --short HEAD 2>/dev/null || echo "nogit")

ifeq ($(HAS_SVN), 0)
VERSION := "\"r$(shell svn info $(ROOT_DIR)|grep Revision|awk '{ print $$2 }')-franken-$(GIT_HASH)\""
SVN_ROOT_DIR := $(shell svn info $(ROOT_DIR)|grep 'Working Copy Root'|awk '{print $$5 }')
else ifneq ($(REDIST_VERSION),)
VERSION := "\"r$(REDIST_VERSION)-franken-$(GIT_HASH)\""
else
VERSION := "\"unknown-franken-$(GIT_HASH)\""
endif

DATE := "\"$(shell date)\""

BOXTAG := "\"cbftp\""

CXX ?= g++
ifndef OPTFLAGS
OPTFLAGS = -g -O0
else
$(info Using OPTFLAGS provided by shell)
endif
ifndef CXXFLAGS
CXXFLAGS = $(OPTFLAGS) -std=c++11 -Wall -MMD -MP
ifdef STATIC_SSL_INCLUDE
CXXFLAGS += $(STATIC_SSL_INCLUDE)
endif
else
$(info Using CXXFLAGS provided by shell)
endif
ifndef CFLAGS
CFLAGS = $(OPTFLAGS) -Wall -MMD -MP
AR ?= ar
RANLIB ?= ranlib
else
$(info Using CFLAGS provided by shell)
endif
ifndef LINKFLAGS
  LINKFLAGS=$(SSL_LINKFLAGS) -lpthread
  ifneq ($(wildcard /etc/debian_version),)
    DEBIAN=1
  endif
  ifneq ($(filter $(UNAME_S),FreeBSD),)
    FREEBSD=1
    AR = llvm-ar
    RANLIB = llvm-ranlib
  endif
  ifneq ($(DEBIAN)$(FREEBSD),)
    LINKFLAGS+=-lncursesw
  else
    NCURSES_CFLAGS := $(shell pkg-config --cflags ncurses 2>/dev/null)
    NCURSES_LIBS := $(shell pkg-config --libs ncurses 2>/dev/null)
    ifeq ($(NCURSES_CFLAGS),)
      ifneq ($(wildcard /usr/pkg/include/ncurses/ncurses.h),)
        NCURSES_CFLAGS := -I/usr/pkg/include -DNCURSES_SUBDIR
        NCURSES_LIBS := -L/usr/pkg/lib -Wl,-rpath,/usr/pkg/lib
      else ifneq ($(wildcard /usr/pkg/include/ncurses.h),)
        NCURSES_CFLAGS := -I/usr/pkg/include
        NCURSES_LIBS := -L/usr/pkg/lib -Wl,-rpath,/usr/pkg/lib
      else ifneq ($(wildcard /usr/local/include/ncurses/ncurses.h),)
        NCURSES_CFLAGS := -I/usr/local/include -DNCURSES_SUBDIR
        NCURSES_LIBS := -L/usr/local/lib -Wl,-rpath,/usr/local/lib
      else ifneq ($(wildcard /usr/local/include/ncurses.h),)
        NCURSES_CFLAGS := -I/usr/local/include
        NCURSES_LIBS := -L/usr/local/lib -Wl,-rpath,/usr/local/lib
      endif
    endif
    CXXFLAGS += $(NCURSES_CFLAGS)
    ifneq ($(NCURSES_LIBS),)
      LINKFLAGS += $(NCURSES_LIBS)
      ifeq ($(findstring -l,$(NCURSES_LIBS)),)
        LINKFLAGS += -lncurses
      endif
    else
      LINKFLAGS += -lncurses
    endif
  endif
else
  $(info Using LINKFLAGS provided by shell)
endif
