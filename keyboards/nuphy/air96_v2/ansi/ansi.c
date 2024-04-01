/*
Copyright 2023 @ Nuphy <https://nuphy.com/>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ansi.h"
#include "usb_main.h"

#ifdef FIRMATA_ENABLE
#include "firmata/Firmata_QMK.h"
#endif

#define RF_LONG_PRESS_DELAY   30
#define DEV_RESET_PRESS_DELAY 30
#define RGB_TEST_PRESS_DELAY  30

user_config_t user_config;
DEV_INFO_STRUCT dev_info =
{
    .rf_baterry = 100,
    .link_mode  = LINK_USB,
    .rf_state   = RF_IDLE,
};

bool f_uart_ack         = 0;
bool f_bat_show         = 0;
bool f_bat_hold         = 0;
bool f_dev_sleep_enable = 1;
bool f_chg_show         = 1;
bool f_sys_show         = 0;
bool f_sleep_show       = 0;
bool f_func_save        = 0;
bool f_rf_read_data_ok  = 0;
bool f_rf_sts_sysc_ok   = 0;
bool f_rf_new_adv_ok    = 0;
bool f_rf_reset         = 0;
bool f_send_channel     = 0;
bool f_rf_hand_ok       = 0;
bool f_dial_sw_init_ok  = 0;
bool f_goto_sleep       = 0;
bool f_wakeup_prepare   = 0;
bool f_rf_sw_press      = 0;
bool f_dev_reset_press  = 0;
bool f_rgb_test_press   = 0;
bool f_bat_num_show     = 0;
bool f_keyb_indicator_config  = 0;
bool f_macwin_ignore_switch = 0;


uint8_t host_mode;
host_driver_t *m_host_driver   = 0;
uint16_t rf_linking_time       = 0;
uint16_t rf_link_show_time     = 0;
uint8_t rf_blink_cnt           = 0;
uint16_t no_act_time           = 0;
uint16_t dev_reset_press_delay = 0;
uint16_t rf_sw_press_delay     = 0;
uint16_t rgb_test_press_delay  = 0;
uint8_t rf_sw_temp             = 0;


/* struct to keep indicator rgb input from user */
typedef struct keyb_indicator_config_input_t
{
    uint8_t input_state;

    uint8_t indicator; // caps/num/... lock
    uint8_t indicator_select; // select led for indicator

    uint8_t rgb;
    uint8_t rgb_index; // 0=r,1=g,2=b

    uint16_t press_delay; // press delay of "keyboard indicator config" custom key
} keyb_indicator_config_input_t;

enum {
    INPUT_BEGIN = 0,
    INPUT_CAPSLOCK_SELECT,
    INPUT_NUMLOCK_SELECT,
    INPUT_RGB,
    INPUT_END
};

enum {
    INDICATOR_NONE = 0,
    INDICATOR_CAPSLOCK,
    INDICATOR_NUMLOCK,
};

static keyb_indicator_config_input_t indicator_config_input = {0};

keyb_indicator_config_t keyb_indicator_config[MAX_KEYB_INDICATORS] = {
    { .r = 0, .g = 128, .b = 128, .indicator_select = 1, }, // caps lock
    { .r = 0, .g = 128, .b = 128, .indicator_select = 2, }  // num lock
};


void rf_uart_init(void);
void rf_device_init(void);
void m_side_led_show(keyb_indicator_config_t keyb_inds_config[MAX_KEYB_INDICATORS]);
void dev_sts_sync(void);
void uart_send_report_func(void);
void uart_receive_pro(void);
void Sleep_Handle(void);
uint8_t uart_send_cmd(uint8_t cmd, uint8_t ack_cnt, uint8_t delayms);
void uart_send_report(uint8_t report_type, uint8_t *report_buf, uint8_t report_size);

void device_reset_show(void);
void device_reset_init(void);
void rgb_test_show(void);

extern uint8_t side_mode;
extern uint8_t side_light;
extern uint8_t side_speed;
extern uint8_t side_rgb;
extern uint8_t side_colour;
extern report_keyboard_t *keyboard_report;
extern uint8_t uart_bit_report_buf[32];
extern uint8_t bitkb_report_buf[32];
extern uint8_t bytekb_report_buf[8];

