enum VEReadWriteFlags {
    VE_READ = 0b01,
    VE_WRITE = 0b10,
    VE_READ_WRITE = 0b11,
};

enum VEValidityFlags {
    VALID_ALL = 0b11111111,
    VALID_BMV700 = 0b00000001,
    VALID_BMV702 = 0b00000010,
    VALID_BMV712 = 0b00000100,
};

enum VECommand {
    VE_CMD_PING = '1',
    VE_CMD_VERSION = '3',
    VE_CMD_ID = '4',
    VE_CMD_RESTART = '6',
    VE_CMD_GET = '7',
    VE_CMD_SET = '8',
    VE_CMD_ASYNC = 'A',
};

// Response Constants
#define VE_RSP_DONE '1'
#define VE_RSP_UNKNOWN '3'
#define VE_RSP_PING '5'
#define VE_RSP_GET '7'
#define VE_RSP_SET '8'

// Response Error Flags
#define VE_RSP_FLG_UNKNOWN 0x01
#define VE_RSP_FLG_UNSUPPORTED 0x02
#define VE_RSP_FLG_PARAMETER_ERROR 0x04

enum VEDirectType {
    VE_TYPE_NONE,
    VE_TYPE_UN8,
    VE_TYPE_SN8,
    VE_TYPE_UN16,
    VE_TYPE_SN16,
    VE_TYPE_UN24,
    VE_TYPE_SN24,
    VE_TYPE_UN32,
    VE_TYPE_SN32,
    VE_TYPE_STR20,
    VE_TYPE_STR32,
};

struct VEDirectMsg {
    char *name;
    uint16_t address;
    enum VEDirectType type;
    float multiplier;
    char *units;
    enum VEReadWriteFlags rw_flags;
    enum VEValidityFlags v_flags;
};

const struct VEDirectMsg vedirect_lookup[] = {
    { "id",                 0x0100, VE_TYPE_UN32,   1.0,        "",     VE_READ,        VALID_ALL       },
    { "main_voltage",       0xED8D, VE_TYPE_SN16,   0.01,       "V",    VE_READ,        VALID_ALL       },
    { "current_coarse",     0xED8F, VE_TYPE_SN16,   0.1,        "A",    VE_READ,        VALID_ALL       },
    { "soc",                0x0FFF, VE_TYPE_UN16,   0.01,       "%",    VE_READ,        VALID_ALL       },
    { "consumed_ah",        0xEEFF, VE_TYPE_SN32,   0.1,        "Ah",   VE_READ,        VALID_ALL       },
};

