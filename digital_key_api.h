/*
 * digital_key_api.h
 *
 *  Created on: 2018年7月12日
 *      Author: zhenchiw
 */

#ifndef DIGITAL_KEY_API_H_
#define DIGITAL_KEY_API_H_


#if defined(__cplusplus)
extern "C" {
#endif /*_cplusplus*/

typedef int32_t status_t;

typedef int32_t (*read_CallBack)(uint8_t *out, uint32_t rlen, uint32_t offset);
typedef int32_t (*write_CallBack)(uint8_t *in, uint32_t wlen, uint32_t offset);
typedef int32_t (*Rand_CallBack)(void *p_rng, uint8_t *rand, uint32_t randlen);

#define SPARAM_MAX_LEN   48

typedef struct
{
	int32_t command;
	int32_t result;
	int32_t index;
	int32_t permission;
	int32_t sparam_size;
    uint8_t sparam[SPARAM_MAX_LEN];
} DK_Cmd_Meg;


typedef enum
{
	INGEEK_SIGNATURE,
	INGEEK_VERIFY,
}OPERAE_T;

extern status_t ingeek_se_init(uint8_t s_type);

extern status_t ingeek_se_final(void);

extern status_t ingeek_push_info(uint8_t *input, uint32_t ilen, uint8_t* output, uint32_t* olen);

extern status_t ingeek_push_auth(uint8_t *input, uint32_t ilen, uint8_t* output, uint32_t* olen);

extern status_t ingeek_push_session(uint8_t *input, uint32_t ilen, uint8_t* output, uint32_t* olen);

extern status_t ingeek_command_input_action(uint8_t *input, uint32_t ilen, DK_Cmd_Meg* Can_Packet);

extern status_t ingeek_command_output_action(DK_Cmd_Meg* Can_Packet, uint8_t* output, uint32_t* olen);

extern status_t ingeek_message_output_action(DK_Cmd_Meg* Can_Packet, uint8_t* output, uint32_t* outlen);

extern status_t ingeek_signature_verify(uint8_t *input, uint32_t ilen, uint8_t *output, uint32_t* olen, OPERAE_T Dir);

extern status_t ingeek_sync_rtc_time(uint8_t *input, uint32_t ilen);

extern uint8_t  ingeek_get_sec_status(void);

extern uint8_t* ingeek_get_version(void);

extern uint32_t ingeek_get_pair_df(void);

extern status_t ingeek_get_seid(uint8_t* output, uint32_t* olen);

extern uint32_t ingeek_calculate_vinhash(uint8_t* output, uint32_t* outlen);

extern void ingeek_set_callback(read_CallBack, write_CallBack, Rand_CallBack);

extern int ingeek_get_base_info(uint32_t *phone_type, uint8_t* phone_number, uint32_t *VKID);

//extern uint8_t*ingeek_get_permission(void);

typedef enum
{
	/*SE SDK CODE*/
    HsmStatus_Success = 0,		//0x00   成功
	HsmStatus_Sumcheckfail, 	//0x01 CRC检验失败
	HsmStatus_InvalidArgument,	//0x02   输入无效参数
	HsmStatus_NO9000,    		//0x03 SE 返回非9000，出错
	HsmStatus_Selectfail, 		//0x04   应用选择失败
	HsmStatus_Personalized, 	//0x05   应用选择返回状态不对
	HsmStatus_Statusfail,   	//0x06 SDK状态出错
	HsmStatus_Apdupackfail, 	//0x07   打包APDU指令出错
	HsmStatus_Spitimeout,		//0x08 SPI非空闲状态
	HsmStatus_ATPFail,			//0x09 SE复位应答失败
	HsmStatus_NoApdutype,		//0x0A 不支持APDU指令类型
	HsmStatus_VCKTimefail,		//VCK有效期校验失败
	HsmStatus_RTCGetTimefail,	//RTC获取时间失败
	HsmStatus_RTCSetTimefail,	//RTC设置时间失败
	HsmStatus_T1InvalidArgument=-1, //T=1入参有误
	HsmStatus_T1Carddead=-2,  	  //SE无响应，需要重新复位
	HsmStatus_SPIfail=-3,         //SPI接口返回异常
	HsmStatus_T1buffover=-4,  	  //T=1 传入参数越界
	HsmStatus_T1Resyncs=-5,       //T=1 resyncs 次数达到上限
}SDKCode;

#define SE_PROD_TEST

#if defined(SE_PROD_TEST)
enum {TEST_OK,TEST_SELECT_ERR,TEST_SEID_ERR,TEST_SEIDLEN_ERR,TEST_VERIFY_ERR};
status_t ingeek_se_prod_test(uint8_t *input, uint32_t ilen, uint8_t seid[16]);
#endif


//void RTC_test(void);
#if defined(__cplusplus)
}
#endif /*_cplusplus*/


#endif /* DIGITAL_KEY_API_H_ */




