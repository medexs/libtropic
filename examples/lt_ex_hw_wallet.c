/**
 * @file TROPIC01_hw_wallet.c
 * @brief Example usage of TROPIC01 chip in a generic *hardware wallet* project.
 * @author Tropic Square s.r.o.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "string.h"
#include "inttypes.h"

#include "libtropic.h"
#include "libtropic_common.h"
#include "libtropic_examples.h"

/**
 * @name CONCEPT: Generic Hardware Wallet
 * @note We recommend reading TROPIC01's datasheet before diving into this example!
 * @par
    This code aims to show an example usage of TROPIC01 chip in a generic *hardware wallet* project.

    It is not focused on final application's usage, even though there is a few
    hints in session_H3() function.

    Instead of that, this code mainly walks you through a different stages during a chip's lifecycle:
    - Initial power up during PCB manufacturing
    - Attestation key uploading
    - Final product usage

    Example shows how content of config objects might be used to set different access rights and how chip behaves during the device's lifecycle.
 */

//-----------------------------------------------------------
#define TO_PAIRING_KEY_SH0PUB(x)     ((x) << 0)
#define TO_PAIRING_KEY_H1(x)     ((x) << 8)
#define TO_PAIRING_KEY_H2(x)     ((x) << 16)
#define TO_PAIRING_KEY_H3(x)     ((x) << 24)

#define TO_LT_MCOUNTER_0_3(x)    ((x) << 0)
#define TO_LT_MCOUNTER_4_7(x)    ((x) << 8)
#define TO_LT_MCOUNTER_8_11(x)   ((x) << 16)
#define TO_LT_MCOUNTER_12_15(x)  ((x) << 24)

#define TO_ECC_KEY_SLOT_0_7(x)   ((x) << 0)
#define TO_ECC_KEY_SLOT_8_15(x)  ((x) << 8)
#define TO_ECC_KEY_SLOT_16_23(x) ((x) << 16)
#define TO_ECC_KEY_SLOT_24_31(x) ((x) << 24)
//-----------------------------------------------------------
#define SESSION_SH0PUB_HAS_ACCESS    (uint8_t)0x01
#define SESSION_H1_HAS_ACCESS    (uint8_t)0x02
#define SESSION_H2_HAS_ACCESS    (uint8_t)0x04
#define SESSION_H3_HAS_ACCESS    (uint8_t)0x08
//-----------------------------------------------------------

struct lt_config_obj_desc_t {
    char desc[60];
    enum CONFIGURATION_OBJECTS_REGS addr;
};

/** @brief This structure is used glocally in this example to simplify looping
 *         through all config addresses and printing out them into debug */