extern void eeconfig_read_user_datablock(void *data);
extern void eeconfig_update_user_datablock(const void *data);
extern void light_speed_contol(uint8_t fast);
extern void light_level_control(uint8_t brighten);
extern void side_colour_control(uint8_t dir);
extern void side_mode_control(uint8_t dir);
extern void num_led_show(void);




/**
 * @brief  gpio initial.
 */
void m_gpio_init(void)
{
    setPinOutput(DC_BOOST_PIN); writePinHigh(DC_BOOST_PIN);

    setPinOutput(RGB_DRIVER_SDB1); writePinHigh(RGB_DRIVER_SDB1);
    setPinOutput(RGB_DRIVER_SDB2); writePinHigh(RGB_DRIVER_SDB2);

    setPinOutput(NRF_WAKEUP_PIN);
    writePinHigh(NRF_WAKEUP_PIN);

    setPinInputHigh(NRF_BOOT_PIN);

    setPinOutput(NRF_RESET_PIN); writePinLow(NRF_RESET_PIN);
    wait_ms(50);
    writePinHigh(NRF_RESET_PIN);

    setPinInputHigh(DEV_MODE_PIN);
    setPinInputHigh(SYS_MODE_PIN);
}

/**
 * @brief  long press key process.
 */
void long_press_key(void)
{
    static uint32_t long_press_timer = 0;

    if (timer_elapsed32(long_press_timer) < 100) return;
    long_press_timer = timer_read32();

    if (f_rf_sw_press) {
        rf_sw_press_delay++;
        if (rf_sw_press_delay >= RF_LONG_PRESS_DELAY)
        {
            f_rf_sw_press = 0;
            dev_info.link_mode   = rf_sw_temp;
            dev_info.rf_channel  = rf_sw_temp;
            dev_info.ble_channel = rf_sw_temp;

            uint8_t timeout = 5;
            while (timeout--) {
                uart_send_cmd(CMD_NEW_ADV, 0, 1);
                wait_ms(20);
                uart_receive_pro();
                if (f_rf_new_adv_ok) break;
            }
        }
    } else {
        rf_sw_press_delay = 0;
    }

    if (f_dev_reset_press) {
        dev_reset_press_delay++;
        if (dev_reset_press_delay >= DEV_RESET_PRESS_DELAY)  {
            f_dev_reset_press = 0;

            if (dev_info.link_mode != LINK_USB) {
                if (dev_info.link_mode != LINK_RF_24) {
                    dev_info.link_mode   = LINK_BT_1;
                    dev_info.ble_channel = LINK_BT_1;
                    dev_info.rf_channel  = LINK_BT_1;
                }
            } else {
                dev_info.ble_channel = LINK_BT_1;
                dev_info.rf_channel  = LINK_BT_1;
            }

            uart_send_cmd(CMD_SET_LINK, 10, 10);
            wait_ms(500);
            uart_send_cmd(CMD_CLR_DEVICE, 10, 10);

            eeconfig_init();
            device_reset_show();
            device_reset_init();

            if (dev_info.sys_sw_state == SYS_SW_MAC) {
                default_layer_set(1 << 0);
            } else {
                default_layer_set(1 << 2);
            }
        }
    } else {
        dev_reset_press_delay = 0;
    }

    if (f_rgb_test_press) {
        rgb_test_press_delay++;
        if (rgb_test_press_delay >= RGB_TEST_PRESS_DELAY) {
            f_rgb_test_press = 0;
            rgb_test_show();
        }
    } else {
        rgb_test_press_delay = 0;
    }

    if (f_keyb_indicator_config) {
        indicator_config_input.press_delay++;
        if (indicator_config_input.press_delay >= 300) {
            f_keyb_indicator_config = 0;
            indicator_config_input.press_delay = 0;
        }
    }
}

/**
 * @brief  Release all keys, clear keyboard report.
 */
