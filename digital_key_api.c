/*
 * digital_key_api.c
 *
 *  Created on: 2018年7月12日
 *      Author: zhenchiw
 */
#include "string.h"
#include "hsm_apdu.h"
#include "digital_key_api.h"
#include "../rtc/ExtRTC.h"
#include "utc_time.h"

#define  APDU_MAXLEN  	 0xF9
#define  APDU_status_OK  0x90
#define  TMSTART_OFFSET  38

#define  SWAP_16(buf) \
    ((*((buf) + 1)) | ((*(buf)) << 8))


enum SDK_Status{SE_INIT,SESSION_INIT,SESSION_AUTH,INVALID=0xFF};
static  uint8_t Sdk_status = INVALID;
static 	uint8_t applet_version[2]={0};

static status_t Check_valid_date(uint8_t *input, uint32_t ilen);
static void Extract_Data(uint8_t *input);

static void set_sdk_status(uint8_t status)
{
	Sdk_status=status;
}


static status_t ingeek_select_Dk(void)
{
	status_t Ret = HsmStatus_Success;
	Se_Resp_t apdu_resp;

	Ret=ingeek_hsm_apdu(APDU_SELECT, NULL, 0, 0, 0, &apdu_resp);
	if(Ret != HsmStatus_Success)
	{
		memset(applet_version, 0x00, 2);
		return Ret;
	}

	if(apdu_resp.status != APDU_status_OK)
	{
		return  HsmStatus_Selectfail;
	}

	if(apdu_resp.Apdu_resp[2] != 0x1F)
	{
		return  HsmStatus_Personalized;
	}

	memcpy(applet_version, apdu_resp.Apdu_resp, 2);

	return Ret;
}


status_t ingeek_se_init(uint8_t s_type)
{
	status_t Ret = HsmStatus_Success;

	if(s_type)
	{
		/* SE power on */
		Ret = ingeek_hsm_init();
		if(Ret != HsmStatus_Success)
		{
			set_sdk_status(INVALID);
			return Ret;
		}

		/*power on select apdu need long time for SE*/
		ingeek_select_Dk();
	}
	else
	{
		/* BLE connect */
		memset(applet_version, 0x00, 2);
		Ret = ingeek_select_Dk();
		if(Ret != HsmStatus_Success)
		{
			set_sdk_status(INVALID);
			return Ret;
		}

		set_sdk_status(SE_INIT);
	}

	return HsmStatus_Success;
}


status_t ingeek_se_final(void)
{
	memset(applet_version, 0x00, 2);

	if(Sdk_status == INVALID)
	{
		return HsmStatus_Statusfail;
	}
	else
	{
		set_sdk_status(SE_INIT);
	}

	return HsmStatus_Success;
}


static status_t Check_valid_date(uint8_t *input, uint32_t ilen)
{
	status_t Ret = HsmStatus_Success;
	struct Time_t Ble_time;
	UTCTime tmsec_now=0,tmsec_start=0,tmsec_end=0;
	UTCTimeStruct tm_now;
	
	memset(&Ble_time, 0x00,sizeof(Ble_time));
	Ret = RTC_GetTime(&Ble_time);
	if(Ret)
	{
		return HsmStatus_RTCGetTimefail;
	}
	else
	{
		tm_now.year = Ble_time.mYear;
		tm_now.month = --Ble_time.mMon;
		tm_now.day = --Ble_time.mDay;
		tm_now.hour = Ble_time.mHour;
		tm_now.minutes = Ble_time.mMin;
		tm_now.seconds = Ble_time.mSec;
		
		tmsec_now = UTC_convertUTCSecs(&tm_now);
	}
	
	uint8_t *time_stamp=input+(SWAP_16(input)+sizeof(uint16_t)+TMSTART_OFFSET);
	Ret = Get_vck_tmsec(time_stamp, &tmsec_start);
	if(Ret != 0)
	{
		return Ret;
	}
	
	time_stamp+=16;
	Ret = Get_vck_tmsec(time_stamp, &tmsec_end);
	if(Ret != 0)
	{
		return Ret;
	}
	
	if((tmsec_now-tmsec_start <= 0) 
		|| (tmsec_now - tmsec_end >= 0)
		|| (tmsec_end -tmsec_start <= 0))
	{
		return HsmStatus_VCKTimefail;
	}

	return Ret;
}


