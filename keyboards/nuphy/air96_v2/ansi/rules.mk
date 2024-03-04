SRC += side.c
SRC += rf.c
SRC += sleep.c
SRC += firmata/FirmataParser.cpp
SRC += firmata/FirmataMarshaller.cpp
SRC += firmata/Firmata.cpp
SRC += firmata/Firmata_QMK.cpp
SRC += firmata/Print.cpp

QUANTUM_LIB_SRC += uart.c

KEYBOARD_SHARED_EP = yes
SHARED_EP_ENABLE = yes
RAW_ENABLE = no
MOUSE_ENABLE = no
MIDI_ENABLE = no
JOYSTICK_ENABLE = no
DIGITIZER_ENABLE = no

CONSOLE_ENABLE = yes
VIRTSER_ENABLE = yes