void m_break_all_key(void)
{
    uint8_t report_buf[16];
    bool nkro_temp = keymap_config.nkro;

    clear_weak_mods();
    clear_mods();
    clear_keyboard();

    keymap_config.nkro = 1;
    memset(keyboard_report, 0, sizeof(report_keyboard_t));
    host_keyboard_send(keyboard_report);
    wait_ms(10);

    keymap_config.nkro = 0;
    memset(keyboard_report, 0, sizeof(report_keyboard_t));
    host_keyboard_send(keyboard_report);
    wait_ms(10);

    keymap_config.nkro = nkro_temp;

    if (dev_info.link_mode != LINK_USB) {
        memset(report_buf, 0, 16);
        uart_send_report(CMD_RPT_BIT_KB, report_buf, 16);
        wait_ms(10);
        uart_send_report(CMD_RPT_BYTE_KB, report_buf, 8);
        wait_ms(10);
    }

    memset(uart_bit_report_buf, 0, sizeof(uart_bit_report_buf));
    memset(bitkb_report_buf, 0, sizeof(bitkb_report_buf));
    memset(bytekb_report_buf, 0, sizeof(bytekb_report_buf));
}

/**
 * @brief  switch device link mode.
 * @param mode : link mode
 */
static void switch_dev_link(uint8_t mode)
{
    if (mode > LINK_USB) return;
    m_break_all_key();

    dev_info.link_mode = mode;
    dev_info.rf_state = RF_IDLE;
    f_send_channel = 1;

    if (mode == LINK_USB) {
        host_mode = HOST_USB_TYPE;
        host_set_driver(m_host_driver);
        rf_link_show_time = 0;
    }
    else {
        host_mode = HOST_RF_TYPE;
        host_set_driver(0);

    }
}

/**
 * @brief  scan dial switch.
 */
void dial_sw_scan(void)
{
    uint8_t dial_scan               = 0;
    static uint8_t dial_save        = 0xf0;
    static uint8_t debounce         = 0;
    static uint32_t dial_scan_timer = 0;
    static bool     f_first         = true;

    if (!f_first) {
        if (timer_elapsed32(dial_scan_timer) < 20) return;
    }
    dial_scan_timer = timer_read32();

    setPinInputHigh(DEV_MODE_PIN);
    setPinInputHigh(SYS_MODE_PIN);

    if (readPin(DEV_MODE_PIN)) dial_scan |= 0X01;
    if (readPin(SYS_MODE_PIN)) dial_scan |= 0X02;

    if (dial_save != dial_scan) {
        m_break_all_key();

        no_act_time     = 0;
        rf_linking_time = 0;

        dial_save         = dial_scan;
        debounce          = 25;
        f_dial_sw_init_ok = 0;
        return;
    } else if (debounce) {
        debounce--;
        return;
    }

    if (dial_scan & 0x01) {
        if (dev_info.link_mode != LINK_USB) {
            switch_dev_link(LINK_USB);
        }
    } else {
        if (dev_info.link_mode != dev_info.rf_channel) {
            switch_dev_link(dev_info.rf_channel);
        }
    }

    if (f_macwin_ignore_switch == 0) {
        if (dial_scan & 0x02) {
            if (dev_info.sys_sw_state != SYS_SW_MAC) {
                f_sys_show = 1;
                default_layer_set(1 << 0);
                dev_info.sys_sw_state = SYS_SW_MAC;
                keymap_config.nkro    = 0;
                m_break_all_key();
            }
        } else {
            if (dev_info.sys_sw_state != SYS_SW_WIN) {
                f_sys_show = 1;
                default_layer_set(1 << 2);
                dev_info.sys_sw_state = SYS_SW_WIN;
                keymap_config.nkro    = 1;
                m_break_all_key();
            }
        }
    }

    if (f_dial_sw_init_ok == 0) {
        f_dial_sw_init_ok = 1;
        f_first           = false;

        if (dev_info.link_mode != LINK_USB) {
            host_set_driver(0);
        }
    }
}

/**
 * @brief  power on scan dial switch.
 */
