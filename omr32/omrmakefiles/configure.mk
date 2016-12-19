###############################################################################
#
# (c) Copyright IBM Corp. 2015, 2016
#
#  This program and the accompanying materials are made available
#  under the terms of the Eclipse Public License v1.0 and
#  Apache License v2.0 which accompanies this distribution.
#
#      The Eclipse Public License is available at
#      http://www.eclipse.org/legal/epl-v10.html
#
#      The Apache License v2.0 is available at
#      http://www.opensource.org/licenses/apache2.0.php
#
# Contributors:
#    Multiple authors (IBM Corp.) - initial implementation and documentation
###############################################################################

###
### omrmakefiles/configure.mk.  Generated from configure.mk.in by configure.
###

###
### Ensure the default target is "all"
###

all:
.PHONY: all

###
### OMR Build Flags
###

include $(top_srcdir)/omrmakefiles/omrcfg.mk

###
### Global Autoconfigure Flags
###

# If OMRGLUE contains relative paths, assume they are relative to the
# top of the build tree.
# We can't convert OMRGLUE to absolute paths because, for z/OS, Windows, and AIX,
# the J9 build runs configure and compiles in different directory structures.
#OMRGLUE ?= $(filter /%,./example/glue)$(addprefix $(top_srcdir)/,$(filter-out /%,./example/glue))
#zg. Trying to using the glue in some++.Can not use ../src/glue directly. Don't know why???TODOTODOTODO
OMRGLUE ?= $(filter /%,../src/glue)$(addprefix $(top_srcdir)/,$(filter-out /%,../src/glue))
#OMRGLUE := ../src/glue
#OMRGLUE_INCLUDE+=$(top_srcdir)/../src/vmobjects
#OMRGLUE ?= $(filter /%,../src/glue) $(addprefix $(top_srcdir)/,$(filter-out /%,../src/glue))


OMR_CROSS_COMPILE := 0

OMR_HOST_OS := linux
OMR_HOST_ARCH := x86
OMR_TARGET_DATASIZE := 32
OMR_TOOLCHAIN := gcc

lib_output_dir := $(top_srcdir)/lib
exe_output_dir := $(top_srcdir)

CPPFLAGS := 
CFLAGS := 
CXXFLAGS := 
LDFLAGS := 

OMRGLUE_CPPFLAGS := 
OMRGLUE_CFLAGS := 
OMRGLUE_CXXFLAGS := 
OMRGLUE_INCLUDES := $(OMRGLUE) $(top_srcdir)/../src/vmobjects $(filter /%,) $(addprefix $(top_srcdir)/,$(filter-out /%,))

# GNU make provides a default value for ARFLAGS. We want to override it with our own value.
ARFLAGS = 

CC := gcc
CXX := g++
CCLINK := $(CC)
CCLINKSHARED := $(CCLINK)
CCLINKEXE := $(CCLINK)
CXXLINK := $(CXX)
CXXLINKSHARED := $(CXXLINK)
CXXLINKEXE := $(CXXLINK)

AS := as
AR := ar
OBJCOPY := objcopy

LIBPREFIX := lib
EXEEXT := 
SOLIBEXT := .so
ARLIBEXT := .a
OBJEXT := .o


###
### Platform Flags
###

# Need to allow deferred expansion of automatic variables like $*
GLOBAL_CPPFLAGS = 
GLOBAL_CFLAGS = 
GLOBAL_CXXFLAGS = 
GLOBAL_INCLUDES = 
GLOBAL_LDFLAGS = 
GLOBAL_STATIC_LIBS = 
GLOBAL_SHARED_LIBS = 
GLOBAL_LIBPATH = 
GLOBAL_ARFLAGS = 

#OMR_OPTIMIZE := 1
OMR_OPTIMIZE := 0
OMR_WARNINGS_AS_ERRORS := 1
OMR_ENHANCED_WARNINGS := 1
OMR_DEBUG := 1

ENABLE_FVTEST := no
ENABLE_FVTEST_AGENT := no
ENABLE_TRACEGEN := yes

ENABLE_DDR := no

###
### Global Options
###

## Add Global Flags
# Header files in the current directory should have first precedence.
MODULE_INCLUDES += .
GLOBAL_INCLUDES += $(top_srcdir)/include_core $(top_srcdir)/nls

GLOBAL_CPPFLAGS += -DUT_DIRECT_TRACE_REGISTRATION

# By default, shared libs are built into exe_output_dir, and
# static libs are built into lib_output_dir.
GLOBAL_LIBPATH += . $(exe_output_dir) $(lib_output_dir)

## Special settings for fvtests
# Location of fvtest framework
OMR_GTEST_DIR := $(top_srcdir)/omr_test/fvtest/gtest-1.7.0
OMR_GTEST_INCLUDES := $(OMR_GTEST_DIR) $(OMR_GTEST_DIR)/include $(top_srcdir)/fvtest/omrGtestGlue
OMR_PUGIXML_DIR := $(top_srcdir)/third_party/pugixml-1.5

# googletest code requires exception handling
ifeq (linux,$(OMR_HOST_OS))
  ifeq (xlc,$(OMR_TOOLCHAIN))
    OMR_GTEST_CXXFLAGS += -qeh
  else
    OMR_GTEST_CXXFLAGS += -fexceptions
  endif
endif
ifeq (msvc,$(OMR_TOOLCHAIN))
  OMR_GTEST_CXXFLAGS += /EHsc
endif

# Public Core OMR header files
OMR_IPATH = \
  $(top_srcdir)/include_core \
  $(top_srcdir)/omr/startup \
  $(top_srcdir)/gc/include \
  $(top_srcdir)/gc/startup \

# Internal OMR header files needed by OMR glue code
OMRGC_IPATH = \
  $(top_srcdir)/gc/base \
  $(top_srcdir)/gc/base/standard \
  $(top_srcdir)/gc/verbose \
  $(top_srcdir)/gc/verbose/handler_standard \
  $(top_srcdir)/gc/stats \
  $(top_srcdir)/gc/structs

ifeq (1,$(OMR_GC_SEGREGATED_HEAP))
OMRGC_IPATH += $(top_srcdir)/gc/base/segregated
endif

ifeq (yes,$(ENABLE_DDR))
# libdwarf location
OMR_LIBDWARF_IPATH = 
OMR_LIBDWARF_STATIC_LIBS = dwarf elf
OMR_LIBDWARF_SHARED_LIBS = 
endif