// TODO: Data table copied from first attempt python program, to be translated into above structure
/*
        'id':                       (0x0100, 'Un32', 1, None, VE_READ, None, None, None, None),
        'revision':                 (0x0101, 'Un24', 1, None, VE_READ, VALID_BMV712, None, None, None),
        'serial':                   (0x010A, 'String32', None, None, VE_READ, None, None, None, None),
        'model':                    (0x010B, 'String32', None, None, VE_READ, None, None, None, None),
        'description':              (0x010C, 'String20', None, None, VE_READ, VALID_BMV712, None, None, None),
        'uptime':                   (0x0120, 'Un32', 1, 's', VE_READ, None, None, None, None),
        'bluetooth':                (0x0150, 'Un32', 1, None, VE_READ, VALID_BMV712, None, None, None), # [0: HAS_SUPPORT_FOR_BLE_MODE, 1: BLE_MODE_OFF_IS_PERMANENT, 2-31: reserved]
        'main_voltage':             (0xED8D, 'Sn16', 0.01, 'V', VE_READ, None, None, None, None),
        'aux_voltage':              (0xED7D, 'Sn16', 0.01, 'V', VE_READ, VALID_BMV702 | VALID_BMV712, None, None, None),
        'current_coarse':           (0xED8F, 'Sn16', 0.1, 'A', VE_READ, None, None, None, None),
        'current_fine':             (0xED8C, 'Sn32', 0.001, 'A', VE_READ, None, None, None, None),
        'power'                     (0xED8D, 'Sn16', 1, 'W', VE_READ, None, None, None, Noue),
        'consumed_ah'               (0xEEFF, 'Sn32', 0.1, 'Ah', VE_READ, None, None, None, None),
        'soc':                      (0x0FFF, 'Un16', 0.01, '%', VE_READ, None, None, None, None),
        'ttg':                      (0x0FFE, 'Un16', 1, 'min', VE_READ, None, None, None, None),
        'temp':                     (0xEDEC, 'Un16', 0.01, '°K', VE_READ, VALID_BMV702 | VALID_BMV712, None, None, None),
        'midpoint_voltage':         (0x0382, 'Un16', 0.01, 'V', VE_READ, VALID_BMV702 | VALID_BMV712, None, None, None),
        'midpoint_voltage_dev':     (0x0383, 'Sn16', 0.1, '%', VE_READ, VALID_BMV702 | VALID_BMV712, None, None, None),
        'sync_state':               (0xEEB6, 'Un8', 1, None, VE_READ, None, None, None, None),
        'max_discharge':            (0x0300, 'Sn32', 0.1, 'Ah', VE_READ, None, None, None, None),
        'last_discharge':           (0x0301, 'Sn32', 0.1, 'Ah', VE_READ, None, None, None, None),
        'avg_discharge':            (0x0302, 'Sn32', 0.1, 'Ah', VE_READ, None, None, None, None),
        'num_cycles':               (0x0303, 'Un32', 1, None, VE_READ, None, None, None, None),
        'num_full_discharge':       (0x0304, 'Un32', 1, None, VE_READ, None, None, None, None),
        'cumulative_ah':            (0x0305, 'Sn32', 0.1, 'Ah', VE_READ, None, None, None, None),
        'min_voltage':              (0x0306, 'Sn32', 0.01, 'V', VE_READ, None, None, None, None),
        'max_voltage':              (0x0307, 'Sn32', 0.01, 'V', VE_READ, None, None, None, None),
        'time_since_full_charge':   (0x0308, 'Un32', 1, 's', VE_READ, None, None, None, None),
        'num_auto_sync':            (0x0309, 'Un32', 1, None, VE_READ, None, None, None, None),
        'num_low_volt_alarm':       (0x030A, 'Un32', 1, None, VE_READ, None, None, None, None),
        'num_high_volt_alarm':      (0x030B, 'Un32', 1, None, VE_READ, None, None, None, None),
        'min_aux_voltage':          (0x030E, 'Sn32', 0.01, 'V', VE_READ, VALID_BMV702 | VALID_BMV712, None, None, None),
        'max_aux_voltage':          (0x030F, 'Sn32', 0.01, 'V', VE_READ, VALID_BMV702 | VALID_BMV712, None, None, None),
        'energy_discharged':        (0x0310, 'Un32', 0.01, 'kWh', VE_READ, None, None, None, None),
        'energy_charged':           (0x0311, 'Un32', 0.01, 'kWh', VE_READ, None, None, None, None),
        'battery_capacity':         (0x1000, 'Un16', 1, 'Ah', VE_READ | VE_WRITE, None, None, None, None),
        'charged_voltage':          (0x1001, 'Un16', 0.1, 'V', VE_READ | VE_WRITE, None, None, None, None),
        'tail_current':             (0x1002, 'Un16', 0.1, '%', VE_READ | VE_WRITE, None, None, None, None),
        'charged_detection_time':   (0x1003, 'Un16', 1, 'min', VE_READ | VE_WRITE, None, None, None, None),
        'charge_efficiency':        (0x1004, 'Un16', 1, '%', VE_READ | VE_WRITE, None, None, None, None),
        'peukert_coefficient':      (0x1005, 'Un16', 0.01, None, VE_READ | VE_WRITE, None, None, None, None),
        'current_threshold':        (0x1006, 'Un16', 0.01, 'A', VE_READ | VE_WRITE, None, None, None, None),
        'ttg_delta_t':              (0x1007, 'Un16', 1, 'min', VE_READ | VE_WRITE, None, None, None, None),
        'relay_low_soc_set':        (0x1008, 'Un16', 0.1, '%', VE_READ | VE_WRITE, None, None, None, None),
        'relay_low_soc_clear':      (0x1009, 'Un16', 0.1, '%', VE_READ | VE_WRITE, None, None, None, None),
        'user_current_zero':        (0x1034, 'Sn16', 1, None, VE_READ, None, None, None, None),
        'alarm_buzzer':             (0xEEFC, 'Un8', 1, None, VE_READ | VE_WRITE, None, None, None, None), # bool
        'alarm_low_voltage':        (0x0320, 'Un16', 0.1, 'V', VE_READ | VE_WRITE, None, None, None, None),
        'alarm_low_voltage_clear':  (0x0321, 'Un16', 0.1, 'V', VE_READ | VE_WRITE, None, None, None, None),
        'alarm_high_voltage':       (0x0322, 'Un16', 0.1, 'V', VE_READ | VE_WRITE, None, None, None, None),
        'alarm_high_voltage_clear': (0x0323, 'Un16', 0.1, 'V', VE_READ | VE_WRITE, None, None, None, None),
        'alarm_low_aux_voltage':    (0x0324,'Un16', 0.1, 'V', VE_READ | VE_WRITE, VALID_BMV702 | VALID_BMV712, None, None, None),
        'alarm_low_aux_voltage_clear':(0x0325,'Un16', 0.1, 'V', VE_READ | VE_WRITE, VALID_BMV702 | VALID_BMV712, None, None, None),
        'alarm_high_aux_voltage':   (0x0326,'Un16', 0.1, 'V', VE_READ | VE_WRITE, VALID_BMV702 | VALID_BMV712, None, None, None),
        'alarm_high_aux_voltage_clear':(0x0327,'Un16', 0.1, 'V', VE_READ | VE_WRITE, VALID_BMV702 | VALID_BMV712, None, None, None),
        'alarm_low_soc':            (0x0328, 'Un16', 0.1, '%', VE_READ | VE_WRITE, None, None, None, None),
        'alarm_low_soc_clear':      (0x0329, 'Un16', 0.1, '%', VE_READ | VE_WRITE, None, None, None, None),
        'alarm_low_temperature':    (0x032A, 'Un16', 0.01, '°K', VE_READ | VE_WRITE, VALID_BMV702 | VALID_BMV712, None, None, None),
        'alarm_low_temperature_clear':(0x032B, 'Un16', 0.01, '°K', VE_READ | VE_WRITE, VALID_BMV702 | VALID_BMV712, None, None, None),
        'alarm_high_temperature':   (0x032C, 'Un16', 0.01, '°K', VE_READ | VE_WRITE, VALID_BMV702 | VALID_BMV712, None, None, None),
        'alarm_high_temperature_clear':(0x032D, 'Un16', 0.01, '°K', VE_READ | VE_WRITE, VALID_BMV702 | VALID_BMV712, None, None, None),
        'alarm_mid_voltage':        (0x0331, 'Un16', 0.1, '%', VE_READ | VE_WRITE, VALID_BMV712, None, None, None),
        'alarm_mid_voltage_clear':  (0x0332, 'Un16', 0.1, '%', VE_READ | VE_WRITE, VALID_BMV712, None, None, None),
        'alarm_acknowledge':        (0x031F, None, None, None, VE_WRITE, None, None, None, None),
        'relay_mode':               (0x034F, 'Un8', 1, None, VE_READ | VE_WRITE, None, None, None, None), # [0: default, 1: charge, 2: remain]
        'relay_invert':             (0x034D, 'Un8', 1, None, VE_READ | VE_WRITE, None, None, None, None), # bool
        'relay_state':              (0x034E, 'Un8', 1, None, VE_READ | VE_WRITE, None, None, None, None), # [0: open, 1: closed]
        'relay_min_enable_time':    (0x100A, 'Un16', 1, 'min', VE_READ | VE_WRITE, None, None, None, None),
        'relay_disable_time':       (0x100B, 'Un16', 1, 'min', VE_READ | VE_WRITE, None, None, None, None),
        'relay_low_voltage':        (0x0350, 'Un16', 0.1, 'V', VE_READ | VE_WRITE, None, None, None, None),
        'relay_low_voltage_clear':  (0x0351, 'Un16', 0.1, 'V', VE_READ | VE_WRITE, None, None, None, None),
        'relay_high_voltage':       (0x0352, 'Un16', 0.1, 'V', VE_READ | VE_WRITE, None, None, None, None),
        'relay_high_voltage_clear': (0x0353, 'Un16', 0.1, 'V', VE_READ | VE_WRITE, None, None, None, None),
        'relay_aux_low_voltage':    (0x0354, 'Un16', 0.1, 'V', VE_READ | VE_WRITE, VALID_BMV702 | VALID_BMV712, None, None, None),
        'relay_aux_low_voltage_clear':(0x0355, 'Un16', 0.1, 'V', VE_READ | VE_WRITE, VALID_BMV702 | VALID_BMV712, None, None, None),
        'relay_aux_high_voltage':   (0x0356, 'Un16', 0.1, 'V', VE_READ | VE_WRITE, VALID_BMV702 | VALID_BMV712, None, None, None),
        'relay_aux_high_voltage_clear':(0x0357, 'Un16', 0.1, 'V', VE_READ | VE_WRITE, VALID_BMV702 | VALID_BMV712, None, None, None),
        'relay_low_temperature':    (0x035A, 'Un16', 0.01, '°K', VE_READ | VE_WRITE, VALID_BMV702 | VALID_BMV712, None, None, None),
        'relay_low_temperature_clear':(0x035B, 'Un16', 0.01, '°K', VE_READ | VE_WRITE, VALID_BMV702 | VALID_BMV712, None, None, None),
        'relay_high_temperature':   (0x035C, 'Un16', 0.01, '°K', VE_READ | VE_WRITE, VALID_BMV702 | VALID_BMV712, None, None, None),
        'relay_high_temperature_clear':(0x035D, 'Un16', 0.01, '°K', VE_READ | VE_WRITE, VALID_BMV702 | VALID_BMV712, None, None, None),
        'relay_mid_voltage':        (0x0361, 'Un16', 0.1, '%', VE_READ | VE_WRITE, VALID_BMV712, None, None, None),
        'relay_mid_voltage_clear':  (0x0362, 'Un16', 0.1, '%', VE_READ | VE_WRITE, VALID_BMV712, None, None, None),
        'backlight_intensity':      (0xEEFE, 'Un8', 1, None, VE_READ | VE_WRITE, None, None, None, None),
        'backlight_always_on':      (0x0400, 'Un8', 1, None, VE_READ | VE_WRITE, None, None, None, None), # bool
        'scroll_speed':             (0xEEF5, 'Un8', 1, None, VE_READ | VE_WRITE, None, None, None, None),
        'show_voltage':             (0xEEE0, 'Un8', 1, None, VE_READ | VE_WRITE, None, None, None, None), # bool
        'show_aux_voltage':         (0xEEE1, 'Un8', 1, None, VE_READ | VE_WRITE, VALID_BMV702 | VALID_BMV712, None, None, None), # bool
        'show_mid_voltage':         (0xEEE2, 'Un8', 1, None, VE_READ | VE_WRITE, VALID_BMV702 | VALID_BMV712, None, None, None), # bool
        'show_current':             (0xEEE3, 'Un8', 1, None, VE_READ | VE_WRITE, None, None, None, None), # bool
        'show_consumed_ah':         (0xEEE4, 'Un8', 1, None, VE_READ | VE_WRITE, None, None, None, None), # bool
        'show_soc':                 (0xEEE5, 'Un8', 1, None, VE_READ | VE_WRITE, None, None, None, None), # bool
        'show_ttg':                 (0xEEE6, 'Un8', 1, None, VE_READ | VE_WRITE, None, None, None, None), # bool
        'show_temperature':         (0xEEE7, 'Un8', 1, None, VE_READ | VE_WRITE, VALID_BMV702 | VALID_BMV712, None, None, None), # bool
        'show_power':               (0xEEE8, 'Un8', 1, None, VE_READ | VE_WRITE, None, None, None, None), # bool
        'zero_current':             (0x1029, None, 1, None, VE_WRITE, None, None, None, None),
        'sync':                     (0x102C, None, 1, None, VE_WRITE, None, None, None, None),
        'restore_defaults':         (0x0004, None, 1, None, VE_WRITE, None, None, None, None),
        'clear_history':            (0x1030, None, 1, None, VE_WRITE, None, None, None, None),
        'sw_version':               (0xEEF9, 'Un16', 1, None, VE_READ, None, None, None, None),
        'setup_lock':               (0xEEF6, 'Un8', 1, None, VE_READ | VE_WRITE, None, None, None, None),  # bool
        'shunt_amps':               (0xEEFB, 'Un16', 1, 'A', VE_READ | VE_WRITE, None, None, None, None),
        'shunt_volts':              (0xEEFA, 'Un16', 0.001, 'V', VE_READ | VE_WRITE, None, None, None, None),
        'temperature_unit':         (0xEEF7, 'Un8', 1, None, VE_READ | VE_WRITE, VALID_BMV702 | VALID_BMV712, None, None, None), # [0: celcius, 1: fahrenheit]
        'temperature_coefficient':  (0xEEF4, 'Un16', 0.1, '%CAP/°C', VE_READ | VE_WRITE, VALID_BMV702 | VALID_BMV712, None, None, None),
        'aux_input':                (0xEEF8, 'Un8', 1, None, VE_READ | VE_WRITE, VALID_BMV702 | VALID_BMV712, None, None, None), # [0: start, 1: mid, 2: temp]
        'start_sync':               (0x0FFD, 'Un8', 1, None, VE_READ | VE_WRITE, None, None, None, None), # bool
        'settings_changed_timestamp':(0xEC41, 'Un32', 1, 's', VE_READ | VE_WRITE, VALID_BMV712, None, None, None), # [0: local change, 0x00000001-0xFFFFFFFE: seconds since change by app, 0xFFFFFFFF: no change]
        'bluetooth_mode':           (0x0090, 'Un8', 1, None, VE_READ | VE_WRITE, VALID_BMV712, None, None, None), # [0: disabled, 1: enabled, 2-7: reserved]
*/

bool ve_lookup_by_name(struct VEDirectMsg *vedirect_msg, const char *name) {
    for (int i = 0; i < ( sizeof(vedirect_lookup) / sizeof(struct VEDirectMsg) ); i++) {
        if( !strcmp(name, vedirect_lookup[i].name) ) {
            *vedirect_msg = vedirect_lookup[i];
            return true;
        }
    }

    return false;
}

bool ve_lookup_by_address(struct VEDirectMsg *vedirect_msg, const unsigned int address) {
    for (int i = 0; i < ( sizeof(vedirect_lookup) / sizeof(struct VEDirectMsg) ); i++) {
        if( address == vedirect_lookup[i].address ) {
            *vedirect_msg = vedirect_lookup[i];
            return true;
        }
    }

    return false;
}
