#include <stdio.h>
#include "checksum.h"


/*
 * Returns LRC of data.
 */
unsigned int csum_lrc_compute(const unsigned char *in, size_t len, unsigned char *rc)
{
	unsigned char	lrc = 0;

	while (len--)
		lrc ^= *in++;

	if (rc)
		*rc = lrc;
	return 1;
}

/*
 * Compute CRC of data.
 */
static unsigned short UpdataCrc(unsigned char ch,unsigned short* lpwCrc)
{
	ch=(ch^(unsigned char)((*lpwCrc)&0x00FF));
	
	ch=(ch^(ch<<4));
	
	*lpwCrc=(*lpwCrc>>8)^((unsigned short)ch<<8)^((unsigned short)ch<<3)^((unsigned short)ch>>4);
	
	return (*lpwCrc);
}


unsigned int csum_crc_compute(const unsigned char*data,size_t len,unsigned char *rc)
{
    unsigned char  chBlock;
    unsigned short wCrc;
	int CRCType=CRC_B;
	
	switch(CRCType){
		case CRC_A:
			wCrc=0x6363;
			break;
		case CRC_B:
			wCrc=0xFFFF;
			break;
		default:
			return 0;
	}
	
	do{
		chBlock=*data++;
		UpdataCrc(chBlock,&wCrc);
	}while(--len);

	if(CRCType == CRC_B)
		wCrc = ~wCrc;
	
	if(rc){
		rc[1]=(unsigned char)((wCrc>>8)&0xFF);
		rc[0]=(unsigned char)(wCrc&0xFF);
	}
	
	return 2;
}
