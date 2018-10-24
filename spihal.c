/*
 * spihsm.c
 *
 *  Created on: 2018年7月26日
 *      Author: zhenchiw
 */

#include "spihal.h"
#include "7816_T1.h"

struct APDU_COMM_Para_t
{
	struct HSM_COMM_Para_t mParam;
	u8_t	mWaitRxByte;		/**< 等待接收字节  */
	u8_t	mWaitRxCount;		/**< 接收字节计数  */
	u8_t	mRxSize;			/**< 接收字节数  */
	u8_t	mStatus;			/**< 状态 */
	u32_t	mWaitTimeCount;		/**< 等待超时时间计数  */
	u32_t	mWaitOuttime;		/**< 等待超进时间  */
	u8_t	mReslut;			/**< 结果  */
};


static struct APDU_COMM_Para_t gAPDU_CommPara;
static u8_t gAPDU_TempTxBuff[APDU_DATA_SIZE];
static u8_t gAPDU_TempRxBuff[APDU_DATA_SIZE];
static u8_t gAPDU_RxBuff[APDU_DATA_SIZE];


void hex_dump(void* name, u8_t* str, u32_t len)
{
	uint32_t i=0;

	DEBUG_PRINTF("%s:",(u8_t*)name);
    for (i = 0U; i < len; i++)
    {
        if ((i & 0x0FU) == 0U)
        {
            DEBUG_PRINTF("\n");
        }

        DEBUG_PRINTF("%02x ", str[i]);
    }

    DEBUG_PRINTF("\n");
}


void Apdu_Para_t_clean(void)
{
	memset(&gAPDU_CommPara, 0x00, sizeof(gAPDU_CommPara));
	gAPDU_CommPara.mStatus = IDLE_STATUS;
}


u8_t  Get_Spi_status(void)
{
	return  gAPDU_CommPara.mStatus;
}


void SPI_Hsm_Task(void *param)
{
	struct HSM_COMM_Para_t uParam;
	rv_t uRet = RV_SUCCESS;

	switch (gAPDU_CommPara.mStatus)
    {
        case WAIT_RX_STATUS:
			uParam.pTxBuff = gAPDU_TempTxBuff;
	    	uParam.pRxBuff = gAPDU_TempRxBuff;
	    	uParam.mCS_DelayTime = WAKEUP_WAIT_TIME;
	    	uParam.mSize = 1;
            HSM_TX_RX_Buff(&uParam);

            if (uParam.pRxBuff[0] == gAPDU_CommPara.mWaitRxByte) //先读取一个字节，判断是否是dad相等，如果不等就说明有错不用收了。这个部分要看spi实现的细节的文档
            {
                memset(gAPDU_TempTxBuff, 0, sizeof(gAPDU_TempTxBuff));
				gAPDU_CommPara.mWaitRxCount = 0;//用作计数用
				gAPDU_CommPara.mParam.pRxBuff[gAPDU_CommPara.mWaitRxCount++] = uParam.pRxBuff[0];

				uParam.pTxBuff = gAPDU_TempTxBuff;
		    	uParam.pRxBuff = gAPDU_TempRxBuff;
		    	uParam.mCS_DelayTime = WAKEUP_WAIT_TIME;
		    	uParam.mSize = T1BLOCK_HEADER - 1;
                uRet = HSM_TX_RX_Buff(&uParam); //该函数既从spi收/发数据
                if (uRet == RV_SUCCESS)
                {
                	//DEBUG_PRINTF("APDU_Task: RX Wait byte ok\r\n");
                	memcpy(&gAPDU_CommPara.mParam.pRxBuff[gAPDU_CommPara.mWaitRxCount], gAPDU_TempRxBuff, T1BLOCK_HEADER - 1);
					gAPDU_CommPara.mWaitRxCount += T1BLOCK_HEADER - 1;
					gAPDU_CommPara.mStatus = RECEIVE_STATUS;
					gAPDU_CommPara.mRxSize = gAPDU_CommPara.mParam.pRxBuff[2] + T1BLOCK_HEADER - 1;
                }
                else
                {
                    //DEBUG_PRINTF("APDU_Task: Tx_RX Failed\r\n");
                    gAPDU_CommPara.mStatus = IDLE_STATUS;
					gAPDU_CommPara.mReslut = RX_FAILED;
					HSM_SendEvent(HSM_EVT_RX_FAIL);
                }
            }
            else
            {
            	//DEBUG_PRINTF("mWaitTimeCount=%d \r\n",gAPDU_CommPara.mWaitTimeCount);
                if (gAPDU_CommPara.mWaitTimeCount >= gAPDU_CommPara.mWaitOuttime)
                {
                	//DEBUG_PRINTF("APDU_Task: Tx_RX Outtime\r\n");
                    gAPDU_CommPara.mStatus = IDLE_STATUS;
					gAPDU_CommPara.mReslut = RX_FAILED;
					HSM_SendEvent(HSM_EVT_RX_FAIL);
                }
				else
				{
					gAPDU_CommPara.mWaitTimeCount += TASK_TIME_CYCLE;
				}
            }
            break;

        case RECEIVE_STATUS:
            memset(gAPDU_TempTxBuff, 0, sizeof(gAPDU_TempTxBuff));
			uParam.pTxBuff = gAPDU_TempTxBuff;
	    	uParam.pRxBuff = gAPDU_TempRxBuff;
	    	uParam.mCS_DelayTime = WAKEUP_WAIT_TIME;
	    	uParam.mSize = gAPDU_CommPara.mRxSize;
            uRet = HSM_TX_RX_Buff(&uParam);
            if (uRet == RV_SUCCESS)
            {
            	//DEBUG_PRINTF("APDU_Task: RX Success\r\n");
            	memcpy(&gAPDU_CommPara.mParam.pRxBuff[gAPDU_CommPara.mWaitRxCount], gAPDU_TempRxBuff, uParam.mSize);
            	gAPDU_CommPara.mRxSize += T1BLOCK_HEADER;
			    gAPDU_CommPara.mStatus = IDLE_STATUS;
				gAPDU_CommPara.mReslut = RX_SUCCESS;
				HSM_SendEvent(HSM_EVT_RX_SUCCESS);
            }
            else
            {
            	//DEBUG_PRINTF("APDU_Task: Tx_RX Failed\r\n");
                gAPDU_CommPara.mStatus = IDLE_STATUS;
				gAPDU_CommPara.mReslut = RX_FAILED;
				HSM_SendEvent(HSM_EVT_RX_FAIL);
            }
            break;

        default:
            break;
    }
}



