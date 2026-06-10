# Minimal build stub for C_mc_mt_sitar integration
# This does not run any build by itself. Adjust paths to your Sitar toolchain.

SITAR_ROOT ?= ../march_v11
SITAR_INC  ?= $(SITAR_ROOT)

CXX ?= g++
CXXFLAGS ?= -O2 -std=c++20 -I$(SITAR_INC) -IAjitPublicResources/processor/64bit/C_mc_mt_sitar/shim -IAjitPublicResources/processor/64bit/C_mc_mt_sitar/memory

SRCS = \
  AjitPublicResources/processor/64bit/C_mc_mt_sitar/shim/ajit_memory_shim.cpp \
  AjitPublicResources/processor/64bit/C_mc_mt_sitar/shim/sitar_port_wrapper.cpp \
  AjitPublicResources/processor/64bit/C_mc_mt_sitar/memory/memory.c

OBJS = $(SRCS:.cpp=.o)
OBJS := $(OBJS:.c=.o)

.PHONY: all
all:
	@echo "This is a build stub. Integrate with your Sitar build system."