void m_power_on_dial_sw_scan(void)
{
    uint8_t dial_scan_dev = 0;
    uint8_t dial_scan_sys = 0;
    uint8_t dial_check_dev = 0;
    uint8_t dial_check_sys = 0;
    uint8_t debounce = 0;

    setPinInputHigh(DEV_MODE_PIN);
    setPinInputHigh(SYS_MODE_PIN);

    for(debounce=0; debounce<10; debounce++) {
        dial_scan_dev = 0;
        dial_scan_sys = 0;
        if (readPin(DEV_MODE_PIN)) dial_scan_dev = 0x01;
        else dial_scan_dev = 0;
        if (readPin(SYS_MODE_PIN)) dial_scan_sys = 0x01;
        else dial_scan_sys = 0;
        if((dial_scan_dev != dial_check_dev)||(dial_scan_sys != dial_check_sys))
        {
            dial_check_dev = dial_scan_dev;
            dial_check_sys = dial_scan_sys;
            debounce = 0;
        }
        wait_ms(1);
    }
    // RF link mode
    if (dial_scan_dev) {
        if (dev_info.link_mode != LINK_USB) {
            switch_dev_link(LINK_USB);
        }
    } else {
        if (dev_info.link_mode != dev_info.rf_channel) {
            switch_dev_link(dev_info.rf_channel);
        }
    }

    if (dial_scan_sys) {
        if (dev_info.sys_sw_state != SYS_SW_MAC) {
            default_layer_set(1 << 0);
            dev_info.sys_sw_state = SYS_SW_MAC;
            keymap_config.nkro    = 0;
            m_break_all_key();
        }
    } else {
        if (dev_info.sys_sw_state != SYS_SW_WIN) {
            default_layer_set(1 << 2);
            dev_info.sys_sw_state = SYS_SW_WIN;
            keymap_config.nkro    = 1;
            m_break_all_key();
        }
    }
}


/**
 * @brief  process keyboard indicator rgb input.
 *
 * - caps lock selection
     press 'c' followed by 0|1|2
     0: off
     1: left side led
     2: caps lock key led

   - num lock selection
     press 'n' followed by 0|1|2
     0: off
     1: right side led
     2: num lock key led

     enter rgb values each value closed with 'enter' key

     example to use caps lock key led as its indicator in color red:

     c2255
     0
     0
 */
bool process_keyb_indicator_config_input(uint16_t keycode, keyb_indicator_config_input_t* config_input)
{
    dprintf("[keyb ind]: state=%d, key=%d\n", config_input->input_state, keycode);

    if (keycode == KC_ESC) {
        memset(config_input, 0, sizeof(keyb_indicator_config_input_t));
        return true;
    }

    if (config_input->input_state == INPUT_BEGIN) {
        if (keycode == KC_C) {
            config_input->input_state = INPUT_CAPSLOCK_SELECT;
            config_input->indicator = INDICATOR_CAPSLOCK;
        }
        if (keycode == KC_N) {
            config_input->input_state = INPUT_NUMLOCK_SELECT;
            config_input->indicator = INDICATOR_NUMLOCK;
        }
        return false;
    }
    if (config_input->input_state == INPUT_CAPSLOCK_SELECT ||
        config_input->input_state == INPUT_NUMLOCK_SELECT ) {

        if (keycode >= KC_1 && keycode <= KC_0) {
            if (config_input->indicator != INDICATOR_NONE) {
                config_input->indicator_select = (keycode == KC_0)? 0: keycode - KC_1 + 1;
                keyb_indicator_config[config_input->indicator-1].indicator_select = config_input->indicator_select;
                config_input->input_state = INPUT_RGB;
            }
        }
        return false;
    }
    if (config_input->input_state == INPUT_RGB) {
        if (keycode >= KC_1 && keycode <= KC_0) {
            config_input->rgb *= 10;
            config_input->rgb += (keycode == KC_0)? 0: keycode - KC_1 + 1;
        }
        if (keycode == KC_ENTER && config_input->indicator) {
            keyb_indicator_config[config_input->indicator-1].rgb[config_input->rgb_index] = config_input->rgb;
            config_input->rgb_index++;
            config_input->rgb = 0;

            if (config_input->rgb_index > 2) {
                {
                    const uint8_t* rgb = keyb_indicator_config[config_input->indicator-1].rgb; (void)rgb;
                    dprintf("[keyb ind]: indicator=%d,%d rgb=%d,%d,%d\n", config_input->indicator
                                                        , config_input->indicator_select
                                                        , rgb[0], rgb[1], rgb[2]
                                                        );
                }

                memset(config_input, 0, sizeof(keyb_indicator_config_input_t));
                return true;
            }
        }
        return false;
    }

    return false;
}

