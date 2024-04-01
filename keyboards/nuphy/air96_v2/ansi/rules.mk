SRC += \
side.c \
rf.c \
sleep.c \
# empty line

mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(dir $(mkfile_path))

include $(current_dir)/firmata/firmata.mk

QUANTUM_LIB_SRC += uart.c
