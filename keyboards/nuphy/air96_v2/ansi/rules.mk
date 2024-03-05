SRC += side.c
SRC += rf.c
SRC += sleep.c
SRC += firmata/FirmataParser.cpp
SRC += firmata/FirmataMarshaller.cpp
SRC += firmata/Firmata.cpp
SRC += firmata/Firmata_QMK.cpp
SRC += firmata/Print.cpp

QUANTUM_LIB_SRC += uart.c

CONSOLE_ENABLE = yes
VIRTSER_ENABLE = yes