static struct lt_config_obj_desc_t config_description[27] = {
    {"CONFIGURATION_OBJECTS_CFG_START_UP                   ", CONFIGURATION_OBJECTS_CFG_START_UP_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_SLEEP_MODE                 ", CONFIGURATION_OBJECTS_CFG_SLEEP_MODE_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_SENSORS                    ", CONFIGURATION_OBJECTS_CFG_SENSORS_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_DEBUG                      ", CONFIGURATION_OBJECTS_CFG_DEBUG_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_UAP_PAIRING_KEY_WRITE      ", CONFIGURATION_OBJECTS_CFG_UAP_PAIRING_KEY_WRITE_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_UAP_PAIRING_KEY_READ       ", CONFIGURATION_OBJECTS_CFG_UAP_PAIRING_KEY_READ_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_UAP_PAIRING_KEY_INVALIDATE ", CONFIGURATION_OBJECTS_CFG_UAP_PAIRING_KEY_INVALIDATE_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_UAP_R_CONFIG_WRITE_ERASE   ", CONFIGURATION_OBJECTS_CFG_UAP_R_CONFIG_WRITE_ERASE_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_UAP_R_CONFIG_READ          ", CONFIGURATION_OBJECTS_CFG_UAP_R_CONFIG_READ_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_UAP_I_CONFIG_WRITE         ", CONFIGURATION_OBJECTS_CFG_UAP_I_CONFIG_WRITE_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_UAP_I_CONFIG_READ          ", CONFIGURATION_OBJECTS_CFG_UAP_I_CONFIG_READ_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_UAP_PING                   ", CONFIGURATION_OBJECTS_CFG_UAP_PING_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_UAP_R_MEM_DATA_WRITE       ", CONFIGURATION_OBJECTS_CFG_UAP_R_MEM_DATA_WRITE_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_UAP_R_MEM_DATA_READ        ", CONFIGURATION_OBJECTS_CFG_UAP_R_MEM_DATA_READ_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_UAP_R_MEM_DATA_ERASE       ", CONFIGURATION_OBJECTS_CFG_UAP_R_MEM_DATA_ERASE_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_UAP_RANDOM_VALUE_GET       ", CONFIGURATION_OBJECTS_CFG_UAP_RANDOM_VALUE_GET_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_UAP_ECC_KEY_GENERATE       ", CONFIGURATION_OBJECTS_CFG_UAP_ECC_KEY_GENERATE_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_UAP_ECC_KEY_STORE          ", CONFIGURATION_OBJECTS_CFG_UAP_ECC_KEY_STORE_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_UAP_ECC_KEY_READ           ", CONFIGURATION_OBJECTS_CFG_UAP_ECC_KEY_READ_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_UAP_ECC_KEY_ERASE          ", CONFIGURATION_OBJECTS_CFG_UAP_ECC_KEY_ERASE_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_UAP_ECDSA_SIGN             ", CONFIGURATION_OBJECTS_CFG_UAP_ECDSA_SIGN_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_UAP_EDDSA_SIGN             ", CONFIGURATION_OBJECTS_CFG_UAP_EDDSA_SIGN_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_UAP_MCOUNTER_INIT          ", CONFIGURATION_OBJECTS_CFG_UAP_MCOUNTER_INIT_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_UAP_MCOUNTER_GET           ", CONFIGURATION_OBJECTS_CFG_UAP_MCOUNTER_GET_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_UAP_MCOUNTER_UPDATE        ", CONFIGURATION_OBJECTS_CFG_UAP_MCOUNTER_UPDATE_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_UAP_MAC_AND_DESTROY        ", CONFIGURATION_OBJECTS_CFG_UAP_MAC_AND_DESTROY_ADDR},
    {"CONFIGURATION_OBJECTS_CFG_UAP_SERIAL_CODE_GET        ", CONFIGURATION_OBJECTS_CFG_UAP_SERIAL_CODE_GET_ADDR}
};

/** @brief  User Conﬁguration Objects - values set here reflect the concept of a generic hardware wallet */
static uint32_t TROPIC01_config[27] = {
    //-------CONFIGURATION_OBJECTS_CFG_START_UP------------------------------------
    // Enable checks on boot
        (CONFIGURATION_OBJECTS_CFG_START_UP_MBIST_DIS_MASK |
         CONFIGURATION_OBJECTS_CFG_START_UP_RNGTEST_DIS_MASK |
         CONFIGURATION_OBJECTS_CFG_START_UP_MAINTENANCE_ENA_MASK |
         CONFIGURATION_OBJECTS_CFG_START_UP_CPU_FW_VERIFY_DIS_MASK |
         CONFIGURATION_OBJECTS_CFG_START_UP_SPECT_FW_VERIFY_DIS_MASK),
    //-------CONFIGURATION_OBJECTS_CFG_SLEEP_MODE----------------------------------
    // Enable sleep mode only
        (CONFIGURATION_OBJECTS_CFG_SLEEP_MODE_SLEEP_MODE_EN_MASK),
    //-------CONFIGURATION_OBJECTS_CFG_SENSORS-------------------------------------
    // Enable all sensors
        (CONFIGURATION_OBJECTS_CFG_SENSORS_PTRNG0_TEST_DIS_MASK |
         CONFIGURATION_OBJECTS_CFG_SENSORS_PTRNG1_TEST_DIS_MASK |
         CONFIGURATION_OBJECTS_CFG_SENSORS_OSCILLATOR_MON_DIS_MASK |
         CONFIGURATION_OBJECTS_CFG_SENSORS_SHIELD_DIS_MASK |
         CONFIGURATION_OBJECTS_CFG_SENSORS_VOLTAGE_MON_DIS_MASK |
         CONFIGURATION_OBJECTS_CFG_SENSORS_GLITCH_DET_DIS_MASK |
         CONFIGURATION_OBJECTS_CFG_SENSORS_TEMP_SENS_DIS_MASK |
         CONFIGURATION_OBJECTS_CFG_SENSORS_LASER_DET_DIS_MASK |
         CONFIGURATION_OBJECTS_CFG_SENSORS_EM_PULSE_DET_DIS_MASK |
         CONFIGURATION_OBJECTS_CFG_SENSORS_CPU_ALERT_DIS_MASK |
         CONFIGURATION_OBJECTS_CFG_SENSORS_PIN_VERIF_BIT_FLIP_DIS_MASK |
         CONFIGURATION_OBJECTS_CFG_SENSORS_SCB_BIT_FLIP_DIS_MASK |
         CONFIGURATION_OBJECTS_CFG_SENSORS_CPB_BIT_FLIP_DIS_MASK |
         CONFIGURATION_OBJECTS_CFG_SENSORS_ECC_BIT_FLIP_DIS_MASK |
         CONFIGURATION_OBJECTS_CFG_SENSORS_R_MEM_BIT_FLIP_DIS_MASK |
         CONFIGURATION_OBJECTS_CFG_SENSORS_EKDB_BIT_FLIP_DIS_MASK |
         CONFIGURATION_OBJECTS_CFG_SENSORS_I_MEM_BIT_FLIP_DIS_MASK |
         CONFIGURATION_OBJECTS_CFG_SENSORS_PLATFORM_BIT_FLIP_DIS_MASK),
    //------- CONFIGURATION_OBJECTS_CFG_DEBUG -------------------------------------
    // Enable TROPIC01's fw log
    (CONFIGURATION_OBJECTS_CFG_DEBUG_FW_LOG_EN_MASK),
    //------- CONFIGURATION_OBJECTS_CFG_UAP_PAIRING_KEY_WRITE ---------------------
    // No session can write pairing keys
    0,
    //------- CONFIGURATION_OBJECTS_CFG_UAP_PAIRING_KEY_READ ----------------------
    // All sessions can read pairing keys
        (TO_PAIRING_KEY_SH0PUB(SESSION_SH0PUB_HAS_ACCESS | SESSION_H1_HAS_ACCESS | SESSION_H2_HAS_ACCESS | SESSION_H3_HAS_ACCESS) |
         TO_PAIRING_KEY_H1(SESSION_SH0PUB_HAS_ACCESS | SESSION_H1_HAS_ACCESS | SESSION_H2_HAS_ACCESS | SESSION_H3_HAS_ACCESS) |
         TO_PAIRING_KEY_H2(SESSION_SH0PUB_HAS_ACCESS | SESSION_H1_HAS_ACCESS | SESSION_H2_HAS_ACCESS | SESSION_H3_HAS_ACCESS) |
         TO_PAIRING_KEY_H3(SESSION_SH0PUB_HAS_ACCESS | SESSION_H1_HAS_ACCESS | SESSION_H2_HAS_ACCESS | SESSION_H3_HAS_ACCESS)),
    //------- CONFIGURATION_OBJECTS_CFG_UAP_PAIRING_KEY_INVALIDATE ----------------
    // Pairing key SH0PUB can be invalidated only from session SH0PUB
    // H3 session can invalidate pairing key H1 H2 and H3
        (TO_PAIRING_KEY_SH0PUB(SESSION_SH0PUB_HAS_ACCESS ) |
         TO_PAIRING_KEY_H1(SESSION_H3_HAS_ACCESS ) |
         TO_PAIRING_KEY_H2(SESSION_H3_HAS_ACCESS ) |
         TO_PAIRING_KEY_H3(SESSION_H3_HAS_ACCESS)),
    //------- CONFIGURATION_OBJECTS_CFG_UAP_R_CONFIG_WRITE_ERASE ------------------
    // Reset value, not used currently
        0x000000ff,
    //------- CONFIGURATION_OBJECTS_CFG_UAP_R_CONFIG_READ -------------------------
    // Reset value, not used currently
        0x0000ffff,
    //------- CONFIGURATION_OBJECTS_CFG_UAP_I_CONFIG_WRITE ------------------------
    // Reset value, not used currently
        0x0000ffff,
    //------- CONFIGURATION_OBJECTS_CFG_UAP_I_CONFIG_READ -------------------------
    // Reset value, not used currently
        0x0000ffff,
    //------- CONFIGURATION_OBJECTS_CFG_UAP_PING ----------------------------------
    // Ping command is available for all session keys
        (SESSION_SH0PUB_HAS_ACCESS | SESSION_H1_HAS_ACCESS | SESSION_H2_HAS_ACCESS | SESSION_H3_HAS_ACCESS),
    //------- CONFIGURATION_OBJECTS_CFG_UAP_R_MEM_DATA_WRITE ----------------------
    // Reset value, not used currently
        0xffffffff,
    //------- CONFIGURATION_OBJECTS_CFG_UAP_R_MEM_DATA_READ -----------------------
    // Reset value, not used currently
        0xffffffff,
    //------- CONFIGURATION_OBJECTS_CFG_UAP_R_MEM_DATA_ERASE ----------------------
    // Reset value, not used currently
        0xffffffff,
    //------- CONFIGURATION_OBJECTS_CFG_UAP_RANDOM_VALUE_GET ----------------------
        (SESSION_SH0PUB_HAS_ACCESS | SESSION_H1_HAS_ACCESS | SESSION_H2_HAS_ACCESS | SESSION_H3_HAS_ACCESS),
    //------- CONFIGURATION_OBJECTS_CFG_UAP_ECC_KEY_GENERATE ----------------------
    // No session can generate key in ecc key slot 0-7
    // H3 can generate key in slots 8-31
        (TO_ECC_KEY_SLOT_0_7(0)                  |
         TO_ECC_KEY_SLOT_8_15(SESSION_H3_HAS_ACCESS)  |
         TO_ECC_KEY_SLOT_16_23(SESSION_H3_HAS_ACCESS) |
         TO_ECC_KEY_SLOT_24_31(SESSION_H3_HAS_ACCESS)),
    //------- CONFIGURATION_OBJECTS_CFG_UAP_ECC_KEY_STORE -------------------------
    // H1 can store key into ecc key slot 0-7
    // H3 can store key into ecc key slot 8-31
        (TO_ECC_KEY_SLOT_0_7(SESSION_H1_HAS_ACCESS)   |
         TO_ECC_KEY_SLOT_8_15(SESSION_H3_HAS_ACCESS)  |
         TO_ECC_KEY_SLOT_16_23(SESSION_H3_HAS_ACCESS) |
         TO_ECC_KEY_SLOT_24_31(SESSION_H3_HAS_ACCESS)),
    //------- CONFIGURATION_OBJECTS_CFG_UAP_ECC_KEY_READ --------------------------
    // All keys can read pubkey
        (TO_ECC_KEY_SLOT_0_7(  SESSION_SH0PUB_HAS_ACCESS | SESSION_H1_HAS_ACCESS | SESSION_H2_HAS_ACCESS | SESSION_H3_HAS_ACCESS) |
         TO_ECC_KEY_SLOT_8_15( SESSION_SH0PUB_HAS_ACCESS | SESSION_H1_HAS_ACCESS | SESSION_H2_HAS_ACCESS | SESSION_H3_HAS_ACCESS) |
         TO_ECC_KEY_SLOT_16_23(SESSION_SH0PUB_HAS_ACCESS | SESSION_H1_HAS_ACCESS | SESSION_H2_HAS_ACCESS | SESSION_H3_HAS_ACCESS) |
         TO_ECC_KEY_SLOT_24_31(SESSION_SH0PUB_HAS_ACCESS | SESSION_H1_HAS_ACCESS | SESSION_H2_HAS_ACCESS | SESSION_H3_HAS_ACCESS)),
    //------- CONFIGURATION_OBJECTS_CFG_UAP_ECC_KEY_ERASE -------------------------
    // H1 key can erase keys in slot 0-7
    // H3 key can erase key in slot  8-31
         (TO_ECC_KEY_SLOT_0_7(SESSION_H1_HAS_ACCESS)  |
         TO_ECC_KEY_SLOT_8_15(SESSION_H3_HAS_ACCESS)  |
         TO_ECC_KEY_SLOT_16_23(SESSION_H3_HAS_ACCESS) |
         TO_ECC_KEY_SLOT_24_31(SESSION_H3_HAS_ACCESS)),
    //------- CONFIGURATION_OBJECTS_CFG_UAP_ECDSA_SIGN ----------------------------
    // H3 can sign with all ECDSA key slots
        (TO_ECC_KEY_SLOT_0_7(  SESSION_H3_HAS_ACCESS) |
         TO_ECC_KEY_SLOT_8_15( SESSION_H3_HAS_ACCESS) |
         TO_ECC_KEY_SLOT_16_23(SESSION_H3_HAS_ACCESS) |
         TO_ECC_KEY_SLOT_24_31(SESSION_H3_HAS_ACCESS)),
    //------- CONFIGURATION_OBJECTS_CFG_UAP_EDDSA_SIGN ----------------------------
    // H3 can sign with all ECC key slots
        (TO_ECC_KEY_SLOT_0_7(  SESSION_H3_HAS_ACCESS) |
         TO_ECC_KEY_SLOT_8_15( SESSION_H3_HAS_ACCESS) |
         TO_ECC_KEY_SLOT_16_23(SESSION_H3_HAS_ACCESS) |
         TO_ECC_KEY_SLOT_24_31(SESSION_H3_HAS_ACCESS)),
    //------- CONFIGURATION_OBJECTS_CFG_UAP_MCOUNTER_INIT -------------------------
    // H3 can init all counters
        (TO_LT_MCOUNTER_0_3(  SESSION_H3_HAS_ACCESS) |
         TO_LT_MCOUNTER_4_7( SESSION_H3_HAS_ACCESS)  |
         TO_LT_MCOUNTER_8_11(SESSION_H3_HAS_ACCESS)  |
         TO_LT_MCOUNTER_12_15(SESSION_H3_HAS_ACCESS)),
    //------- CONFIGURATION_OBJECTS_CFG_UAP_MCOUNTER_GET --------------------------
    // H3 can get all counters
        (TO_LT_MCOUNTER_0_3(  SESSION_H3_HAS_ACCESS) |
         TO_LT_MCOUNTER_4_7( SESSION_H3_HAS_ACCESS)  |
         TO_LT_MCOUNTER_8_11(SESSION_H3_HAS_ACCESS)  |
         TO_LT_MCOUNTER_12_15(SESSION_H3_HAS_ACCESS)),
    //------- CONFIGURATION_OBJECTS_CFG_UAP_MCOUNTER_UPDATE -----------------------
    // H3 can update all counters
        (TO_LT_MCOUNTER_0_3(  SESSION_H3_HAS_ACCESS) |
         TO_LT_MCOUNTER_4_7( SESSION_H3_HAS_ACCESS)  |
         TO_LT_MCOUNTER_8_11(SESSION_H3_HAS_ACCESS)  |
         TO_LT_MCOUNTER_12_15(SESSION_H3_HAS_ACCESS)),
    //------- CONFIGURATION_OBJECTS_TODO -----------------------
    // All slots can use MAC-and-destroy
        (0xffffffff),
    //------- CONFIGURATION_OBJECTS_CFG_UAP_SERIAL_CODE_GET -----------------------
        (0),
};

/**
 * @brief This function establish a secure channel between host MCU and TROPIC01 chip
 *
 * @param h           Device's handle
 * @param shipriv     Host's private pairing key (SHiPUB)
 * @param shipub      Host's public pairing key  (SHiPUB)
 * @param pkey_index  Pairing key's index
 * @return            LT_OK if success, otherwise returns other error code.
 */
static lt_ret_t verify_chip_and_start_secure_session(lt_handle_t *h, uint8_t *shipriv, uint8_t *shipub, uint8_t pkey_index)
{
    lt_ret_t ret = LT_FAIL;

    // This is not used in this example, but let's read it anyway
    uint8_t chip_id[LT_L2_GET_INFO_CHIP_ID_SIZE] = {0};
    ret = lt_get_info_chip_id(h, chip_id, LT_L2_GET_INFO_CHIP_ID_SIZE);
    if (ret != LT_OK) {
        return ret;
    }

    // This is not used in this example, but let's read it anyway
    uint8_t riscv_fw_ver[LT_L2_GET_INFO_RISCV_FW_SIZE] = {0};
    ret = lt_get_info_riscv_fw_ver(h, riscv_fw_ver, LT_L2_GET_INFO_RISCV_FW_SIZE);
    if (ret != LT_OK) {
        return ret;
    }

    // This is not used in this example, but let's read it anyway
    uint8_t spect_fw_ver[LT_L2_GET_INFO_SPECT_FW_SIZE] = {0};
    ret = lt_get_info_spect_fw_ver(h, spect_fw_ver, LT_L2_GET_INFO_SPECT_FW_SIZE);
    if (ret != LT_OK) {
        return ret;
    }

    uint8_t X509_cert[LT_L2_GET_INFO_REQ_CERT_SIZE] = {0};
    ret = lt_get_info_cert(h, X509_cert, LT_L2_GET_INFO_REQ_CERT_SIZE);
    if (ret != LT_OK) {
        return ret;
    }

    uint8_t stpub[32] = {0};
    ret = lt_cert_verify_and_parse(X509_cert, LT_L2_GET_INFO_REQ_CERT_SIZE, stpub);
    if (ret != LT_OK) {
        return ret;
    }

    ret = lt_session_start(h, stpub, pkey_index, shipriv, shipub);
    if (ret != LT_OK) {
        return ret;
    }

    return LT_OK;
}

/**
 * @brief This function reads config objects and print them out
 *
 * @param h           Device's handle
 * @return            LT_OK if success, otherwise returns other error code.
 */
/*
static lt_ret_t read_whole_I_config(lt_handle_t *h)
{
    lt_ret_t ret;

    LT_LOG("");
    for (int i=0; i<27;i++) {
        uint32_t check = 0;
        ret = lt_i_config_read(h, config_description[i].addr, &check);
        if(ret != LT_OK) {
            return ret;
        }
        LT_LOG("\t\t%s  %08" PRIX32, config_description[i].desc, check);
    }
    LT_LOG("");

    return LT_OK;
}
*/
/**
 * @brief This function reads config objects and print them out
 *
 * @param h           Device's handle
 * @return            LT_OK if success, otherwise returns other error code.
 */
static lt_ret_t read_whole_R_config(lt_handle_t *h)
{
    lt_ret_t ret;

    LT_LOG("");
    for (int i=0; i<27;i++) {
        uint32_t check = 0;
        ret = lt_r_config_read(h, config_description[i].addr, &check);
        if(ret != LT_OK) {
            return ret;
        }
        LT_LOG("\t\t%s  %08" PRIX32, config_description[i].desc, check);
    }
    LT_LOG("");

    return LT_OK;
}

/**
 * @brief This function writes config objecs of TROPIC01
 *
 * @param h           Device's handle
 * @return            LT_OK if success, otherwise returns other error code.
 */
static lt_ret_t write_whole_R_config(lt_handle_t *h)
{
    lt_ret_t ret;

    for(int i=0; i<27;i++) {
        ret = lt_r_config_write(h, config_description[i].addr, TROPIC01_config[i]);
        if(ret != LT_OK) {
            return ret;
        }
    }

    return LT_OK;
}

/**
 * @brief Initial session, when chip is powered for the first time during manufacturing.
 *        This function writes chip's configuration into r config area.
 *
 * @param h           Device's handle
 * @return            0 if success, otherwise -1
 */
static int session_initial(void)
{
    lt_handle_t h = {0};

    lt_init(&h);

    // Establish secure handshake with default H0 pairing keys
    LT_LOG("%s", "Creating session with initial factory keys H0");
    LT_ASSERT(LT_OK, verify_chip_and_start_secure_session(&h, sh0priv, sh0pub, pkey_index_0));
    //LT_LOG("%s", "TODO: show I config values");
    //LT_LOG("%s", "R CONFIG read:");
    //LT_ASSERT(LT_OK, read_whole_R_config(h));
    LT_LOG("%s", "Writing whole R config");
    LT_ASSERT(LT_OK, write_whole_R_config(&h));
    LT_LOG("%s", "Reading R CONFIG again");
    read_whole_R_config(&h);

    // Writing pairing keys 1-3
    LT_LOG("%s", "Writing pairing key H1");
    LT_ASSERT(LT_OK, lt_pairing_key_write(&h, sh1pub, pkey_index_1));
    LT_LOG("%s", "Writing pairing key H2");
    LT_ASSERT(LT_OK, lt_pairing_key_write(&h, sh2pub, pkey_index_2));
    LT_LOG("%s", "Writing pairing key H3");
    LT_ASSERT(LT_OK, lt_pairing_key_write(&h, sh3pub, pkey_index_3));
    LT_LOG("%s", "Invalidating SH0PUB pairing key");
    LT_ASSERT(LT_OK, lt_pairing_key_invalidate(&h, pkey_index_0));

    LT_LOG("%s", "Closing session H0");
    LT_ASSERT(LT_OK, lt_session_abort(&h));

    LT_LOG("%s", "Rebooting TROPIC01");
    LT_ASSERT(LT_OK, lt_reboot(&h, LT_L2_STARTUP_ID_REBOOT));

    lt_deinit(&h);

    return 0;
}

/**
 * @brief Session with H0 pairing keys
 *
 * @param h           Device's handle
 * @return            0 if success, otherwise -1
 */
static int session_H0(void)
{
    lt_handle_t h = {0};

    lt_init(&h);

    /* Establish secure handshake with default H0 pairing keys should fail, because this key was invalidated during session_initial() */
    LT_LOG("%s", "Establish session with H0 must fail");
    LT_ASSERT(LT_L2_HSK_ERR, verify_chip_and_start_secure_session(&h, sh0priv, sh0pub, pkey_index_0));

    lt_deinit(&h);

    return 0;
}

/**
 * @brief Session with H1 pairing keys
 *
 * @param h           Device's handle
 * @return            0 if success, otherwise -1
 */
static int session_H1(void)
{
    lt_handle_t h = {0};

    lt_init(&h);

    /* Establish secure handshake with default H1 pairing keys */
    LT_LOG("%s", "Creating session with H1 keys");
    LT_ASSERT(LT_OK, verify_chip_and_start_secure_session(&h, sh1priv, sh1pub, pkey_index_1));

    uint8_t in[100];
    uint8_t out[100];
    LT_LOG("%s", "lt_ping() ");
    LT_ASSERT(LT_OK, lt_ping(&h, out, in, 100));

    //LT_LOG("%s", "Macand: ");
    //uint8_t data_in[32] = {0};
    //uint8_t data_out[32] = {0};
    //memset(data_out, 0xab, 32);
    //LT_ASSERT(LT_OK, lt_mac_and_destroy(&h, MAC_AND_DESTROY_SLOT_0, data_out, data_in));

    uint8_t attestation_key[32] = {0x22,0x57,0xa8,0x2f,0x85,0x8f,0x13,0x32,0xfa,0x0f,0xf6,0x0c,0x76,0x29,0x42,0x70,0xa9,0x58,0x9d,0xfd,0x47,0xa5,0x23,0x78,0x18,0x4d,0x2d,0x38,0xf0,0xa7,0xc4,0x01};
    LT_LOG("%s", "Storing attestation key into slot 0");
    LT_ASSERT(LT_OK, lt_ecc_key_store(&h, ECC_SLOT_0, CURVE_ED25519, attestation_key));

    uint8_t serial_code[SERIAL_CODE_SIZE] = {0};
    LT_LOG("%s","Serial code get must fail");
    LT_ASSERT(LT_L3_UNAUTHORIZED, lt_serial_code_get(&h, serial_code, SERIAL_CODE_SIZE));

    uint8_t dummykey[32];
    LT_LOG("%s","Pairing key write into slot 0 must fail");
    LT_ASSERT(LT_L3_UNAUTHORIZED, lt_pairing_key_write(&h, dummykey, PAIRING_KEY_SLOT_INDEX_0));
    LT_LOG("%s","Pairing key write into slot 1 must fail");
    LT_ASSERT(LT_L3_UNAUTHORIZED, lt_pairing_key_write(&h, dummykey, PAIRING_KEY_SLOT_INDEX_1));
    LT_LOG("%s","Pairing key write into slot 2 must fail");
    LT_ASSERT(LT_L3_UNAUTHORIZED, lt_pairing_key_write(&h, dummykey, PAIRING_KEY_SLOT_INDEX_2));
    LT_LOG("%s","Pairing key write into slot 3 must fail");
    LT_ASSERT(LT_L3_UNAUTHORIZED, lt_pairing_key_write(&h, dummykey, PAIRING_KEY_SLOT_INDEX_3));


    LT_LOG("%s", "Aborting session H1");
    LT_ASSERT(LT_OK, lt_session_abort(&h));

    lt_deinit(&h);

    return LT_OK;
}

/**
 * @brief Session with H2 pairing keys
 *
 * @param h           Device's handle
 * @return            0 if success, otherwise -1
 */
static int session_H2(void)
{
    lt_handle_t h = {0};

    lt_init(&h);

    LT_LOG("%s", "Creating session with H2 keys");
    LT_ASSERT(LT_OK, verify_chip_and_start_secure_session(&h, sh2priv, sh2pub, pkey_index_2));

    uint8_t in[100];
    uint8_t out[100];
    LT_LOG("%s", "lt_ping() ");
    LT_ASSERT(LT_OK, lt_ping(&h, out, in, 100));

    //LT_LOG("%s", "Macand: ");
    //uint8_t data_in[32] = {0};
    //uint8_t data_out[32] = {0};
    //memset(data_out, 0xab, 32);
    //LT_ASSERT(LT_OK, lt_mac_and_destroy(&h, MAC_AND_DESTROY_SLOT_0, data_out, data_in));

    uint8_t serial_code[SERIAL_CODE_SIZE] = {0};
    LT_LOG("%s","Serial code get must fail");
    LT_ASSERT(LT_L3_UNAUTHORIZED, lt_serial_code_get(&h, serial_code, SERIAL_CODE_SIZE));

    uint8_t dummykey[32];
    LT_LOG("%s","ECC key store into slot 1 must fail");
    LT_ASSERT(LT_L3_UNAUTHORIZED, lt_ecc_key_store(&h, ECC_SLOT_0, CURVE_ED25519, dummykey));

    LT_LOG("%s","Pairing key write into slot 0 must fail");
    LT_ASSERT(LT_L3_UNAUTHORIZED, lt_pairing_key_write(&h, dummykey, PAIRING_KEY_SLOT_INDEX_0));
    LT_LOG("%s","Pairing key write into slot 1 must fail");
    LT_ASSERT(LT_L3_UNAUTHORIZED, lt_pairing_key_write(&h, dummykey, PAIRING_KEY_SLOT_INDEX_1));
    LT_LOG("%s","Pairing key write into slot 2 must fail");
    LT_ASSERT(LT_L3_UNAUTHORIZED, lt_pairing_key_write(&h, dummykey, PAIRING_KEY_SLOT_INDEX_2));
    LT_LOG("%s","Pairing key write into slot 3 must fail");
    LT_ASSERT(LT_L3_UNAUTHORIZED, lt_pairing_key_write(&h, dummykey, PAIRING_KEY_SLOT_INDEX_3));

    uint32_t mcounter_value = 0x000000ff;
    LT_LOG("%s","Initializing mcounter 0 must fail");
    LT_ASSERT(LT_L3_UNAUTHORIZED, lt_mcounter_init(&h,  0, mcounter_value));
    LT_LOG("%s","Mcounter 0 update must fail");
    LT_ASSERT(LT_L3_UNAUTHORIZED, lt_mcounter_update(&h, 0));
    LT_LOG("%s","Mcounter get must fail");
    LT_ASSERT(LT_L3_UNAUTHORIZED, lt_mcounter_get(&h, 0, &mcounter_value));


    // TODO Unauthorized sign with attestation key

    LT_LOG("%s", "Aborting session H2");
    LT_ASSERT(LT_OK, lt_session_abort(&h));

    lt_deinit(&h);

    return LT_OK;
}

/**
 * @brief Session with H3 pairing keys
 *
 * @param h           Device's handle
 * @return            0 if success, otherwise -1
 */
static int session_H3(void)
{
    lt_handle_t h = {0};

    lt_init(&h);

    LT_LOG("%s", "Creating session with H3 keys");
    LT_ASSERT(LT_OK, verify_chip_and_start_secure_session(&h, sh3priv, sh3pub, pkey_index_3));

    // Following commands must NOT pass, check if they return LT_L3_UNAUTHORIZED

    LT_LOG("%s", "lt_ping() ");
    uint8_t in[100];
    uint8_t out[100];
    LT_ASSERT(LT_OK, lt_ping(&h, out, in, 100));

    //LT_LOG("%s", "Macand: ");
    //uint8_t data_in[32] = {0};
    //uint8_t data_out[32] = {0};
    //memset(data_out, 0xab, 32);
    //LT_ASSERT(LT_OK, lt_mac_and_destroy(&h, MAC_AND_DESTROY_SLOT_0, data_out, data_in));

    // Sign with attestation key which was updated through session keys H1
    LT_LOG("%s", "lt_ecc_eddsa_sign() ");
    uint8_t msg[] = {'a', 'h', 'o', 'j'};
    uint8_t rs[64];
    LT_ASSERT(LT_OK, lt_ecc_eddsa_sign(&h, ECC_SLOT_0, msg, 4, rs, 64));

    LT_LOG("%s", "lt_ecc_key_read() ");
    uint8_t slot_0_pubkey[64];
    lt_ecc_curve_type_t curve;
    ecc_key_origin_t origin;
    LT_ASSERT(LT_OK, lt_ecc_key_read(&h, ECC_SLOT_0, slot_0_pubkey, 64, &curve, &origin));

    LT_LOG("%s", "lt_ecc_eddsa_sig_verify() ");
    LT_ASSERT(LT_OK, lt_ecc_eddsa_sig_verify(msg, 4, slot_0_pubkey, rs));

    // TODO generate key
    LT_LOG("%s", "lt_ecc_key_generate() in ECC_SLOT_8");
    LT_ASSERT(LT_OK, lt_ecc_key_generate(&h, ECC_SLOT_8, CURVE_ED25519));
    LT_LOG("%s", "lt_ecc_key_generate() in ECC_SLOT_16");
    LT_ASSERT(LT_OK, lt_ecc_key_generate(&h, ECC_SLOT_16, CURVE_ED25519));
    LT_LOG("%s", "lt_ecc_key_generate() in ECC_SLOT_24");
    LT_ASSERT(LT_OK, lt_ecc_key_generate(&h, ECC_SLOT_24, CURVE_ED25519));

    LT_LOG("%s", "lt_random_get() RANDOM_VALUE_GET_LEN_MAX == 255");
    uint8_t buff[RANDOM_VALUE_GET_LEN_MAX];
    LT_ASSERT(LT_OK, lt_random_get(&h, buff, RANDOM_VALUE_GET_LEN_MAX));

    // TODO write r mem data
    LT_LOG("%s", "lt_r_mem_data_erase() slot 0 ");
    LT_ASSERT(LT_OK, lt_r_mem_data_erase(&h, 0));
    // TODO read r mem data

    uint32_t mcounter_value = 0x000000ff;
    LT_LOG_VALUE("mcounter_value %08" PRIX32, mcounter_value);
    LT_LOG("%s","Initializing mcounter 0 ");
    LT_ASSERT(LT_OK, lt_mcounter_init(&h,  0, mcounter_value));
    LT_LOG("%s","Mcounter 0 update");
    LT_ASSERT(LT_OK, lt_mcounter_update(&h, 0));
    LT_LOG("%s","Mcounter get ");
    LT_ASSERT(LT_OK, lt_mcounter_get(&h, 0, &mcounter_value));
    LT_LOG_VALUE("mcounter_value %08" PRIX32, mcounter_value);

    // Following commands must NOT pass, check if they return LT_L3_UNAUTHORIZED
    uint8_t serial_code[SERIAL_CODE_SIZE] = {0};
    LT_LOG("%s","Serial code get must fail");
    LT_ASSERT(LT_L3_UNAUTHORIZED, lt_serial_code_get(&h, serial_code, SERIAL_CODE_SIZE));

    uint8_t dummykey[32];
    LT_LOG("%s","ECC key store into slot 1 must fail");
    LT_ASSERT(LT_L3_UNAUTHORIZED, lt_ecc_key_store(&h, ECC_SLOT_0, CURVE_ED25519, dummykey));

    LT_LOG("%s","Pairing key write into slot 0 must fail");
    LT_ASSERT(LT_L3_UNAUTHORIZED, lt_pairing_key_write(&h, dummykey, PAIRING_KEY_SLOT_INDEX_0));
    LT_LOG("%s","Pairing key write into slot 1 must fail");
    LT_ASSERT(LT_L3_UNAUTHORIZED, lt_pairing_key_write(&h, dummykey, PAIRING_KEY_SLOT_INDEX_1));
    LT_LOG("%s","Pairing key write into slot 2 must fail");
    LT_ASSERT(LT_L3_UNAUTHORIZED, lt_pairing_key_write(&h, dummykey, PAIRING_KEY_SLOT_INDEX_2));
    LT_LOG("%s","Pairing key write into slot 3 must fail");
    LT_ASSERT(LT_L3_UNAUTHORIZED, lt_pairing_key_write(&h, dummykey, PAIRING_KEY_SLOT_INDEX_3));

    LT_LOG("%s", "Aborting session H3");
    LT_ASSERT(LT_OK, lt_session_abort(&h));

    lt_deinit(&h);

    return LT_OK;
}

int lt_ex_hardware_wallet(void)
{
    LT_LOG("");
    LT_LOG("\t=======================================================================");
    LT_LOG("\t=====  TROPIC01 example 1 - Hardware Wallet                         ===");
    LT_LOG("\t=======================================================================");

    LT_LOG_LINE();
    LT_LOG("\t Session initial:");
    if(session_initial() == -1) {
        LT_LOG("Error during session_initial()");
        return -1;
    }

    LT_LOG_LINE();
    LT_LOG("\t Session H0:");
    if(session_H0() == -1)  {
        LT_LOG("Error during session_H0()");
        return -1;
    }
    LT_LOG_LINE();
    LT_LOG("\t Session H1:");
    if(session_H1() == -1) {
        LT_LOG("Error during session_H1()");
        return -1;
    }
    LT_LOG_LINE();
    LT_LOG("\t Session H2:");
    if(session_H2() == -1) {
        LT_LOG("Error during session_H2()");
        return -1;
    }
    LT_LOG_LINE();
    LT_LOG("\t Session H3:");
    if(session_H3() == -1) {
        LT_LOG("Error during session_H3()");
        return -1;
    }
    LT_LOG_LINE();

    LT_LOG("\t End of execution, no errors.");

    return 0;
}
