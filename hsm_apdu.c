/*
#include <spihal.h>
 * hsm_apdu.c
 *
 *  Created on: 2018年7月12日
 *      Author: zhenchiw
 */

/************************************************************************************
 *************************************************************************************
 * Include
 *************************************************************************************
 ************************************************************************************/
#include <string.h>
#include <stdlib.h>
#include "spihal.h"
#include "7816_T1.h"
#include "hsm_apdu.h"
#include "digital_key_api.h"

#if defined(HSM_APDU)
/************************************************************************************
 *************************************************************************************
 * Private macros
 *************************************************************************************
 ************************************************************************************/
#define AIDLEN				0x0B
#define POWER_ON_ATP_LEN   	0x26
#define CMD_APDU_SIZE(lc)  	((lc) + 4)
#define APDU_BUF_SIZE  		(4+1+256+1)
/************************************************************************************
 *************************************************************************************
 * Private  definitions
 *************************************************************************************
 ************************************************************************************/
static t1_state_t t1;
static uint8_t Apdu_buff[APDU_BUF_SIZE];
static int16_t Apdulen=0;
static uint8_t Tblock[T1_BUFFER_SIZE];
static int32_t Tblock_len=0;
static uint8_t SE_ATP[POWER_ON_ATP_LEN];
static uint8_t gLockFlg = 0;

enum{CLA = 0,INS = 1,P1 = 2,P2 = 3,LC = 4,CDATA = 5,OFF_LE = 0};
enum{ATPLEN = 0,CHECKTYPE=12,IFSC = 13,CHECKSUM = 36};

static const uint8_t  AID[AIDLEN] = {0xA0, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x49, 0x43, 0x41, 0x53};
/************************************************************************************
*************************************************************************************
*Functions
*************************************************************************************
************************************************************************************/
static void ingeek_zeroize( void *v, uint32_t n )
{
    volatile uint8_t *p = (uint8_t*)v; while( n-- ) *p++ = 0;
}


status_t ingeek_hsm_init(void)
{
	rv_t Ret=0;
	uint8_t csum[2]={0};
	uint32_t sumlen=0;

	t1_init(&t1);

	Apdu_Para_t_clean();

	HSM_Init(SPI_Hsm_Task, TASK_TIME_CYCLE, NULL); //每隔10s轮询一次，如果安全芯片返回了数据就接收一下。

	Ret = HSM_AnswerToPowerOn(SE_ATP, SE_ATP, POWER_ON_ATP_LEN);
    if(RV_SUCCESS != Ret || SE_ATP[ATPLEN] != 0x25)
    {
    	//DEBUG_PRINTF("HSM ATP Failed\n");
    	return HsmStatus_ATPFail;
    }

    //DEBUG_PRINTF("HSM ATP SUCCESS !\r\n");
    //hex_dump("SE ATP",SE_ATP, POWER_ON_ATP_LEN);

	t1_set_param(&t1, IFD_PROTOCOL_T1_IFSC, SE_ATP[IFSC]);

	if(SE_ATP[CHECKTYPE] == CHECKSUM_CRC)
	{
		t1_set_param(&t1, IFD_PROTOCOL_T1_CHECKSUM_CRC, 0);
	}
	else
	{
		t1_set_param(&t1, IFD_PROTOCOL_T1_CHECKSUM_LRC, 0);
	}

    sumlen = t1.checksum(SE_ATP, (SE_ATP[ATPLEN] - t1.rc_bytes+1), csum);
	if((t1.rc_bytes != sumlen) ||
			(0 != memcmp(&SE_ATP[CHECKSUM], csum, t1.rc_bytes)))
	{
		return HsmStatus_Sumcheckfail;
	}

	gLockFlg = 0;

	return HsmStatus_Success;
}


static int16_t Apdu_packaging(uint8_t* apdu_cmd, const uint8_t *apdu_data, uint8_t lc, uint8_t le)
{
	int16_t apdu_length = CMD_APDU_SIZE(lc);

	if(apdu_cmd == NULL || lc > 0xFF || le > 0xFF)
	{
		return -1;
	}

    if(lc)
    {
        apdu_length++;
    }

    if(le)
    {
        apdu_length++;
    }

	if(lc)
	{
		apdu_cmd[LC] = lc;
	}

	if (apdu_data != NULL)
	{
		memcpy(&apdu_cmd[CDATA], apdu_data, lc);
	}

	if(le && apdu_data != NULL && lc != 0)
	{
		apdu_cmd[CDATA + lc + OFF_LE] = le;
	}
	else if(le)
	{
		apdu_cmd[LC] = le;
	}

    return apdu_length;
}


