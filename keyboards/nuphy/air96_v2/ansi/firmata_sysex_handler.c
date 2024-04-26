#include "ansi.h"
#include "action_layer.h"
#include "rgb_matrix.h"
#include "debug.h"
#include "debug_user.h"

#include "firmata/Firmata_QMK.h"
#include "dynld_func.h"

//------------------------------------------------------------------------------

extern DEV_INFO_STRUCT dev_info;
extern bool f_macwin_ignore_switch;
extern bool f_sys_show;

extern void m_break_all_key(void);

//------------------------------------------------------------------------------


static void dprintf_buf(uint8_t *buf, uint8_t len) {
#ifdef CONSOLE_ENABLE
    if (debug_config.enable) {
    for (int i = 0; i < len; i++) {
        dprintf("%02x ", buf[i]);
        if (((i+1)%16) == 0) dprintf("\n");
    }
    dprintf("\n");
}
#endif
}

// adjusting the function pointer for thumb mode
uint32_t thumb_fun_addr(void* funptr) {
    uint8_t thumb_bit = 0;
#ifdef THUMB_PRESENT
    thumb_bit = 1;
#endif
    return (((uint32_t)funptr) | thumb_bit);
}

#define DYNLD_FUNC_SIZE 1024 // dynld function max size
static uint8_t dynld_func_buf[DYNLD_FUN_ID_MAX][DYNLD_FUNC_SIZE] __attribute__((aligned(4)));
dynld_funcs_t g_dynld_funcs = { 0 };

void load_function(const uint16_t fun_id, const uint8_t* data, size_t offset, size_t size) {
    // set function pointer after fully loaded
    if (offset == 0xffff) {
        if (memcmp(dynld_func_buf[fun_id], "\0\0", 2) != 0) {
            g_dynld_funcs.func[fun_id] = (void*)thumb_fun_addr((uint8_t*)dynld_func_buf[fun_id]);
            DBG_USR(firmata, "[FA]dynld fun[%d]:%p\n", (int)fun_id, g_dynld_funcs.func[fun_id]);
        }
        return;
    }
    if (offset + size > DYNLD_FUNC_SIZE) {
        memset((void*)&dynld_func_buf[fun_id][offset], 0, DYNLD_FUNC_SIZE);
        g_dynld_funcs.func[fun_id] = NULL;
        DBG_USR(firmata, "[FA]dynld fun too large\n");
        return;
    }
    if (fun_id >= DYNLD_FUN_ID_MAX) {
        DBG_USR(firmata, "[FA]dynld fun id too large\n");
        return;
    }
    if (offset == 0) {
        g_dynld_funcs.func[fun_id] = NULL;
        memset((void*)dynld_func_buf[fun_id], 0, DYNLD_FUNC_SIZE);
        if (size >= 2 && memcmp(data, "\0\0", 2) == 0) {
            DBG_USR(firmata, "[FA]dynld fun[%d]:0\n", (int)fun_id);
            return;
        }
    }
    memcpy((void*)&dynld_func_buf[fun_id][offset], data, size);
}

//------------------------------------------------------------------------------
rgb_matrix_host_buffer_t g_rgb_matrix_host_buf;

void virtser_recv(uint8_t c)
{
    if (firmata_recv(c) < 0) {
        dprintf("[E]firmata_recv\n");
    }
}

