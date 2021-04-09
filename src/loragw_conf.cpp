#include "loragw_hal.h"
#include "loragw_conf.h"

/* -------------------------------------------------------------------------- */
/* --- MACROS PRIVADAS ------------------------------------------------------- */

char dbug_msg[100];

#define ARRAY_SIZE(a)   (sizeof(a) / sizeof((a)[0]))
#define MSG(args...)    {\
                          memset(dbug_msg, 0, sizeof(dbug_msg));\
                          sprintf(dbug_msg,"loragw_pkt_logger: " args);\
                          Serial.print(dbug_msg);\
                          }

/* -------------------------------------------------------------------------- */
/* --- VARIABLES PRIVADAS ---------------------------------------------------- */

const lgw_conf_board_s boardconf {
    .lorawan_public=true,           /*!> Enable ONLY for *public* networks using the LoRa MAC protocol */
    .clksrc=1                       /*!> Index of RF chain which provides clock to concentrator */
};

const lgw_conf_rxrf_s rfconf[2]  {
    {   //radio_0
        .enable=true,                           /*!> enable or disable that RF chain */
        .freq_hz=917200000,                     /*!> center frequency of the radio in Hz */
        .rssi_offset=-159.0,                    /*!> Board-specific RSSI correction factor */
        .type=LGW_RADIO_TYPE_SX1257,            /*!> Radio type for that RF chain (SX1255, SX1257....) */
        .tx_enable=true                         /*!> enable or disable TX on that RF chain */
    },
    {   //radio_1
        .enable=true,                           /*!> enable or disable that RF chain */
        .freq_hz=917900000,                     /*!> center frequency of the radio in Hz */
        .rssi_offset=-159.0,                    /*!> Board-specific RSSI correction factor */
        .type=LGW_RADIO_TYPE_SX1257,            /*!> Radio type for that RF chain (SX1255, SX1257....) */
        .tx_enable=false                        /*!> enable or disable TX on that RF chain */
    }
};

const lgw_conf_rxif_s ifconfig[10] {
    { //chan_multiSF_0
    .enable=true,                       /*!> enable or disable that IF chain */
    .rf_chain=0,                        /*!> to which RF chain is that IF chain associated */
    .freq_hz=-400000                    /*!> center frequ of the IF chain, relative to RF chain frequency */
    },
    { //chan_multiSF_1
    .enable=true,                       /*!> enable or disable that IF chain */
    .rf_chain=0,                        /*!> to which RF chain is that IF chain associated */
    .freq_hz=-200000                    /*!> center frequ of the IF chain, relative to RF chain frequency */
    },
    { //chan_multiSF_2
    .enable=true,                       /*!> enable or disable that IF chain */
    .rf_chain=0,                        /*!> to which RF chain is that IF chain associated */
    .freq_hz=-0                         /*!> center frequ of the IF chain, relative to RF chain frequency */
    },
    { //chan_multiSF_3
    .enable=true,                       /*!> enable or disable that IF chain */
    .rf_chain=0,                        /*!> to which RF chain is that IF chain associated */
    .freq_hz=200000                    /*!> center frequ of the IF chain, relative to RF chain frequency */
    },
    { //chan_multiSF_4
    .enable=true,                       /*!> enable or disable that IF chain */
    .rf_chain=1,                        /*!> to which RF chain is that IF chain associated */
    .freq_hz=-300000                    /*!> center frequ of the IF chain, relative to RF chain frequency */
    },
    { //chan_multiSF_5
    .enable=true,                       /*!> enable or disable that IF chain */
    .rf_chain=1,                        /*!> to which RF chain is that IF chain associated */
    .freq_hz=-100000                    /*!> center frequ of the IF chain, relative to RF chain frequency */
    },
    { //chan_multiSF_6
    .enable=true,                       /*!> enable or disable that IF chain */
    .rf_chain=1,                        /*!> to which RF chain is that IF chain associated */
    .freq_hz=100000                    /*!> center frequ of the IF chain, relative to RF chain frequency */
    },
    { //chan_multiSF_7
    .enable=true,                       /*!> enable or disable that IF chain */
    .rf_chain=1,                        /*!> to which RF chain is that IF chain associated */
    .freq_hz=300000                    /*!> center frequ of the IF chain, relative to RF chain frequency */
    },
    { //chan_Lora_std
    .enable=true,                 /*!> enable or disable that IF chain */
    .rf_chain=0,       /*!> to which RF chain is that IF chain associated */
    .freq_hz=300000,       /*!> center frequ of the IF chain, relative to RF chain frequency */
    .bandwidth=BW_500KHZ,      /*!> RX bandwidth, 0 for default */
    .datarate=DR_LORA_SF7       /*!> RX datarate, 0 for default */
    },
    { //chan_FSK
    .enable=false,                 /*!> enable or disable that IF chain */
    }
};

