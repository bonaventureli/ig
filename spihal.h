/*
 * spihsm.h
 *
 *  Created on: 2018年7月26日
 *      Author: zhenchiw
 */

#ifndef HSM_SPIHSM_H_
#define HSM_SPIHSM_H_

#include "../spi/public.h"
#include "../spi/hsm.h"

#if defined(__cplusplus)
extern "C" {
#endif /*_cplusplus*/

#define TASK_TIME_CYCLE		(10) /*<ms>*/
#define T1BLOCK_HEADER  	(3)	 /*<NAD+PCB+LEN>*/
#define APDU_DATA_SIZE  	254
#define WAKEUP_WAIT_TIME	1	 /*Wakeup wait for 1ms */
#define WAIT_TIMEOUT		3000 /*ms*/

typedef enum
{
    IDLE_STATUS = 0,
    WAIT_RX_STATUS,
    RECEIVE_STATUS,
} Hsm_Status_e;


typedef enum
{
	RX_NONE = 0,
	RX_SUCCESS,
	RX_FAILED
} Hsm_Reslut_e;


extern void Apdu_Para_t_clean(void);

extern u8_t Get_Spi_status(void);

extern void SPI_Hsm_Task(void *param);

extern rv_t SPI_hsm_transfer(u8_t dad, u8_t *T1block, u32_t T1blocklen, u32_t *pRxSize);

extern void hex_dump(void* name, u8_t* str, u32_t len);

#if defined(__cplusplus)
}

#endif /*_cplusplus*/

#endif /*HSM_SPIHSM_H_*/
