
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    SYSEX_RGB_MATRIX_CMD    = 1,
    SYSEX_DEFAULT_LAYER_SET = 2,
    SYSEX_DEBUG_MASK_SET = 3,

    SYSEX_CMD_EXTENDED = 0xff
};

typedef void (*sysexCallbackFunction)(uint8_t command, uint8_t argc, uint8_t *argv);

// Declare your wrapper functions here
void firmata_initialize(const char* firmware);
void firmata_begin(void);

void firmata_attach(uint8_t cmd, sysexCallbackFunction newFunction);

int firmata_recv(uint8_t c);
void firmata_send_command(int command, const unsigned char* data, int length);

void firmata_process(void);

#ifdef __cplusplus
}
#endif

