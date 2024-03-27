
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FIRMATA_QMK_MAJOR_VERSION   0
#define FIRMATA_QMK_MINOR_VERSION   1

void firmata_sysex_handler(uint8_t cmd, uint8_t len, uint8_t *buf);

#define _PROCESS_CMD_SET(name)       void _process_cmd_set_##name     (uint8_t cmd, uint8_t len, uint8_t *buf)
#define _PROCESS_CMD_GET(name)       void _process_cmd_get_##name     (uint8_t cmd, uint8_t len, uint8_t *buf)
#define _PROCESS_CMD_SETGET(name)    _PROCESS_CMD_SET(name); _PROCESS_CMD_GET(name)

_PROCESS_CMD_SETGET(default_layer);
_PROCESS_CMD_SETGET(debug_mask);
_PROCESS_CMD_SETGET(macwin_mode);
_PROCESS_CMD_SETGET(battery_status);
_PROCESS_CMD_SETGET(rgb_matrix_buf);
_PROCESS_CMD_SETGET(rgb_matrix_mode);


enum {
    FRMT_CMD_RESPONSE = 0,
    FRMT_CMD_SET = 1,
    FRMT_CMD_GET = 2,
    FRMT_CMD_ADD = 3,
    FRMT_CMD_DEL = 4,
    FRMT_CMD_PUB = 5,
    FRMT_CMD_SUB = 6,

    FRMT_CMD_EXTENDED = 0xff
};

enum {
    FRMT_ID_RGB_MATRIX_BUF  = 1,
    FRMT_ID_DEFAULT_LAYER   = 2,
    FRMT_ID_DEBUG_MASK      = 3,
    FRMT_ID_BATTERY_STATUS  = 4,
    FRMT_ID_MACWIN_MODE     = 5,
    FRMT_ID_RGB_MATRIX_MODE = 6,
};


// rgb matrix buffer set from host
typedef struct rgb_matrix_host_buffer_t {
    struct {
        uint8_t duration;
        uint8_t r; // todo bb: store 4 bits, value diff around 15 not distinguishable
        uint8_t g;
        uint8_t b;
    } led[RGB_MATRIX_LED_COUNT];

    bool written;
} rgb_matrix_host_buffer_t;


typedef void (*sysexCallbackFunction)(uint8_t command, uint8_t argc, uint8_t *argv);

// Declare your wrapper functions here
void firmata_initialize(const char* firmware);
void firmata_begin(void);

void firmata_attach(uint8_t cmd, sysexCallbackFunction newFunction);

int firmata_recv(uint8_t c);
void firmata_send_sysex(uint8_t cmd, uint8_t* data, int len);

void firmata_process(void);

#ifdef __cplusplus
}
#endif