const lgw_tx_gain_lut_s txgain_lut = {
    {{
        .dig_gain = 0,
        .pa_gain = 0,
        .dac_gain = 3,
        .mix_gain = 12,
        .rf_power = 0
    },
    {
        .dig_gain = 0,
        .pa_gain = 1,
        .dac_gain = 3,
        .mix_gain = 12,
        .rf_power = 10
    },
    {
        .dig_gain = 0,
        .pa_gain = 2,
        .dac_gain = 3,
        .mix_gain = 10,
        .rf_power = 14
    },
    {
        .dig_gain = 0,
        .pa_gain = 3,
        .dac_gain = 3,
        .mix_gain = 9,
        .rf_power = 20
    },
    {
        .dig_gain = 0,
        .pa_gain = 3,
        .dac_gain = 3,
        .mix_gain = 14,
        .rf_power = 27
    }},
    .size = 5,
};

/* -------------------------------------------------------------------------- */
/* --- FUNCIONES PRIVADAS --------------------------------------------------- */

int parse_SX1301_configuration(const char * conf_file) {
    int i;
    const char conf_obj[] = "SX1301_conf";
    char param_name[32]; /* used to generate variable parameter names */
    const char *str; /* used to store string value from JSON object */
    uint32_t sf, bw;



    /* set board configuration */
    /* submitting configuration to the HAL */
    if (lgw_board_setconf(boardconf) != LGW_HAL_SUCCESS) {
        MSG("ERROR: Failed to configure board\n");
        return -1;
    }

    /* set configuration for RF chains */
    for (i = 0; i < LGW_RF_CHAIN_NB; ++i) {
        if (rfconf[i].enable == false) { /* radio disabled, nothing else to parse */
            MSG("INFO: radio %i disabled\n", i); 
        } else  { /* radio enabled, will parse the other parameters */
            if (rfconf[i].type == LGW_RADIO_TYPE_SX1255) {
                snprintf(param_name, sizeof param_name, "SX1255", i);
            } else if (rfconf[i].type == LGW_RADIO_TYPE_SX1257) {
                snprintf(param_name, sizeof param_name, "SX1257", i);
            } else {
                MSG("WARNING: invalid radio type: %s (should be SX1255 or SX1257)\n", str);
            }
            MSG("INFO: radio %i enabled (type %s), center frequency %u, RSSI offset %f, tx enabled %d\n", i, param_name, rfconf[i].freq_hz, rfconf[i].rssi_offset, rfconf[i].tx_enable);
        }
        /* all parameters parsed, submitting configuration to the HAL */
        if (lgw_rxrf_setconf(i, rfconf[i]) != LGW_HAL_SUCCESS) {
            MSG("ERROR: invalid configuration for radio %i\n", i);
            return -1;
        }
    }

    /* set configuration for LoRa multi-SF channels (bandwidth cannot be set) */
    for (i = 0; i < LGW_MULTI_NB; ++i) {
        memset(&ifconf, 0, sizeof(ifconf)); /* initialize configuration structure */
        sprintf(param_name, "chan_multiSF_%i", i); /* compose parameter path inside JSON structure */
        val = json_object_get_value(conf, param_name); /* fetch value (if possible) */
        if (json_value_get_type(val) != JSONObject) {
            MSG("INFO: no configuration for LoRa multi-SF channel %i\n", i);
            continue;
        }
        /* there is an object to configure that LoRa multi-SF channel, let's parse it */
        sprintf(param_name, "chan_multiSF_%i.enable", i);
        val = json_object_dotget_value(conf, param_name);
        if (json_value_get_type(val) == JSONBoolean) {
            ifconf.enable = (bool)json_value_get_boolean(val);
        } else {
            ifconf.enable = false;
        }
        if (ifconf.enable == false) { /* LoRa multi-SF channel disabled, nothing else to parse */
            MSG("INFO: LoRa multi-SF channel %i disabled\n", i);
        } else  { /* LoRa multi-SF channel enabled, will parse the other parameters */
            sprintf(param_name, "chan_multiSF_%i.radio", i);
            ifconf.rf_chain = (uint32_t)json_object_dotget_number(conf, param_name);
            sprintf(param_name, "chan_multiSF_%i.if", i);
            ifconf.freq_hz = (int32_t)json_object_dotget_number(conf, param_name);
            // TODO: handle individual SF enabling and disabling (spread_factor)
            MSG("INFO: LoRa multi-SF channel %i enabled, radio %i selected, IF %i Hz, 125 kHz bandwidth, SF 7 to 12\n", i, ifconf.rf_chain, ifconf.freq_hz);
        }
        /* all parameters parsed, submitting configuration to the HAL */
        if (lgw_rxif_setconf(i, ifconf) != LGW_HAL_SUCCESS) {
            MSG("ERROR: invalid configuration for Lora multi-SF channel %i\n", i);
            return -1;
        }
    }

    /* set configuration for LoRa standard channel */
    memset(&ifconf, 0, sizeof(ifconf)); /* initialize configuration structure */
    val = json_object_get_value(conf, "chan_Lora_std"); /* fetch value (if possible) */
    if (json_value_get_type(val) != JSONObject) {
        MSG("INFO: no configuration for LoRa standard channel\n");
    } else {
        val = json_object_dotget_value(conf, "chan_Lora_std.enable");
        if (json_value_get_type(val) == JSONBoolean) {
            ifconf.enable = (bool)json_value_get_boolean(val);
        } else {
            ifconf.enable = false;
        }
        if (ifconf.enable == false) {
            MSG("INFO: LoRa standard channel %i disabled\n", i);
        } else  {
            ifconf.rf_chain = (uint32_t)json_object_dotget_number(conf, "chan_Lora_std.radio");
            ifconf.freq_hz = (int32_t)json_object_dotget_number(conf, "chan_Lora_std.if");
            bw = (uint32_t)json_object_dotget_number(conf, "chan_Lora_std.bandwidth");
            switch(bw) {
                case 500000: ifconf.bandwidth = BW_500KHZ; break;
                case 250000: ifconf.bandwidth = BW_250KHZ; break;
                case 125000: ifconf.bandwidth = BW_125KHZ; break;
                default: ifconf.bandwidth = BW_UNDEFINED;
            }
            sf = (uint32_t)json_object_dotget_number(conf, "chan_Lora_std.spread_factor");
            switch(sf) {
                case  7: ifconf.datarate = DR_LORA_SF7;  break;
                case  8: ifconf.datarate = DR_LORA_SF8;  break;
                case  9: ifconf.datarate = DR_LORA_SF9;  break;
                case 10: ifconf.datarate = DR_LORA_SF10; break;
                case 11: ifconf.datarate = DR_LORA_SF11; break;
                case 12: ifconf.datarate = DR_LORA_SF12; break;
                default: ifconf.datarate = DR_UNDEFINED;
            }
            MSG("INFO: LoRa standard channel enabled, radio %i selected, IF %i Hz, %u Hz bandwidth, SF %u\n", ifconf.rf_chain, ifconf.freq_hz, bw, sf);
        }
        if (lgw_rxif_setconf(8, ifconf) != LGW_HAL_SUCCESS) {
            MSG("ERROR: invalid configuration for Lora standard channel\n");
            return -1;
        }
    }

    /* set configuration for FSK channel */
    memset(&ifconf, 0, sizeof(ifconf)); /* initialize configuration structure */
    val = json_object_get_value(conf, "chan_FSK"); /* fetch value (if possible) */
    if (json_value_get_type(val) != JSONObject) {
        MSG("INFO: no configuration for FSK channel\n");
    } else {
        val = json_object_dotget_value(conf, "chan_FSK.enable");
        if (json_value_get_type(val) == JSONBoolean) {
            ifconf.enable = (bool)json_value_get_boolean(val);
        } else {
            ifconf.enable = false;
        }
        if (ifconf.enable == false) {
            MSG("INFO: FSK channel %i disabled\n", i);
        } else  {
            ifconf.rf_chain = (uint32_t)json_object_dotget_number(conf, "chan_FSK.radio");
            ifconf.freq_hz = (int32_t)json_object_dotget_number(conf, "chan_FSK.if");
            bw = (uint32_t)json_object_dotget_number(conf, "chan_FSK.bandwidth");
            if      (bw <= 7800)   ifconf.bandwidth = BW_7K8HZ;
            else if (bw <= 15600)  ifconf.bandwidth = BW_15K6HZ;
            else if (bw <= 31200)  ifconf.bandwidth = BW_31K2HZ;
            else if (bw <= 62500)  ifconf.bandwidth = BW_62K5HZ;
            else if (bw <= 125000) ifconf.bandwidth = BW_125KHZ;
            else if (bw <= 250000) ifconf.bandwidth = BW_250KHZ;
            else if (bw <= 500000) ifconf.bandwidth = BW_500KHZ;
            else ifconf.bandwidth = BW_UNDEFINED;
            ifconf.datarate = (uint32_t)json_object_dotget_number(conf, "chan_FSK.datarate");
            MSG("INFO: FSK channel enabled, radio %i selected, IF %i Hz, %u Hz bandwidth, %u bps datarate\n", ifconf.rf_chain, ifconf.freq_hz, bw, ifconf.datarate);
        }
        if (lgw_rxif_setconf(9, ifconf) != LGW_HAL_SUCCESS) {
            MSG("ERROR: invalid configuration for FSK channel\n");
            return -1;
        }
    }
    json_value_free(root_val);
    return 0;
}