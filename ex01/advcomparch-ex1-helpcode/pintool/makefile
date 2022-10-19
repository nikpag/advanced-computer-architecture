# If the tool is built out of the kit, PIN_ROOT must be specified in the make invocation and point to the kit root.
PIN_ROOT ?= /home/nick/arch-ntua/ex01/pin-3.22-98547-g7a303a835-gcc-linux
ifdef PIN_ROOT
CONFIG_ROOT := $(PIN_ROOT)/source/tools/Config
else
CONFIG_ROOT := ../Config
endif
include $(CONFIG_ROOT)/makefile.config
include makefile.rules
include $(TOOLS_ROOT)/Config/makefile.default.rules
