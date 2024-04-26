SRC += \
side.c \
rf.c \
sleep.c \
debug_user.c \
# empty line

QUANTUM_LIB_SRC += uart.c

mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
current_dir := $(dir $(mkfile_path))

include $(current_dir)/firmata/firmata.mk