static void Extract_Data(uint8_t *input)
{
	int i=0;
	uint8_t* p1 = input+(sizeof(uint16_t)+SWAP_16(input));
	uint8_t* p2 = p1+sizeof(uint16_t);
	uint16_t vckinfo_len=SWAP_16(p1);

	for(i=0; i< vckinfo_len; i++){
		p1[i]=p2[i];
	}
}


status_t ingeek_push_auth(uint8_t *input, uint32_t ilen, uint8_t* output, uint32_t* olen)
{
	status_t Ret = HsmStatus_Success;
	uint32_t offset=0;
	Se_Resp_t apdu_resp;

	if(Sdk_status != SE_INIT)
	{
		return HsmStatus_Statusfail;
	}

	if(input == NULL || ilen <= 0)
	{
		return HsmStatus_InvalidArgument;
	}

	Ret = Check_valid_date(input, ilen);
	if(Ret != HsmStatus_Success)
	{
		return Ret;
	}

	Extract_Data(input);
	input+=sizeof(uint16_t);

	while(1)
	{
		if(ilen- offset <= APDU_MAXLEN)
		{
			Ret = ingeek_hsm_apdu(APDU_SESSION_INIT,
					input+offset, ilen-offset,
					INGEEK_VCK_BUSINESS, INGEEK_LAST_DATA, &apdu_resp);
			if(Ret != HsmStatus_Success)
			{
				return Ret;
			}

			break;
		}
		else
		{
			Ret = ingeek_hsm_apdu(APDU_SESSION_INIT,
					input+offset, APDU_MAXLEN,
					INGEEK_VCK_BUSINESS, INGEEK_MORE_DATA, &apdu_resp);
			if(Ret != HsmStatus_Success)
			{
				return Ret;
			}

			offset+=APDU_MAXLEN;
		}
	}

	if(apdu_resp.status != APDU_status_OK)
	{
		return  HsmStatus_NO9000;
	}

	*olen = apdu_resp.resp_len;
	memcpy(output, apdu_resp.Apdu_resp, apdu_resp.resp_len);

	set_sdk_status(SESSION_INIT);

	return Ret;
}


status_t ingeek_push_session(uint8_t *input, uint32_t ilen, uint8_t* output, uint32_t* olen)
{
	status_t Ret = HsmStatus_Success;
	Se_Resp_t apdu_resp;

	if(Sdk_status != SESSION_INIT)
	{
		set_sdk_status(SE_INIT);
		return HsmStatus_Statusfail;
	}

	if(input == NULL || ilen <= 0)
	{
		set_sdk_status(SE_INIT);
		return HsmStatus_InvalidArgument;
	}

	Ret=ingeek_hsm_apdu(APDU_SESSION_AUTH,
			input, ilen,
			INGEEK_VCK_BUSINESS, INGEEK_LAST_DATA, &apdu_resp);
	if(Ret != HsmStatus_Success)
	{
		set_sdk_status(SE_INIT);
		return Ret;
	}

	if(apdu_resp.status != APDU_status_OK)
	{
		set_sdk_status(SE_INIT);
		return  HsmStatus_NO9000;
	}

	*olen = apdu_resp.resp_len;
	memcpy(output, apdu_resp.Apdu_resp, apdu_resp.resp_len);

	set_sdk_status(SESSION_AUTH);
	return Ret;
}