static status_t Apdu_Select(void)
{
	status_t Ret=HsmStatus_Success;

	ingeek_zeroize(Apdu_buff,APDU_BUF_SIZE);

    Apdu_buff[CLA] = 0x00;
    Apdu_buff[INS] = 0xA4;
    Apdu_buff[P1]  = 0x04;
    Apdu_buff[P2]  = 0x00;

    Apdulen = Apdu_packaging(Apdu_buff, AID, AIDLEN, 0);
	if(Apdulen < 0)
	{
		Ret=HsmStatus_Apdupackfail;
	}

	return Ret;
}


static status_t Apdu_Get_data(uint8_t p1)
{
	status_t Ret=HsmStatus_Success;
	uint8_t le=0;

	if(p1 != INGEEK_SEID_DATA &&
			p1 != INGEEK_VIN_DATA)
	{
		return HsmStatus_InvalidArgument;
	}

	ingeek_zeroize(Apdu_buff, APDU_BUF_SIZE);

    Apdu_buff[CLA] = 0x00;
    Apdu_buff[INS] = 0xB0;
    Apdu_buff[P1]  = p1;
    Apdu_buff[P2]  = 0x00;

    if(p1 == INGEEK_SEID_DATA)
    {
    	le=0x10;
    }
    else
    {
    	le=0x11;
    }

    Apdulen = Apdu_packaging(Apdu_buff, NULL, 0, le);
	if(Apdulen < 0)
	{
		Ret=HsmStatus_Apdupackfail;
	}

	return Ret;
}


static status_t Apdu_Session_Init(uint8_t* data, uint32_t data_len, uint8_t p1, uint8_t p2)
{
	status_t Ret=HsmStatus_Success;
	uint8_t  business_type=INGEEK_VCK_BUSINESS;
	uint8_t  more_data=INGEEK_LAST_DATA;

	if(data == NULL
			|| data_len <= 0)
	{
		return HsmStatus_InvalidArgument;
	}

	ingeek_zeroize(Apdu_buff, APDU_BUF_SIZE);

	if(p1 != INGEEK_VCK_BUSINESS)
	{
		business_type= p1;
	}

	if(p2 != INGEEK_LAST_DATA
			&& p2 == INGEEK_MORE_DATA)
	{
		more_data = p2;
	}

    Apdu_buff[CLA] = 0x00;
    Apdu_buff[INS] = 0xB2;
    Apdu_buff[P1]  = business_type;
    Apdu_buff[P2]  = more_data;

    Apdulen = Apdu_packaging(Apdu_buff, data, data_len, 0);
	if(Apdulen < 0)
	{
		Ret=HsmStatus_Apdupackfail;
	}

	return Ret;
}



static status_t Apdu_Session_Auth(uint8_t* data, uint32_t data_len, uint8_t p1)
{
	status_t Ret=HsmStatus_Success;
	uint8_t  business_type=INGEEK_VCK_BUSINESS;

	if(data == NULL || data_len <= 0)
	{
		return HsmStatus_InvalidArgument;
	}

	ingeek_zeroize(Apdu_buff, APDU_BUF_SIZE);

	if(p1 != INGEEK_VCK_BUSINESS)
	{
		business_type= p1;
	}

    Apdu_buff[CLA] = 0x00;
    Apdu_buff[INS] = 0xB4;
    Apdu_buff[P1]  = business_type;
    Apdu_buff[P2]  = 0x00;

    Apdulen = Apdu_packaging(Apdu_buff, data, data_len, 0);
	if(Apdulen < 0)
	{
		Ret = HsmStatus_Apdupackfail;
	}

	return Ret;
}



static status_t Apdu_Command(uint8_t* data, uint32_t data_len, uint8_t p1)
{
	status_t Ret=HsmStatus_Success;

	if(data == NULL || data_len <= 0)
	{
		return HsmStatus_InvalidArgument;
	}

	ingeek_zeroize(Apdu_buff, APDU_BUF_SIZE);

    Apdu_buff[CLA] = 0x00;
    Apdu_buff[INS] = 0xB6;
    Apdu_buff[P1]  = p1;
    Apdu_buff[P2]  = 0x00;

    Apdulen = Apdu_packaging(Apdu_buff, data, data_len, 0);
	if(Apdulen < 0)
	{
		Ret = HsmStatus_Apdupackfail;
	}

	return Ret;
}