/**
 * @brief  qmk process record
 */
bool process_record_user(uint16_t keycode, keyrecord_t *record)
{
    if (f_keyb_indicator_config && record->event.pressed) {
        if (process_keyb_indicator_config_input(keycode, &indicator_config_input)) {
            f_keyb_indicator_config = 0;
        }
    }

    switch (keycode) {
        case RF_DFU:
            if (record->event.pressed) {
                if (dev_info.link_mode != LINK_USB) return false;
                uart_send_cmd(CMD_RF_DFU, 10, 20);
            }
            return false;

        case LNK_USB:
            if (record->event.pressed) {
                m_break_all_key();
            } else {
                dev_info.link_mode = LINK_USB;
                uart_send_cmd(CMD_SET_LINK, 10, 10);
                rf_blink_cnt = 3;
            }
            return false;

        case LNK_RF:
            if (record->event.pressed) {
                if (dev_info.link_mode != LINK_USB) {
                    rf_sw_temp    = LINK_RF_24;
                    f_rf_sw_press = 1;
                    m_break_all_key();
                }
            } else if (f_rf_sw_press) {
                f_rf_sw_press = 0;
                if (rf_sw_press_delay < RF_LONG_PRESS_DELAY) {
                    dev_info.link_mode   = rf_sw_temp;
                    dev_info.rf_channel  = rf_sw_temp;
                    dev_info.ble_channel = rf_sw_temp;
                    uart_send_cmd(CMD_SET_LINK, 10, 20);
                }
            }
            return false;

        case LNK_BLE1:
            if (record->event.pressed) {
                if (dev_info.link_mode != LINK_USB) {
                    rf_sw_temp    = LINK_BT_1;
                    f_rf_sw_press = 1;
                    m_break_all_key();
                }
            } else if (f_rf_sw_press) {
                f_rf_sw_press = 0;
                if (rf_sw_press_delay < RF_LONG_PRESS_DELAY) {
                    dev_info.link_mode   = rf_sw_temp;
                    dev_info.rf_channel  = rf_sw_temp;
                    dev_info.ble_channel = rf_sw_temp;
                    uart_send_cmd(CMD_SET_LINK, 10, 20);
                }
            }
            return false;

        case LNK_BLE2:
            if (record->event.pressed) {
                if (dev_info.link_mode != LINK_USB) {
                    rf_sw_temp    = LINK_BT_2;
                    f_rf_sw_press = 1;
                    m_break_all_key();
                }
            } else if (f_rf_sw_press) {
                f_rf_sw_press = 0;
                if (rf_sw_press_delay < RF_LONG_PRESS_DELAY) {
                    dev_info.link_mode   = rf_sw_temp;
                    dev_info.rf_channel  = rf_sw_temp;
                    dev_info.ble_channel = rf_sw_temp;
                    uart_send_cmd(CMD_SET_LINK, 10, 20);
                }
            }
            return false;

        case LNK_BLE3:
            if (record->event.pressed) {
                if (dev_info.link_mode != LINK_USB) {
                    rf_sw_temp    = LINK_BT_3;
                    f_rf_sw_press = 1;
                    m_break_all_key();
                }
            } else if (f_rf_sw_press) {
                f_rf_sw_press = 0;
                if (rf_sw_press_delay < RF_LONG_PRESS_DELAY) {
                    dev_info.link_mode   = rf_sw_temp;
                    dev_info.rf_channel  = rf_sw_temp;
                    dev_info.ble_channel = rf_sw_temp;
                    uart_send_cmd(CMD_SET_LINK, 10, 20);
                }
            }
            return false;

        case MAC_TASK:
            if (record->event.pressed) {
                host_consumer_send(0x029F);
            } else {
                host_consumer_send(0);
            }
            return false;

        case MAC_SEARCH:
            if (record->event.pressed) {
                register_code(KC_LGUI);
                register_code(KC_SPACE);
                uart_send_report_func();
                wait_ms(50);
                unregister_code(KC_LGUI);
                unregister_code(KC_SPACE);
            }
            return false;

        case MAC_VOICE:
            if (record->event.pressed) {
                host_consumer_send(0xcf);
            } else {
                host_consumer_send(0);
            }
            return false;

        case MAC_CONSOLE:
            if (record->event.pressed) {
                host_consumer_send(0x02A0);
            } else {
                host_consumer_send(0);
            }
            return false;

        case MAC_DND:
            if (record->event.pressed) {
                host_system_send(0x9b);
            } else {
                host_system_send(0);
            }
            return false;

        case MAC_PRT:
            if (record->event.pressed) {
                register_code(KC_LGUI);
                register_code(KC_LSFT);
                register_code(KC_3);
                wait_ms(50);
                unregister_code(KC_3);
                unregister_code(KC_LSFT);
                unregister_code(KC_LGUI);
            }
            return false;

        case MAC_PRTA:
            if (record->event.pressed) {
                if (keymap_config.nkro) {
                    register_code(KC_LGUI);
                    register_code(KC_LSFT);
                    register_code(KC_S);
                    wait_ms(50);
                    unregister_code(KC_S);
                    unregister_code(KC_LSFT);
                    unregister_code(KC_LGUI);
                }
                else {
                    register_code(KC_LGUI);
                    register_code(KC_LSFT);
                    register_code(KC_4);
                    wait_ms(50);
                    unregister_code(KC_4);
                    unregister_code(KC_LSFT);
                    unregister_code(KC_LGUI);
                }
            }
            return false;

        case SIDE_VAI:
            if (record->event.pressed) {
                light_level_control(1);
            }
            return false;

        case SIDE_VAD:
            if (record->event.pressed) {
                light_level_control(0);
            }
            return false;

        case SIDE_MOD:
            if (record->event.pressed) {
                side_mode_control(1);
            }
            return false;

        case SIDE_HUI:
            if (record->event.pressed) {
                side_colour_control(1);
            }
            return false;

        case SIDE_SPI:
            if (record->event.pressed) {
                light_speed_contol(1);
            }
            return false;

        case SIDE_SPD:
            if (record->event.pressed) {
                light_speed_contol(0);
            }
            return false;

        case DEV_RESET:
            if (record->event.pressed) {
                f_dev_reset_press = 1;
                m_break_all_key();
            } else {
                f_dev_reset_press = 0;
            }
            return false;

        case SLEEP_MODE:
            if (record->event.pressed) {
                if(user_config.sleep_enable) user_config.sleep_enable = false;
                else user_config.sleep_enable = true;
                f_sleep_show       = 1;
                eeconfig_update_user_datablock(&user_config);
            }
            return false;

        case BAT_SHOW:
            if (record->event.pressed) {
                f_bat_hold = !f_bat_hold;
            }
            return false;

        case RGB_TEST:
            if (record->event.pressed) {
                f_rgb_test_press = 1;
            } else {
                f_rgb_test_press = 0;
            }
            return false;

        case KEYB_INDICATOR_CONFIG:
            if (record->event.pressed) {
                f_keyb_indicator_config = 1;
                memset(&indicator_config_input, 0, sizeof(keyb_indicator_config_input_t));
            }
            return false;

        case BAT_NUM:
            if (record->event.pressed) {
                f_bat_num_show = 1;
            } else {
                f_bat_num_show = 0;
            }
            return false;

        default:
            return true;
    }
}


