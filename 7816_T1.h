#ifndef __7816_T1_H__
#define __7816_T1_H__


#include <stdint.h>
#include "buffer.h"

/* T=1 protocol constants */
#define T1_I_BLOCK		0x00
#define T1_R_BLOCK		0x80
#define T1_S_BLOCK		0xC0
#define T1_MORE_BLOCKS	0x20

/* I block */
#define T1_I_SEQ_SHIFT		6

/* R block */
#define T1_IS_ERROR(pcb)	((pcb) & 0x0F)
#define T1_EDC_ERROR		0x01
#define T1_OTHER_ERROR		0x02
#define T1_R_SEQ_SHIFT		4

/* S block stuff */
#define T1_S_IS_RESPONSE(pcb)	((pcb) & T1_S_RESPONSE)
#define T1_S_TYPE(pcb)			((pcb) & 0x0F)
#define T1_S_RESPONSE	0x20
#define T1_S_RESYNC		0x00
#define T1_S_IFS		0x01
#define T1_S_ABORT		0x02
#define T1_S_WTX		0x03
#define swap_nibbles(x) ( (x >> 4) | ((x & 0xF) << 4) )

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define NAD 0
#define PCB 1
#define LEN 2
#define TDATA 3

#define T1_BUFFER_SIZE		(3 + 254 + 2)

/* internal state, do not mess with it. */
/* should be != DEAD after reset/init */
enum {SENDING, RECEIVING, RESYNCH, DEAD};

enum
{
	IFD_PROTOCOL_RECV_TIMEOUT = 0x0000,
	IFD_PROTOCOL_T1_BLOCKSIZE,
	IFD_PROTOCOL_T1_CHECKSUM_CRC,
	IFD_PROTOCOL_T1_CHECKSUM_LRC,
	IFD_PROTOCOL_T1_IFSC,
	IFD_PROTOCOL_T1_IFSD,
	IFD_PROTOCOL_T1_STATE,
	IFD_PROTOCOL_T1_MORE
};


typedef enum
{
	T1_InvalidArgument = -1,
	T1_Card_dead = -2,

}ReCode;



typedef struct
{
	int		state;
	unsigned char	ns;	/* reader side */
	unsigned char	nr;	/* card side */
	unsigned int	ifsc;
	unsigned int	ifsd;
	unsigned char	wtx;
	unsigned int	retries;
	unsigned int	rc_bytes;
	unsigned int	(*checksum)(const uint8_t *, size_t, unsigned char *);
	char			more;	/* more data bit */
	unsigned char	previous_block[4];	/* to store the last R-block */
} t1_state_t;

extern int t1_transceive(t1_state_t *t1, unsigned int dad,
		const void *snd_buf, size_t snd_len,
		void *rcv_buf, size_t rcv_len);
extern int t1_init(t1_state_t *t1);
extern void t1_release(t1_state_t *t1);
extern int t1_set_param(t1_state_t *t1, int type, long value);
extern int t1_negotiate_ifsd(t1_state_t *t1, unsigned int dad, int ifsd);

#endif