status_t ingeek_command_input_action(uint8_t *input, uint32_t ilen, DK_Cmd_Meg* Can_Packet)
{
	status_t Ret = HsmStatus_Success;
	Se_Resp_t apdu_resp;

	if(Sdk_status != SESSION_AUTH)
	{
		set_sdk_status(SE_INIT);
		return HsmStatus_Statusfail;
	}

	if(input == NULL || ilen == 0 || Can_Packet == NULL)
	{
		set_sdk_status(SE_INIT);
		return HsmStatus_InvalidArgument;
	}

	Ret=ingeek_hsm_apdu(APDU_CMD, input, ilen, INGEEK_DECRYPT, 0, &apdu_resp);
	if(Ret != HsmStatus_Success)
	{
		set_sdk_status(SE_INIT);
		return Ret;
	}

	if(apdu_resp.status != APDU_status_OK)
	{
		set_sdk_status(SE_INIT);
		return  HsmStatus_NO9000;
	}

	Can_Packet->command=apdu_resp.Apdu_resp[0];
	Can_Packet->result = Ret;
	Can_Packet->index = 0x01;
	Can_Packet->sparam_size=apdu_resp.Apdu_resp[1];
	memcpy(Can_Packet->sparam, &apdu_resp.Apdu_resp[2], Can_Packet->sparam_size);

	return Ret;
}


status_t ingeek_command_output_action(DK_Cmd_Meg* Can_Packet, uint8_t* output, uint32_t* olen)
{
	status_t Ret = HsmStatus_Success;
	Se_Resp_t apdu_resp;

	if(Sdk_status != SESSION_AUTH)
	{
		set_sdk_status(SE_INIT);
		return HsmStatus_Statusfail;
	}

    if(Can_Packet->sparam_size > SPARAM_MAX_LEN
    		|| Can_Packet->sparam == NULL)
    {
    	set_sdk_status(SE_INIT);
    	return HsmStatus_InvalidArgument;
    }

	Ret=ingeek_hsm_apdu(APDU_CMD,
			(uint8_t*)Can_Packet, sizeof(DK_Cmd_Meg),
			INGEEK_ENCRYPT, 0, &apdu_resp);
	if(Ret != HsmStatus_Success)
	{
		set_sdk_status(SE_INIT);
		return Ret;
	}

	if(apdu_resp.status != APDU_status_OK)
	{
		set_sdk_status(SE_INIT);
		return  HsmStatus_NO9000;
	}

	*olen = apdu_resp.resp_len;
	memcpy(output, apdu_resp.Apdu_resp, apdu_resp.resp_len);

	return Ret;
}


status_t ingeek_message_output_action(DK_Cmd_Meg* Can_Packet, uint8_t* output, uint32_t* olen)
{
	status_t Ret = HsmStatus_Success;
	Se_Resp_t apdu_resp;

	if(Sdk_status != SESSION_AUTH)
	{
		return HsmStatus_Statusfail;
	}

    if(Can_Packet->sparam_size > SPARAM_MAX_LEN
    		|| Can_Packet->sparam == NULL)
    {
    	return HsmStatus_InvalidArgument;
    }

	Ret=ingeek_hsm_apdu(APDU_CMD,
			(uint8_t*)Can_Packet, sizeof(DK_Cmd_Meg),
			INGEEK_ENCRYPT, 0, &apdu_resp);
	if(Ret != HsmStatus_Success)
	{
		return Ret;
	}

	if(apdu_resp.status != APDU_status_OK)
	{
		return  HsmStatus_NO9000;
	}

	*olen = apdu_resp.resp_len;
	memcpy(output, apdu_resp.Apdu_resp, apdu_resp.resp_len);

	return Ret;
}


uint8_t ingeek_get_sec_status(void)
{
	return Sdk_status;
}


uint8_t* ingeek_get_version(void)
{
	if(applet_version[0] == 0x00
			&& applet_version[1] == 0x00)
	{
		return NULL;
	}

	return applet_version;
}


