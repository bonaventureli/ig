
#include "private_profile_interface.h"
#include "RmtCtrlTask.h"
#include "ECU_Control.h"
#include "digital_key_api.h"
//#include "pepstask.h"

extern void set_stb_filter_number(const uint8_t *p_data);
static u8_t gBuff[RES_BUFF_MAX_SIZ];

static void Sbm_Write_Sdk_Status(void)
{
	u8_t uStatus;
	bleResult_t  Result;

	uStatus = ingeek_get_sec_status();
	DEBUG_PRINTF("INGEEK_sdk_status:[0x%02x] \r\n", uStatus);

	Result = Sbm_WriteToBle(qppGatt1_c, 1, &uStatus);
	if(Result != gBleSuccess_c){
		DEBUG_PRINTF("YF_Write_StatusToBle:failed:Result=[%d] \r\n", Result);
	}
}


void BleApp_WriteGatt(uint16_t uuid, uint16_t length, uint8_t* pValue)
{
	u32_t uRet;
	u32_t uLen;
	bleResult_t  Result;
	
	switch (uuid) {
		case qppGatt2_c:
			hex_dump("W_info", pValue, length);
			uRet = ingeek_push_info(pValue, length, gBuff, &uLen);
			if (uRet){
				DEBUG_PRINTF("INGEEK_W_info:failed ==>ret=[0x%02x]\r\n", uRet);
			}
			else{
				Result = Sbm_WriteToBle(qppGatt2_c, uLen, gBuff);
				if(Result != gBleSuccess_c){
					DEBUG_PRINTF("YF_Info_WriteToBle:failed:Result=[%d]\r\n", Result);
				}
			}

			Sbm_Write_Sdk_Status();
			break;
		case qppGatt3_c:
			hex_dump("W_auth", pValue, length);
			uRet = ingeek_push_auth(pValue, length, gBuff, &uLen);
			if (uRet){
				DEBUG_PRINTF("INGEEK_W_auth:failed ==>ret=[0x%02x]\r\n", uRet);
			}
			else{
				Result = Sbm_WriteToBle(qppGatt3_c, uLen, gBuff);
				if(Result != gBleSuccess_c){
					DEBUG_PRINTF("YF_Auth_WriteToBle:failed:Result=[%d]\r\n", Result);
				}
			}

			Sbm_Write_Sdk_Status();
			break;
		case qppGatt4_c:
			hex_dump("W_session", pValue, length);
			uRet = ingeek_push_session(pValue, length, gBuff, &uLen);
			if (uRet){
				DEBUG_PRINTF("INGEEK_W_session:failed ==>ret=[0x%02x]\r\n", uRet);
			}
			else{
				Result = Sbm_WriteToBle(qppGatt4_c, uLen, gBuff);
				if(Result != gBleSuccess_c){
					DEBUG_PRINTF("YF_Session_WriteToBle:failed:Result=[%d]\r\n", Result);
				}
			}

			RmtCtrlSetSDK_Status(3);
			Sbm_Write_Sdk_Status();
			break;
		case qppGatt5_c:
		{
			hex_dump("W_cmd", pValue, length);
			struct ECU_ControlPara_t tPara;
			DK_Cmd_Meg structCmd;
			u32_t uLen = 0;

			uRet = ingeek_command_input_action(pValue, length, &structCmd);
			tPara.mCmd = structCmd.command;
			tPara.mNum = structCmd.index;
			tPara.mSubID = structCmd.sparam[0];
			structCmd.sparam_size = sizeof(tPara.mSubID);
			if (uRet == RV_SUCCESS){
                uRet = ECU_CtrlCmd(&tPara);
				if (uRet != RV_SUCCESS){
					structCmd.result = ACK_CMD_TOO_MANY;
				}
				else{
					structCmd.result = ACK_RECV;
				}
            }
			else{
                structCmd.result = ACK_SDK_FAILED;
                DEBUG_PRINTF("INGEEK_CMD_IN:failed ==>ret=[0x%02x]\r\n", uRet);
                Sbm_Write_Sdk_Status();
                return ;
            }

            uRet = ingeek_command_output_action(&structCmd, gBuff, &uLen);
            if (uRet == RV_SUCCESS){
            	Result = Sbm_WriteToBle(qppGatt5_c, uLen, gBuff);
				if(Result != gBleSuccess_c){
					DEBUG_PRINTF("YF_CMD_WriteToBle_1:failed:Result=[%d]\r\n", Result);
				}
            }
            else{
                DEBUG_PRINTF("INGEEK_CMD_OUT:failed ==>uRet=[0x%02x]\r\n", uRet);
            }

            Sbm_Write_Sdk_Status();
		}
		break;
		case qppGatt6_c:
			// yf cmd
			if (5 == length && 0x01 == pValue[0]) {
				set_stb_filter_number(pValue+1);
			}
			break;
		case qppGatt7_c:
			// TODO
			break;
		case qppGatt9_c:
			// TODO
			break;
	}
}