/**
 *   @brief  timer process.
 */
void timer_pro(void)
{
    static uint32_t interval_timer = 0;
    static bool f_first            = true;

    if (f_first) {
        f_first        = false;
        interval_timer = timer_read32();
        m_host_driver  = host_get_driver();
    }

    if (timer_elapsed32(interval_timer) < 10) {
        return;
    } else if (timer_elapsed32(interval_timer) > 20) {
        interval_timer = timer_read32();
    } else {
        interval_timer += 10;
    }

    if (rf_link_show_time < RF_LINK_SHOW_TIME)
        rf_link_show_time++;

    if (no_act_time < 0xffff)
        no_act_time++;

    if (rf_linking_time < 0xffff)
        rf_linking_time++;
}


/**
 * @brief  londing eeprom data.
 */
void m_londing_eeprom_data(void)
{
    eeconfig_read_user_datablock(&user_config);
    if (user_config.default_brightness_flag != 0xA5) {
        rgb_matrix_sethsv(RGB_MATRIX_NUPHY_DEFAULT_HSV);
        user_config.default_brightness_flag = 0xA5;
        user_config.ee_side_mode            = side_mode;
        user_config.ee_side_light           = side_light;
        user_config.ee_side_speed           = side_speed;
        user_config.ee_side_rgb             = side_rgb;
        user_config.ee_side_colour          = side_colour;
        user_config.sleep_enable            = SLEEP_ENABLE_DEFAULT;
        eeconfig_update_user_datablock(&user_config);
    } else {
        side_mode   = user_config.ee_side_mode;
        side_light  = user_config.ee_side_light;
        side_speed  = user_config.ee_side_speed;
        side_rgb    = user_config.ee_side_rgb;
        side_colour = user_config.ee_side_colour;
    }
}