status_t ingeek_get_seid(uint8_t* output, uint32_t* olen)
{
	status_t Ret = HsmStatus_Success;
	Se_Resp_t apdu_resp;

	Ret=ingeek_hsm_apdu(APDU_DATA, NULL, 0, INGEEK_SEID_DATA, 0, &apdu_resp);
	if(Ret != HsmStatus_Success)
	{
		return Ret;
	}

	if(apdu_resp.status != APDU_status_OK)
	{
		return  HsmStatus_NO9000;
	}

	*olen = apdu_resp.resp_len;
	memcpy(output, apdu_resp.Apdu_resp, apdu_resp.resp_len);

	return Ret;
}


status_t  ingeek_sync_rtc_time(uint8_t *input, uint32_t ilen)
{
	status_t Ret = HsmStatus_Success;
	Se_Resp_t apdu_resp;
	struct Time_t Stime;
	UTCTimeStruct Utc_time;

	if(input == NULL || ilen <= 0)
	{
		return HsmStatus_InvalidArgument;
	}

	Ret=ingeek_hsm_apdu(APDU_SIGNATURE_VERFIY, input, ilen, INGEEK_ECDSA_SHA_256, 0, &apdu_resp);
	if(Ret != HsmStatus_Success)
	{
		return Ret;
	}

	if(apdu_resp.status != APDU_status_OK)
	{
		return  HsmStatus_NO9000;
	}

	uint8_t * time_stamp = NULL;
	time_stamp=input+(SWAP_16(input)+sizeof(uint16_t));/* little-endian*/
	time_stamp +=sizeof(uint16_t);
	Filter_time(&Utc_time, time_stamp);
	
	memset(&Stime, 0x00, sizeof(Stime));
	Stime.mYear	= Utc_time.year;
	Stime.mMon	= Utc_time.month;
	Stime.mDay	= Utc_time.day;
	Stime.mHour	= Utc_time.hour;
	Stime.mMin	= Utc_time.minutes;
	Stime.mSec	= Utc_time.seconds;
	
	Ret=RTC_SetTime(&Stime);
	if(Ret != 0)
	{
		return HsmStatus_RTCSetTimefail;
	}
	
	return Ret;
}


status_t ingeek_signature_verify(uint8_t *input, uint32_t ilen, uint8_t *output, uint32_t* olen, OPERAE_T Dir)
{
	status_t Ret = HsmStatus_Success;
	Se_Resp_t apdu_resp;
	uint32_t offset = 0;

	if(input == NULL || ilen == 0)
	{
		return HsmStatus_InvalidArgument;
	}

	if(Dir == INGEEK_SIGNATURE)
	{
		while(1)
		{
			if(ilen- offset <= APDU_MAXLEN)
			{
				Ret = ingeek_hsm_apdu(APDU_SIGNATURE,
						input+offset, ilen-offset,
						0, INGEEK_LAST_DATA, &apdu_resp);
				if(Ret != HsmStatus_Success)
				{
					return Ret;
				}

				break;
			}
			else
			{
				Ret = ingeek_hsm_apdu(APDU_SIGNATURE,
						input+offset, APDU_MAXLEN,
						0, INGEEK_MORE_DATA, &apdu_resp);
				if(Ret != HsmStatus_Success)
				{
					return Ret;
				}

				offset+=APDU_MAXLEN;
			}
		}
	}
	else
	{
		Ret=ingeek_hsm_apdu(APDU_SIGNATURE_VERFIY, input, ilen, INGEEK_ECDSA_SHA_256, INGEEK_LAST_DATA, &apdu_resp);
		while(1)
		{
			if(ilen- offset <= APDU_MAXLEN)
			{
				Ret = ingeek_hsm_apdu(APDU_SIGNATURE_VERFIY,
						input+offset, ilen-offset,
						INGEEK_ECDSA_SHA_256, INGEEK_LAST_DATA, &apdu_resp);
				if(Ret != HsmStatus_Success)
				{
					return Ret;
				}

				break;
			}
			else
			{
				Ret = ingeek_hsm_apdu(APDU_SIGNATURE_VERFIY,
						input+offset, APDU_MAXLEN,
						INGEEK_ECDSA_SHA_256, INGEEK_MORE_DATA, &apdu_resp);
				if(Ret != HsmStatus_Success)
				{
					return Ret;
				}

				offset+=APDU_MAXLEN;
			}
		}
	}

	if(apdu_resp.status != APDU_status_OK)
	{
		return  HsmStatus_NO9000;
	}

	if(Dir == INGEEK_SIGNATURE)
	{
		*olen = apdu_resp.resp_len;
		memcpy(output, apdu_resp.Apdu_resp, apdu_resp.resp_len);
	}

	return Ret;
}