void firmata_sysex_handler(uint8_t cmd, uint8_t len, uint8_t *buf) {
    if (cmd == FRMT_CMD_SET) {
        uint8_t id = buf[0];
        buf++; len--;
        if (id == FRMT_ID_RGB_MATRIX_BUF)   _frmt_handle_cmd_set_rgb_matrix_buf(cmd, len, buf);
        if (id == FRMT_ID_DEFAULT_LAYER)    _frmt_handle_cmd_set_default_layer(cmd, len, buf);
        if (id == FRMT_ID_DEBUG_MASK)       _frmt_handle_cmd_set_debug_mask(cmd, len, buf);
        if (id == FRMT_ID_MACWIN_MODE)      _frmt_handle_cmd_set_macwin_mode(cmd, len, buf);
        if (id == FRMT_ID_RGB_MATRIX_MODE)  _frmt_handle_cmd_set_rgb_matrix_mode(cmd, len, buf);
        if (id == FRMT_ID_RGB_MATRIX_HSV)   _frmt_handle_cmd_set_rgb_matrix_hsv(cmd, len, buf);
        if (id == FRMT_ID_DYNLD_FUNCTION)   _frmt_handle_cmd_set_dynld_function(cmd, len, buf);
        if (id == FRMT_ID_DYNLD_FUNEXEC)    _frmt_handle_cmd_set_dynld_funexec(cmd, len, buf);
    }
    if (cmd == FRMT_CMD_GET) {
        uint8_t id = buf[0];
        buf++; len--;
        if (id == FRMT_ID_DEFAULT_LAYER)    _frmt_handle_cmd_get_default_layer(cmd, len, buf);
        if (id == FRMT_ID_DEBUG_MASK)       _frmt_handle_cmd_get_debug_mask(cmd, len, buf);
        if (id == FRMT_ID_MACWIN_MODE)      _frmt_handle_cmd_get_macwin_mode(cmd, len, buf);
        if (id == FRMT_ID_BATTERY_STATUS)   _frmt_handle_cmd_get_battery_status(cmd, len, buf);
        if (id == FRMT_ID_RGB_MATRIX_MODE)  _frmt_handle_cmd_get_rgb_matrix_mode(cmd, len, buf);
        if (id == FRMT_ID_RGB_MATRIX_HSV)   _frmt_handle_cmd_get_rgb_matrix_hsv(cmd, len, buf);
    }
}

//------------------------------------------------------------------------------
_FRMT_HANDLE_CMD_SET(rgb_matrix_buf) {
    for (uint16_t i = 0; i < len;) {
        //DBG_USR(firmata, "rgb:%d(%d):%d,%d,%d\n", (int)buf[i], (int)buf[i+1], (int)buf[i+2], (int)buf[i+3], (int)buf[i+4]);
        uint8_t li = buf[i++];
        if (li < RGB_MATRIX_LED_COUNT) {
            g_rgb_matrix_host_buf.led[li].duration = buf[i++];
            g_rgb_matrix_host_buf.led[li].r = buf[i++];
            g_rgb_matrix_host_buf.led[li].g = buf[i++];
            g_rgb_matrix_host_buf.led[li].b = buf[i++];

            g_rgb_matrix_host_buf.written = 1;
        }
        else
            break;
    }
}

//------------------------------------------------------------------------------
_FRMT_HANDLE_CMD_SET(default_layer) {
    DBG_USR(firmata, "[FA]layer:%d\n", (int)buf[0]);
    layer_state_t state = 1 << buf[0];
    default_layer_set(state);
}

_FRMT_HANDLE_CMD_GET(default_layer) {
}

//------------------------------------------------------------------------------
_FRMT_HANDLE_CMD_SET(debug_mask) {
    if (len < 4) {
        DBG_USR(firmata, "[FA]debug:%02x\n", buf[0]);
        debug_config.raw = buf[0];
        return;
    }
    if (len >= 4) {
        //uint32_t dbg_mask = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3];
        debug_config.raw = buf[3];
    }
    if (len >= 8) {
        const int off = 4;
        uint32_t dbg_user_mask = buf[off] << 24 | buf[off+1] << 16 | buf[off+2] << 8 | buf[off+3];
        debug_config_user.raw = dbg_user_mask;
        DBG_USR(firmata, "[FA]debug:%02x:%08lx\n", debug_config.raw, debug_config_user.raw);
    }
}

_FRMT_HANDLE_CMD_GET(debug_mask) {
    uint8_t response[9];
    response[0] = FRMT_ID_DEBUG_MASK;
    response[1] = response[2] = response[3] = 0;
    response[4] = debug_config.raw;
    response[5] = (debug_config_user.raw >> 24) & 0xff;
    response[6] = (debug_config_user.raw >> 16) & 0xff;
    response[7] = (debug_config_user.raw >> 8) & 0xff;
    response[8] = (debug_config_user.raw) & 0xff;
    firmata_send_sysex(FRMT_CMD_RESPONSE, response, sizeof(response));
}

//------------------------------------------------------------------------------
_FRMT_HANDLE_CMD_SET(macwin_mode) {
    DBG_USR(firmata, "[FA]macwin:%c\n", buf[0]);
    if ((buf[0] == 'm') && (dev_info.sys_sw_state != SYS_SW_MAC)) {
        f_macwin_ignore_switch = 1;
        f_sys_show = 1;
        default_layer_set(1 << 0);
        dev_info.sys_sw_state = SYS_SW_MAC;
        keymap_config.nkro    = 0;
        m_break_all_key();
    } else
    if ((buf[0] == 'w') && (dev_info.sys_sw_state != SYS_SW_WIN)) {
        f_macwin_ignore_switch = 1;
        f_sys_show = 1;
        default_layer_set(1 << 2);
        dev_info.sys_sw_state = SYS_SW_WIN;
        keymap_config.nkro    = 1;
        m_break_all_key();
    } else
    if (buf[0] == '-') {
        f_macwin_ignore_switch = 0;
    }
}

