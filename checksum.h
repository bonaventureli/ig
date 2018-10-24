#ifndef _CHECKSUM_H__
#define _CHECKSUM_H__

#ifdef __cplusplus
extern "C" {
#endif

#define CRC_A 1
#define CRC_B 2

extern unsigned int	csum_lrc_compute(const unsigned char *, size_t, unsigned char *);
extern unsigned int	csum_crc_compute(const unsigned char *, size_t, unsigned char *);

#ifdef __cplusplus
}
#endif

#endif

