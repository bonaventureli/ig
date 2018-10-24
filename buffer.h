#ifndef _BUFFER_H
#define _BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct ct_buf {
	unsigned char *		base;
	unsigned int		head, tail, size;
	unsigned int		overrun;
} ct_buf_t;

extern void *	ct_buf_head(ct_buf_t *);
extern void		ct_buf_init(ct_buf_t *, void *, int);
extern void		ct_buf_set(ct_buf_t *, void *, int);
extern int		ct_buf_get(ct_buf_t *, void *, int);
extern int		ct_buf_put(ct_buf_t *, const void *, int);
extern int		ct_buf_putc(ct_buf_t *, int);
extern unsigned int	ct_buf_avail(ct_buf_t *);


#ifdef __cplusplus
}
#endif

#endif
