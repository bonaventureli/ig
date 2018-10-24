/*
 * hsm_apdu.h
 *
 *  Created on: 2018年7月12日
 *      Author: zhenchiw
 */

#ifndef HSM_APDU_H_
#define HSM_APDU_H_

#define HSM_APDU

#if defined(HSM_APDU)
/************************************************************************************
*************************************************************************************
* Includes
*************************************************************************************
************************************************************************************/
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif /*_cplusplus*/

/**********************************************************************************************************************
 * Definitions
 *********************************************************************************************************************/
typedef int32_t status_t;

#define CHECKSUM_LRC 	0x00
#define CHECKSUM_CRC 	0x01

#define T1_NAD  		0x21

/* For APDU CMD Param (P1, P2) config */
typedef enum
{
	/*APDU Param1_u config*/
	INGEEK_VCK_BUSINESS	 = 0,
    INGEEK_ENCRYPT	 	 = 0,
	INGEEK_DECRYPT	 	 = 1,
    INGEEK_ADD_BLIST 	 = 0,
	INGEEK_DELE_BLIST	 = 1,
	INGEEK_READ_BLIST	 = 2,
	INGEEK_OFFSET_BLIST  = 0,
	INGEEK_CONTINU_BLIST = 1,
	INGEEK_SEID_DATA 	 = 0x50,
	INGEEK_VIN_DATA  	 = 0x51,

	/*APDU Param2_u config*/
	INGEEK_MORE_DATA 	 = 0x00,
	INGEEK_LAST_DATA 	 = 0x80,
	INGEEK_AES_CBC_ISO9797_M2 = 0x00,
	INGEEK_ECDSA_SHA_256 = 0x00,
}Apdu_config_e;


typedef enum
{
	APDU_SELECT,
	APDU_DATA,
	APDU_SESSION_INIT,
	APDU_SESSION_AUTH,
	APDU_CMD,
	APDU_MSG,
	APDU_VERFIY_PERM,
	APDU_CAR_BIND,
	APDU_BLACKLIST,
	APDU_CIPHER,
	APDU_SIGNATURE,
	APDU_SIGNATURE_VERFIY,
}Apdu_type_e;


typedef struct
{
	uint16_t  status;
	uint32_t  resp_len;
	uint8_t*  Apdu_resp;
}Se_Resp_t;

/************************************************************************************
*************************************************************************************
* Public prototypes
*************************************************************************************
************************************************************************************/
extern  status_t ingeek_hsm_init(void);

extern  status_t ingeek_hsm_apdu(Apdu_type_e apdu_type, uint8_t* input, uint32_t ilen, uint8_t P1, uint8_t P2, Se_Resp_t* apdu_resp);

#if defined(__cplusplus)
}
#endif /*_cplusplus*/

#endif /*HSM_APDU*/

#endif /*HSM_APDU_H_*/
