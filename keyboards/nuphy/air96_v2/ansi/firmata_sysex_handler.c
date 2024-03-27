#include "ansi.h"
#include "debug.h"
#include "firmata/Firmata_QMK.h"

//------------------------------------------------------------------------------

extern DEV_INFO_STRUCT dev_info;
extern bool f_macwin_ignore_switch;
extern bool f_sys_show;

extern void m_break_all_key(void);

//------------------------------------------------------------------------------

rgb_matrix_host_buffer_t g_rgb_matrix_host_buf;

void virtser_recv(uint8_t c)
{
#if VIRTSER_FIRMATA
    if (firmata_recv(c) < 0) {
        dprintf("[E] firmata_recv\n");
    }
#endif
}


void firmata_sysex_handler(uint8_t cmd, uint8_t len, uint8_t *buf)
{
    if (cmd == FRMT_CMD_SET) {
        uint8_t id = buf[0];
        buf++; len--;
        if (id == FRMT_ID_RGB_MATRIX_BUF)   _process_cmd_set_rgb_matrix_buf(cmd, len, buf);
        if (id == FRMT_ID_DEFAULT_LAYER)    _process_cmd_set_default_layer(cmd, len, buf);
        if (id == FRMT_ID_DEBUG_MASK)       _process_cmd_set_debug_mask(cmd, len, buf);
        if (id == FRMT_ID_MACWIN_MODE)      _process_cmd_set_macwin_mode(cmd, len, buf);
        if (id == FRMT_ID_RGB_MATRIX_MODE)  _process_cmd_set_rgb_matrix_mode(cmd, len, buf);
    }
    if (cmd == FRMT_CMD_GET) {
        uint8_t id = buf[0];
        buf++; len--;
        if (id == FRMT_ID_DEFAULT_LAYER)    _process_cmd_get_default_layer(cmd, len, buf);
        if (id == FRMT_ID_DEBUG_MASK)       _process_cmd_get_debug_mask(cmd, len, buf);
        if (id == FRMT_ID_MACWIN_MODE)      _process_cmd_get_macwin_mode(cmd, len, buf);
        if (id == FRMT_ID_BATTERY_STATUS)   _process_cmd_get_battery_status(cmd, len, buf);
        if (id == FRMT_ID_RGB_MATRIX_MODE)  _process_cmd_get_rgb_matrix_mode(cmd, len, buf);
    }
}


void _process_cmd_set_rgb_matrix_buf(uint8_t cmd, uint8_t len, uint8_t *buf)
{
    for (uint16_t i = 0; i < len;) {
        //dprintf("rgb:%d(%d):%d,%d,%d\n", buf[i], buf[i+1], buf[i+2], buf[i+3], buf[i+4]);
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

void _process_cmd_set_default_layer(uint8_t cmd, uint8_t len, uint8_t *buf)
{
    dprintf("[L]%d\n", buf[0]);
    layer_state_t state = 1 << buf[0];
    default_layer_set(state);
}

void _process_cmd_set_debug_mask(uint8_t cmd, uint8_t len, uint8_t *buf)
{
    debug_config = (debug_config_t) buf[0];
    dprintf("[D]%02x\n", buf[0]);
}

void _process_cmd_set_macwin_mode(uint8_t cmd, uint8_t len, uint8_t *buf)
{
    //dprintf("macwin %02x\n", buf[0]);
    if ((buf[0] == 'm') && (dev_info.sys_sw_state != SYS_SW_MAC)) {
        dprintf("mac\n");
        f_macwin_ignore_switch = 1;
        f_sys_show = 1;
        default_layer_set(1 << 0);
        dev_info.sys_sw_state = SYS_SW_MAC;
        keymap_config.nkro    = 0;
        m_break_all_key();
    } else
    if ((buf[0] == 'w') && (dev_info.sys_sw_state != SYS_SW_WIN)) {
        dprintf("win\n");
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

void _process_cmd_get_default_layer(uint8_t cmd, uint8_t len, uint8_t *buf)
{
}

void _process_cmd_get_debug_mask(uint8_t cmd, uint8_t len, uint8_t *buf)
{
    uint8_t response[2];
    response[0] = FRMT_ID_DEBUG_MASK;
    response[1] = *((uint8_t*)&debug_config);
    firmata_send_sysex(FRMT_CMD_RESPONSE, response, sizeof(response));
}

void _process_cmd_get_macwin_mode(uint8_t cmd, uint8_t len, uint8_t *buf)
{
    uint8_t response[2];
    response[0] = FRMT_ID_MACWIN_MODE;

    response[1] = '-';
    if (dev_info.sys_sw_state == SYS_SW_MAC)
        response[1] = 'm';
    if (dev_info.sys_sw_state == SYS_SW_WIN)
        response[1] = 'w';

    firmata_send_sysex(FRMT_CMD_RESPONSE, response, sizeof(response));
}

void _process_cmd_get_battery_status(uint8_t cmd, uint8_t len, uint8_t *buf)
{
    uint8_t response[3];
    response[0] = FRMT_ID_BATTERY_STATUS;
    response[1] = dev_info.rf_charge;
    response[2] = dev_info.rf_baterry;
    firmata_send_sysex(FRMT_CMD_RESPONSE, response, sizeof(response));
}

_PROCESS_CMD_SET(rgb_matrix_mode)
{
    uint8_t matrix_mode = buf[0];
    dprintf("[RGB]mode=%d\n", matrix_mode);
    if (matrix_mode) {
        rgb_matrix_enable_noeeprom();
        rgb_matrix_mode_noeeprom(matrix_mode);
    } else {
        rgb_matrix_disable_noeeprom();
    }
}

_PROCESS_CMD_GET(rgb_matrix_mode)
{
    uint8_t response[2];
    response[0] = FRMT_ID_RGB_MATRIX_MODE;
    response[1] = rgb_matrix_get_mode();

    firmata_send_sysex(FRMT_CMD_RESPONSE, response, sizeof(response));
}