_FRMT_HANDLE_CMD_GET(macwin_mode) {
    uint8_t response[2];
    response[0] = FRMT_ID_MACWIN_MODE;

    response[1] = '-';
    if (dev_info.sys_sw_state == SYS_SW_MAC)
        response[1] = 'm';
    if (dev_info.sys_sw_state == SYS_SW_WIN)
        response[1] = 'w';

    firmata_send_sysex(FRMT_CMD_RESPONSE, response, sizeof(response));
}

//------------------------------------------------------------------------------
_FRMT_HANDLE_CMD_GET(battery_status) {
    uint8_t response[3];
    response[0] = FRMT_ID_BATTERY_STATUS;
    response[1] = dev_info.rf_charge;
    response[2] = dev_info.rf_baterry;
    firmata_send_sysex(FRMT_CMD_RESPONSE, response, sizeof(response));
}

//------------------------------------------------------------------------------
_FRMT_HANDLE_CMD_SET(rgb_matrix_mode) {
    uint8_t matrix_mode = buf[0];
    DBG_USR(firmata, "[FA]rgb:%d\n", (int)matrix_mode);
    if (matrix_mode) {
        rgb_matrix_enable_noeeprom();
        rgb_matrix_mode_noeeprom(matrix_mode);
    } else {
        rgb_matrix_disable_noeeprom();
    }
}

_FRMT_HANDLE_CMD_GET(rgb_matrix_mode) {
    uint8_t response[2];
    response[0] = FRMT_ID_RGB_MATRIX_MODE;
    response[1] = rgb_matrix_get_mode();

    firmata_send_sysex(FRMT_CMD_RESPONSE, response, sizeof(response));
}

//------------------------------------------------------------------------------
_FRMT_HANDLE_CMD_SET(rgb_matrix_hsv) {
    uint8_t h = buf[0];
    uint8_t s = buf[1];
    uint8_t v = buf[2];
    DBG_USR(firmata, "[FA]hsv:%d,%d,%d\n", (int)h, (int)s, (int)v);
    rgb_matrix_sethsv_noeeprom(h, s, v);
}

_FRMT_HANDLE_CMD_GET(rgb_matrix_hsv) {
    uint8_t response[4];
    HSV hsv = rgb_matrix_get_hsv();
    response[0] = FRMT_ID_RGB_MATRIX_HSV;
    response[1] = hsv.h;
    response[2] = hsv.s;
    response[3] = hsv.v;
    firmata_send_sysex(FRMT_CMD_RESPONSE, response, sizeof(response));
}

//------------------------------------------------------------------------------
static int dynld_env_printf(const char* fmt, ...) {
/*
    if (debug_config.enable) {
        xprintf(...);
}
*/
    return -1;
}

static dynld_test_env_t s_dynld_test_env = {
    .printf = dynld_env_printf
};

_FRMT_HANDLE_CMD_SET(dynld_function) {
    uint16_t fun_id = buf[0] | buf[1] << 8;
    uint16_t offset = buf[2] | buf[3] << 8;
    len -= 4;
    DBG_USR(firmata, "[FA]dynld id=%d,off=%d,len=%d\n", (int)fun_id, (int)offset, (int)len);
    load_function(fun_id, &buf[4], offset, len);
}

_FRMT_HANDLE_CMD_SET(dynld_funexec) {
    uint16_t fun_id = buf[0] | buf[1] << 8;
    len -= 2;
    DBG_USR(firmata, "[FA]dynld exec id=%d\n", (int)fun_id);

    if (fun_id == DYNLD_FUN_ID_TEST) {
        funptr_test_t fun_test = (funptr_test_t)g_dynld_funcs.func[DYNLD_FUN_ID_TEST];
        int ret = fun_test(&s_dynld_test_env); (void) ret;

        if (debug_config_user.firmata) {
            DBG_USR(firmata, "[FA]dynld exec ret=%d\n", ret);
        dprintf_buf(s_dynld_test_env.buf, 32);
        }
    }
}
