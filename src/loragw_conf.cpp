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

const lgw_conf_rxif_s ifconf[10] {
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
        /* there is an object to configure that LoRa multi-SF channel, let's parse it */
        if (ifconf[i].enable == false) { /* LoRa multi-SF channel disabled, nothing else to parse */
            MSG("INFO: LoRa multi-SF channel %i disabled\n", i);
        } else  { /* LoRa multi-SF channel enabled, will parse the other parameters */
            // TODO: handle individual SF enabling and disabling (spread_factor)
            MSG("INFO: LoRa multi-SF channel %i enabled, radio %i selected, IF %i Hz, 125 kHz bandwidth, SF 7 to 12\n", i, ifconf[i].rf_chain, ifconf[i].freq_hz);
        }
        /* all parameters parsed, submitting configuration to the HAL */
        if (lgw_rxif_setconf(i, ifconf[i]) != LGW_HAL_SUCCESS) {
            MSG("ERROR: invalid configuration for Lora multi-SF channel %i\n", i);
            return -1;
        }
    }

    /* set configuration for LoRa standard channel */
    if (ifconf[8].enable == false) {
        MSG("INFO: LoRa standard channel %i disabled\n", i);
    } else  {
        switch(ifconf[8].bandwidth) {
            case BW_500KHZ: bw=500000; break;
            case BW_250KHZ: bw=250000; break;
            case BW_125KHZ: bw=125000; break;
            default: bw=0;
        }
        switch(ifconf[8].datarate) {
            case  DR_LORA_SF7: sf=7;  break;
            case  DR_LORA_SF8: sf=8;  break;
            case  DR_LORA_SF9: sf=9;  break;
            case  DR_LORA_SF10: sf=10; break;
            case  DR_LORA_SF11: sf=11; break;
            case  DR_LORA_SF12: sf=12; break;
            default: sf=0;
        }
        MSG("INFO: LoRa standard channel enabled, radio %i selected, IF %i Hz, %u Hz bandwidth, SF %u\n", ifconf[i].rf_chain, ifconf[i].freq_hz, bw, sf);
    }
    if (lgw_rxif_setconf(8, ifconf[8]) != LGW_HAL_SUCCESS) {
        MSG("ERROR: invalid configuration for Lora standard channel\n");
        return -1;
    }

    /* set configuration for FSK channel */
    if (ifconf[9].enable == false) {
        MSG("INFO: FSK channel %i disabled\n", i);
    } else  {
        switch(ifconf[9].bandwidth) {
            case  BW_7K8HZ: bw = 7800;  break;
            case  BW_15K6HZ: bw = 15600;  break;
            case  BW_31K2HZ: bw = 31200;  break;
            case  BW_62K5HZ: bw = 62500; break;
            case  BW_125KHZ: bw = 125000; break;
            case  BW_250KHZ: bw = 250000; break;
            case  BW_500KHZ: bw = 500000; break;
            default: bw = 0;

        MSG("INFO: FSK channel enabled, radio %i selected, IF %i Hz, %u Hz bandwidth, %u bps datarate\n", ifconf[9].rf_chain, ifconf[9].freq_hz, bw, ifconf[9].datarate);
    }
    if (lgw_rxif_setconf(9, ifconf[9]) != LGW_HAL_SUCCESS) {
        MSG("ERROR: invalid configuration for FSK channel\n");
        return -1;
    }
    return 0;
}