rv_t SPI_hsm_transfer(u8_t dad, u8_t *T1block, u32_t T1blocklen, u32_t *pRxSize)
{
	rv_t uRet = RV_SUCCESS;

	if (NULL == T1block || 0 == T1blocklen)
	{
		return RV_PARAM_ERR;
	}

	hex_dump("MCU---->SE", T1block, T1blocklen);
	if (gAPDU_CommPara.mStatus == IDLE_STATUS)
	{
		gAPDU_CommPara.mWaitTimeCount = 0 ; /*reset for next*/

	    gAPDU_CommPara.mParam.pTxBuff = T1block;
	    gAPDU_CommPara.mParam.pRxBuff = gAPDU_RxBuff;
	    gAPDU_CommPara.mParam.mCS_DelayTime = WAKEUP_WAIT_TIME;
	    gAPDU_CommPara.mParam.mSize = T1blocklen;

		gAPDU_CommPara.mWaitRxByte = swap_nibbles(dad);
		gAPDU_CommPara.mWaitOuttime = WAIT_TIMEOUT;
		gAPDU_CommPara.mWaitRxCount = 0;
		gAPDU_CommPara.mReslut = RX_NONE;
		uRet=HSM_TX_RX_Buff(&gAPDU_CommPara.mParam); //这个函数是往底层SPI写数据
		if(uRet != RV_SUCCESS)
		{
			//DEBUG_PRINTF("HSM_TX_RX_Buff Send fail \r\n");
			return uRet;
		}

		gAPDU_CommPara.mStatus = WAIT_RX_STATUS;
		HSM_WaitEvent();

		gAPDU_CommPara.mStatus = IDLE_STATUS;

		if (gAPDU_CommPara.mReslut == RX_SUCCESS)
		{
			memset(T1block, 0x00, T1_BUFFER_SIZE);
			memcpy(T1block, gAPDU_CommPara.mParam.pRxBuff, gAPDU_CommPara.mRxSize);
			*pRxSize = gAPDU_CommPara.mRxSize;
			hex_dump("MCU<----SE", T1block, *pRxSize);
		}
		else
		{
			uRet = RV_FAILED;
		}
	}
	else
	{
		uRet = RV_BUSY;
	}

	return uRet;
}