/**
 *   qmk keyboard post init
 */
void keyboard_post_init_user(void)
{
    m_gpio_init();
    rf_uart_init();
    wait_ms(500);
    rf_device_init();

    m_break_all_key();
    m_londing_eeprom_data();
    m_power_on_dial_sw_scan();

#ifdef FIRMATA_ENABLE
    firmata_initialize("Nuphy Firmata");
    firmata_attach(0, firmata_sysex_handler);
#endif
}

/**
   rgb_matrix_indicators_user
 */
bool rgb_matrix_indicators_user(void)
{
    if(f_bat_num_show) {
        num_led_show();
    }
    return true;
}


#ifdef FIRMATA_ENABLE

extern rgb_matrix_host_buffer_t g_rgb_matrix_host_buf;

// show rgb matrix set by user on host side
void rgb_matrix_host_show(void)
{
    if (!g_rgb_matrix_host_buf.written) return;

    bool matrix_set = 0;
    for (uint8_t li = 0; li < RGB_MATRIX_LED_COUNT; li++) {
        if (g_rgb_matrix_host_buf.led[li].duration > 0) {
            rgb_matrix_set_color(li, g_rgb_matrix_host_buf.led[li].r, g_rgb_matrix_host_buf.led[li].g, g_rgb_matrix_host_buf.led[li].b);
            g_rgb_matrix_host_buf.led[li].duration--;
            matrix_set = 1;
        }
    }

    if (!matrix_set) g_rgb_matrix_host_buf.written = 0;
}

#endif

/**
   housekeeping_task_user
 */
void housekeeping_task_user(void)
{
#if NUPHY_TASK_USER_TIME_MEASURE
    static uint32_t task_user_counter = 0;
    static uint32_t task_user_total_time = 0;
    uint32_t time_start = timer_read32();
    if (task_user_counter++ == 10000) {
        dprintf("[task_user] timer=%ld, total time=%ld\n", time_start, task_user_total_time);
        task_user_counter = 0;
        task_user_total_time = 0;
    }
#endif

    timer_pro();

    uart_receive_pro();

    uart_send_report_func();

    dev_sts_sync();

    long_press_key();

    dial_sw_scan();

    m_side_led_show(keyb_indicator_config);

#ifdef FIRMATA_ENABLE
    rgb_matrix_host_show();

    firmata_process();
#endif

#if NUPHY_TASK_USER_TIME_MEASURE
    task_user_total_time +=  timer_read32() - time_start;
#endif

    Sleep_Handle();
}