static status_t Apdu_Verify_Permission(uint8_t* data, uint32_t data_len)
{
	status_t Ret=HsmStatus_Success;

	if(data == NULL || data_len <= 0)
	{
		return HsmStatus_InvalidArgument;
	}

	ingeek_zeroize(Apdu_buff, APDU_BUF_SIZE);

    Apdu_buff[CLA] = 0x00;
    Apdu_buff[INS] = 0xB8;
    Apdu_buff[P1]  = 0x00;
    Apdu_buff[P2]  = 0x00;

    Apdulen = Apdu_packaging(Apdu_buff, data, data_len, 0);
	if(Apdulen < 0)
	{
		Ret=HsmStatus_Apdupackfail;
	}

	return Ret;
}


static status_t Apdu_Car_Bind(uint8_t* data, uint32_t data_len)
{
	status_t Ret=HsmStatus_Success;

	if(data == NULL || data_len <= 0)
	{
		return HsmStatus_InvalidArgument;
	}

	ingeek_zeroize(Apdu_buff, APDU_BUF_SIZE);

    Apdu_buff[CLA] = 0x00;
    Apdu_buff[INS] = 0xF0;
    Apdu_buff[P1]  = 0x0F;
    Apdu_buff[P2]  = 0x00;

    Apdulen = Apdu_packaging(Apdu_buff, data, data_len, 0);
	if(Apdulen < 0)
	{
		Ret=HsmStatus_Apdupackfail;
	}

	return Ret;
}



static status_t Apdu_Blacklist(uint8_t* data, uint32_t data_len, uint8_t p1, uint8_t p2)
{
	status_t Ret=HsmStatus_Success;

	if(data == NULL || data_len <= 0)
	{
		return HsmStatus_InvalidArgument;
	}

	ingeek_zeroize(Apdu_buff, APDU_BUF_SIZE);

    Apdu_buff[CLA] = 0x00;
    Apdu_buff[INS] = 0xF2;
    Apdu_buff[P1]  = p1;
    Apdu_buff[P2]  = p2;

    Apdulen = Apdu_packaging(Apdu_buff, data, data_len, 0);
	if(Apdulen < 0)
	{
		Ret=HsmStatus_Apdupackfail;
	}

	return Ret;
}


static status_t Apdu_Cipher_cmd(uint8_t* data, uint32_t data_len, uint8_t p1, uint8_t p2)
{
	status_t Ret=HsmStatus_Success;

	if(data == NULL || data_len <= 0)
	{
		return HsmStatus_InvalidArgument;
	}

	ingeek_zeroize(Apdu_buff, APDU_BUF_SIZE);

    Apdu_buff[CLA] = 0x00;
    Apdu_buff[INS] = 0xF4;
    Apdu_buff[P1]  = p1;
    Apdu_buff[P2]  = p2;

    Apdulen = Apdu_packaging(Apdu_buff, data, data_len, 0);
	if(Apdulen < 0)
	{
		Ret=HsmStatus_Apdupackfail;
	}

	return Ret;
}


static status_t Apdu_Signature_cmd(uint8_t* data, uint32_t data_len, uint8_t p2)
{
	status_t Ret=HsmStatus_Success;

	if(data == NULL || data_len <= 0)
	{
		return HsmStatus_InvalidArgument;
	}

	ingeek_zeroize(Apdu_buff, APDU_BUF_SIZE);

    Apdu_buff[CLA] = 0x00;
    Apdu_buff[INS] = 0xF6;
    Apdu_buff[P1]  = 0x00;
    Apdu_buff[P2]  = p2;

    Apdulen = Apdu_packaging(Apdu_buff, data, data_len, 0);
	if(Apdulen < 0)
	{
		Ret=HsmStatus_Apdupackfail;
	}

	return Ret;
}


static status_t Apdu_Sign_Verify(uint8_t* data, uint32_t data_len, uint8_t p1, uint8_t p2)
{
	status_t Ret=HsmStatus_Success;

	if(data == NULL || data_len <= 0)
	{
		return HsmStatus_InvalidArgument;
	}

	ingeek_zeroize(Apdu_buff, APDU_BUF_SIZE);

    Apdu_buff[CLA] = 0x00;
    Apdu_buff[INS] = 0xF8;
    Apdu_buff[P1]  = p1;
    Apdu_buff[P2]  = p2;

    Apdulen = Apdu_packaging(Apdu_buff, data, data_len, 0);
	if(Apdulen < 0)
	{
		Ret=HsmStatus_Apdupackfail;
	}

	return Ret;
}