status_t ingeek_push_info(uint8_t *input, uint32_t ilen, uint8_t* output, uint32_t* olen)
{
	return 0;
}


uint32_t ingeek_get_pair_df(void)
{

	return 0xaac71eeb;
}


uint32_t ingeek_calculate_vinhash(uint8_t* output, uint32_t* outlen)
{
	return 8888;
}


void ingeek_set_callback(read_CallBack rcb,write_CallBack wcb,Rand_CallBack randcb)
{

}


int ingeek_get_base_info(uint32_t *phone_type, uint8_t* phone_number, uint32_t *VKID)
{
	return 0;
}

#if defined(SE_PROD_TEST)

status_t ingeek_se_prod_test(uint8_t *input, uint32_t ilen, uint8_t seid[16])
{
	status_t Ret = TEST_OK;
	uint8_t Pbuf[80]={0};
	uint32_t Pbuflen=0;
	uint32_t seidlen=0;

	Ret = ingeek_select_Dk();
	if(Ret != HsmStatus_Success)
	{
		return TEST_SELECT_ERR;
	}

	Ret = ingeek_get_seid(seid, &seidlen);
	if(Ret != HsmStatus_Success)
	{
		return TEST_SEID_ERR;
	}

	if(seidlen != 0x10)
	{
		return TEST_SEIDLEN_ERR;
	}

	Ret = ingeek_push_auth(input, ilen, Pbuf, &Pbuflen);
	if(Ret != HsmStatus_Success)
	{
		return TEST_VERIFY_ERR;
	}

	return Ret;
}

#endif




#if 0
status_t ingeek_Get_Permission()
{

}

status_t ingeek_check_whilelist()
{

}


void RTC_test(void)
{
	status_t Ret = HsmStatus_Success;
	struct Time_t Stime;
	UTCTimeStruct Utc_time;
	uint8_t * time_stamp = "20010207T040500Z";

	Filter_time(&Utc_time, time_stamp);

	memset(&Stime, 0x00, sizeof(Stime));
	Stime.mYear = Utc_time.year;
	Stime.mMon = Utc_time.month;
	Stime.mDay = Utc_time.day;
	Stime.mHour = Utc_time.hour;
	Stime.mMin = Utc_time.minutes;
	Stime.mSec = Utc_time.seconds;

	DEBUG_PRINTF("SET:%d-%d-%d-%d-%d-%d\n",Stime.mYear,Stime.mMon,Stime.mDay,Stime.mHour,Stime.mMin,Stime.mSec);
	Ret=RTC_SetTime(&Stime);
	if(Ret != 0)
	{
		DEBUG_PRINTF("RTC_SetTime_failed\n");
	}

	memset(&Stime, 0x00,sizeof(Stime));
	Ret = RTC_GetTime(&Stime);
	if(Ret != 0)
	{
		DEBUG_PRINTF("RTC_GetTime_failed\n");
	}
	DEBUG_PRINTF("GET:%d-%d-%d-%d-%d-%d\n",Stime.mYear,Stime.mMon,Stime.mDay,Stime.mHour,Stime.mMin,Stime.mSec);
}
#endif

/*****************************************************************/