rv_t RmtCtrlResEncryption(struct ECU_ControlPara_t *pRes)
{
	DK_Cmd_Meg structCmd;
	u32_t uLen = 0;
	bleResult_t  Result;
	rv_t sRet = RV_SUCCESS;

	if (!pRes)
	{
		return RV_PARAM_ERR;
	}

	memset(&structCmd, 0, sizeof(structCmd));
	structCmd.command = pRes->mCmd;
	structCmd.index = pRes->mNum;
	structCmd.result = pRes->mRes;
	structCmd.sparam[0] = pRes->mSubID;
	structCmd.sparam_size = sizeof(pRes->mSubID);
	
	sRet = ingeek_command_output_action(&structCmd, gBuff, &uLen);
	if (sRet == RV_SUCCESS)
	{
		Result = Sbm_WriteToBle(qppGatt5_c, uLen, gBuff);
		if(Result != gBleSuccess_c){
			DEBUG_PRINTF("YF_CMD_WriteToBle_2:failed:Result=[%d]\r\n", Result);
		}
	}
	else
	{
		DEBUG_PRINTF("INGEEK_CMD_PERFORM:failed ==>ret=[0x%02x]\r\n", sRet);
	}

	return sRet;
}


rv_t RmtCtrlUploadMessageEncryption(struct ECU_UploadMsg_t *pMsg)
{
	rv_t sRet = RV_SUCCESS;
	DK_Cmd_Meg structCmd;
	bleResult_t  Result;
	u32_t uLen = 0;
	
	if (!pMsg)
	{
		return RV_PARAM_ERR;
	}

    memset(&structCmd, 0, sizeof(DK_Cmd_Meg));
    structCmd.command = 0xFF;
    memcpy(structCmd.sparam, (u8_t *)pMsg, sizeof(struct ECU_UploadMsg_t));	
	structCmd.sparam_size = sizeof(struct ECU_UploadMsg_t);

	sRet = ingeek_message_output_action(&structCmd, gBuff, &uLen);
	if (sRet == RV_SUCCESS)
	{
		Result = Sbm_WriteToBle(qppGatt8_c, uLen, gBuff);
		if(Result != gBleSuccess_c){
			DEBUG_PRINTF("YF_MSG_WriteToBle:failed:Result=[%d]\r\n", Result);
		}
	}
	else
	{
		DEBUG_PRINTF("INGEEK_MSG_UPLOAD:failed ==>ret=[0x%02x]\r\n", sRet);
	}

	return sRet;
}


void RmtCtrlSDKInit(void)
{
	status_t Ret = HsmStatus_Success;

	ingeek_set_callback(ReadKey, WriteKey, RandomGeneration);
	Ret = ingeek_se_init(1);
	if(Ret != HsmStatus_Success){
		DEBUG_PRINTF("INGEEK_SE_Power_On_Init:failed==>ret=[0x%02x]\r\n",Ret);
	}
	else{
		DEBUG_PRINTF("INGEEK_SE_Power_On_Init\r\n");
	}
}


void RmtCtrlSetBleStatus(u8_t status)
{
	status_t Ret = HsmStatus_Success;

	if (status)
	{
		Ret=ingeek_se_init(0);
		if(Ret != HsmStatus_Success){
			DEBUG_PRINTF("INGEEK_BLE_Connect_Se_Init:failed==>ret=[0x%02x]\r\n",Ret);
		}
		else{
			DEBUG_PRINTF("INGEEK_BLE_Connect_Se_Init\r\n");
		}
		Sbm_Write_Sdk_Status();

		RmtCtrlSetBT_Status(5);
		RmtCtrlSetSDK_Status(2);
	}
	else
	{
		RmtCtrlSetBT_Status(2);
		RmtCtrlSetSDK_Status(1);
		ingeek_se_final();
		DEBUG_PRINTF("INGEEK_BLE_Disconnect_sdk_status:[0x%02x]\r\n", ingeek_get_sec_status());
	}
}


int RmtCtrlGetVinHash(u8_t *pData, u32_t *pSize)
{
	return ingeek_calculate_vinhash(pData, pSize);
}


int RmtCtrlGetPairDf(void)
{
	int value = ingeek_get_pair_df();
	DEBUG_PRINTF("Pair_DF: 0x%04x\r\n", value);

	return value;
}

