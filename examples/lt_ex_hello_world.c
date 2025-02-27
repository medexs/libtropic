/**
 * @file TROPIC01_hw_wallet.c
 * @brief Example usage of TROPIC01 chip in a generic *hardware wallet* project.
 * @author Tropic Square s.r.o.
 *
 * @license For the license see file LICENSE.txt file in the root directory of this source tree.
 */

#include "string.h"

#include "libtropic.h"
#include "libtropic_common.h"
#include "libtropic_examples.h"

/**
 * @name Hello World
 * @note We recommend reading TROPIC01's datasheet before diving into this example!
 * @par
 */

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
 * @brief Session with H0 pairing keys
 *
 * @param h           Device's handle
 * @return            0 if success, otherwise -1
 */
static int session_H0(void)
{
    lt_handle_t h = {0};

    lt_init(&h);

    LT_LOG("%s", "Establish session with H0");

    LT_ASSERT(LT_OK, verify_chip_and_start_secure_session(&h, sh0priv, sh0pub, pkey_index_0));

    uint8_t in[100] = {0};
    uint8_t out[100] = {0};
    memcpy(out, "This is Hello World message from TROPIC01!!", 43);
    LT_LOG("%s", "lt_ping() ");
    LT_ASSERT(LT_OK, lt_ping(&h, out, in, 43));
    LT_LOG("\t\tMessage: %s", in);

    lt_deinit(&h);

    return 0;
}

int lt_ex_hello_world(void)
{
    LT_LOG("");
    LT_LOG("\t=======================================================================");
    LT_LOG("\t=====  TROPIC01 Hello World                                         ===");
    LT_LOG("\t=======================================================================");


    LT_LOG_LINE();
    LT_LOG("\t Session with H0 keys:");
    if(session_H0() == -1)  {
        LT_LOG("Error during session_H0()");
    }

    LT_LOG_LINE();

    LT_LOG("\t End of execution, no errors.");

    return 0;
}