static uint32_t Apdu_Get_Response(Se_Resp_t* apdu_resp)
{
    int i=0;

	ingeek_zeroize(Apdu_buff, APDU_BUF_SIZE);
	apdu_resp->Apdu_resp=Apdu_buff;
	apdu_resp->resp_len=0;
	apdu_resp->status=0;

    if(apdu_resp->Apdu_resp == NULL)
    {
    	return HsmStatus_InvalidArgument;
    }

    if(Tblock_len < 2 || Tblock_len > 0xFF)
    {
        return HsmStatus_InvalidArgument;
    }

    for(i=0; i < (Tblock_len-2); i++)
    {
    	apdu_resp->Apdu_resp[i] = Tblock[i];
    }

    memcpy(&apdu_resp->status,&Tblock[i],sizeof(uint16_t));

    apdu_resp->resp_len=Tblock_len-2;

    return HsmStatus_Success;
}



status_t ingeek_hsm_apdu(Apdu_type_e apdu_type, uint8_t* input, uint32_t ilen, uint8_t p1, uint8_t p2, Se_Resp_t* apdu_resp)
{
	status_t Ret = HsmStatus_Success;

	while (gLockFlg == 1)
	{
		HSM_Delay(3);
	}

	gLockFlg = 1;

	if(Get_Spi_status() != IDLE_STATUS)
	{
		gLockFlg = 0;
		return HsmStatus_Spitimeout;
	}

    switch(apdu_type)
    {
    	case APDU_SELECT:
    		Ret=Apdu_Select();
    		break;
    	case APDU_DATA:
    		Ret=Apdu_Get_data(p1);
    		break;
    	case APDU_SESSION_INIT:
    		Ret=Apdu_Session_Init(input, ilen, p1, p2);
    		break;
    	case APDU_SESSION_AUTH:
    		Ret=Apdu_Session_Auth(input, ilen, p1);
    		break;
    	case APDU_CMD:
    	case APDU_MSG:
    		Ret=Apdu_Command(input, ilen, p1);
    		break;
    	case APDU_VERFIY_PERM:
    		Ret=Apdu_Verify_Permission(input, ilen);
    		break;
    	case APDU_CAR_BIND:
    		Ret=Apdu_Car_Bind(input, ilen);
    		break;
    	case APDU_BLACKLIST:
    		Ret=Apdu_Blacklist(input, ilen, p1, p2);
    		break;
    	case APDU_CIPHER:
    		Ret=Apdu_Cipher_cmd(input, ilen, p1, p2);
    		break;
    	case APDU_SIGNATURE:
    		Ret=Apdu_Signature_cmd(input, ilen, p2);
    		break;
    	case APDU_SIGNATURE_VERFIY:
    		Ret=Apdu_Sign_Verify(input, ilen, p1, p2);
    		break;
        default:
        	Ret = HsmStatus_NoApdutype;
            break;
    }

    //hex_dump("APDU_CMD",Apdu_buff,Apdulen);
    if(Ret != HsmStatus_Success)
    {
    	gLockFlg = 0;
    	ingeek_zeroize(Apdu_buff, APDU_BUF_SIZE); /*if failed. clear apdu Buff*/
    	Apdulen=0;
    	return Ret;
    }

    ingeek_zeroize(Tblock,T1_BUFFER_SIZE);

    Tblock_len = t1_transceive(&t1, T1_NAD, Apdu_buff, Apdulen, Tblock, T1_BUFFER_SIZE);
    if(Tblock_len <= 0)
    {
    	gLockFlg = 0;
    	return Tblock_len;  /*if < 0 T=1 return error code*/
    }

    //hex_dump("HSM_RESP",Tblock, Tblock_len);
	Ret = Apdu_Get_Response(apdu_resp);
	if(Ret != HsmStatus_Success)
	{
		apdu_resp->Apdu_resp=NULL;
		apdu_resp->resp_len=0;
		apdu_resp->status=0;
		ingeek_zeroize(Apdu_buff, APDU_BUF_SIZE);
	}

	gLockFlg = 0;
	return Ret;
}


#endif  /*HSM_APDU*/



