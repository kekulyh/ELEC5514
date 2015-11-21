
/* This is an independent implementation of the encryption algorithm:   */
/*                                                                      */
/*         RIJNDAEL by Joan Daemen and Vincent Rijmen                   */
/*                                                                      */
/* which is a candidate algorithm in the Advanced Encryption Standard   */
/* programme of the US National Institute of Standards and Technology.  */
/*                                                                      */
/* Copyright in this implementation is held by Dr B R Gladman but I     */
/* hereby give permission for its free direct or derivative use subject */
/* to acknowledgment of its origin and compliance with any conditions   */
/* that the originators of the algorithm place on its exploitation.     */
/*                                                                      */
/* Dr Brian Gladman (gladman@seven77.demon.co.uk) 14th January 1999     */

#include "aes_defs.h"

/*------------------ DLW debug code */
#if	_VERBOSE_
#include <stdio.h>
int		rNum;
void ShowBlk(const u32b *b,int final)
	{
	int  i,j;
	u32b x;
	u08b a;

	printf("%s %2d: ",(final) ? "Final" : "Round",rNum++);
	for (i=0;i<4;i++)
		{
		printf(" ");
		x = b[i];			/* always used internally as "little-endian" */
		for (j=0;j<4;j++)
			{
			a = byte(x,j);
			printf(" %02X",a);
			}
		}
	printf("\n");
	}

void ShowKeySched(const u32b *key,int cnt,const char *hdrMsg)
	{
	int  i,j;
	u32b x;
	u08b a;

	printf("%s\n",hdrMsg);
	for (i=0;i<4*cnt;i++)
		{
		x = key[i];		/* key always used as "little-endian" */
		printf(" ");
		for (j=0;j<4;j++)
			{
			a = byte(x,j);
			printf(" %02X",a);
			}
		if ((i%4) == 3) printf("\n");
		}
	}
#define	SetR(r)	{ rNum = r; }
#else
#define	ShowBlk(b,f)
#define	SetR(r)
#define	ShowKeySched(key,cnt,hdrMsg)
#endif
/*---------------- end of DLW debug */


#define LARGE_TABLES

u08b  pow_tab[256];
u08b  log_tab[256];
u08b  sbx_tab[256];
u08b  isb_tab[256];
u32b  rco_tab[ 10];
u32b  ft_tab[4][256];
u32b  it_tab[4][256];

#ifdef  LARGE_TABLES
  u32b  fl_tab[4][256];
  u32b  il_tab[4][256];
#endif

u32b  tab_gen = 0;

u32b  k_len;
u32b  e_key[60];
u32b  d_key[60];

#define ff_mult(a,b)    (a && b ? pow_tab[(log_tab[a] + log_tab[b]) % 255] : 0)

#define f_rn(bo, bi, n, k)                          \
    bo[n] =  ft_tab[0][byte(bi[n],0)] ^             \
             ft_tab[1][byte(bi[(n + 1) & 3],1)] ^   \
             ft_tab[2][byte(bi[(n + 2) & 3],2)] ^   \
             ft_tab[3][byte(bi[(n + 3) & 3],3)] ^ *(k + n)

#define i_rn(bo, bi, n, k)                          \
    bo[n] =  it_tab[0][byte(bi[n],0)] ^             \
             it_tab[1][byte(bi[(n + 3) & 3],1)] ^   \
             it_tab[2][byte(bi[(n + 2) & 3],2)] ^   \
             it_tab[3][byte(bi[(n + 1) & 3],3)] ^ *(k + n)

#ifdef LARGE_TABLES

#define ls_box(x)                \
    ( fl_tab[0][byte(x, 0)] ^    \
      fl_tab[1][byte(x, 1)] ^    \
      fl_tab[2][byte(x, 2)] ^    \
      fl_tab[3][byte(x, 3)] )

#define f_rl(bo, bi, n, k)                          \
    bo[n] =  fl_tab[0][byte(bi[n],0)] ^             \
             fl_tab[1][byte(bi[(n + 1) & 3],1)] ^   \
             fl_tab[2][byte(bi[(n + 2) & 3],2)] ^   \
             fl_tab[3][byte(bi[(n + 3) & 3],3)] ^ *(k + n)

#define i_rl(bo, bi, n, k)                          \
    bo[n] =  il_tab[0][byte(bi[n],0)] ^             \
             il_tab[1][byte(bi[(n + 3) & 3],1)] ^   \
             il_tab[2][byte(bi[(n + 2) & 3],2)] ^   \
             il_tab[3][byte(bi[(n + 1) & 3],3)] ^ *(k + n)

#else

#define ls_box(x)                            \
    ((u32b)sbx_tab[byte(x, 0)] <<  0) ^    \
    ((u32b)sbx_tab[byte(x, 1)] <<  8) ^    \
    ((u32b)sbx_tab[byte(x, 2)] << 16) ^    \
    ((u32b)sbx_tab[byte(x, 3)] << 24)

#define f_rl(bo, bi, n, k)                                      \
    bo[n] = (u32b)sbx_tab[byte(bi[n],0)] ^                    \
        rotl(((u32b)sbx_tab[byte(bi[(n + 1) & 3],1)]),  8) ^  \
        rotl(((u32b)sbx_tab[byte(bi[(n + 2) & 3],2)]), 16) ^  \
        rotl(((u32b)sbx_tab[byte(bi[(n + 3) & 3],3)]), 24) ^ *(k + n)

#define i_rl(bo, bi, n, k)                                      \
    bo[n] = (u32b)isb_tab[byte(bi[n],0)] ^                    \
        rotl(((u32b)isb_tab[byte(bi[(n + 3) & 3],1)]),  8) ^  \
        rotl(((u32b)isb_tab[byte(bi[(n + 2) & 3],2)]), 16) ^  \
        rotl(((u32b)isb_tab[byte(bi[(n + 1) & 3],3)]), 24) ^ *(k + n)

#endif

void gen_tabs(void)
{   u32b  i, t;
    u08b  p, q;

    /* log and power tables for GF(2**8) finite field with  */
    /* 0x11b as modular polynomial - the simplest prmitive  */
    /* root is 0x11, used here to generate the tables       */

    for(i = 0,p = 1; i < 256; ++i)
    {
        pow_tab[i] = (u08b)p; log_tab[p] = (u08b)i;

        p = p ^ (p << 1) ^ (p & 0x80 ? 0x01b : 0);
    }

    log_tab[1] = 0; p = 1;

    for(i = 0; i < 10; ++i)
    {
        rco_tab[i] = p; 

        p = (p << 1) ^ (p & 0x80 ? 0x1b : 0);
    }

    /* note that the affine byte transformation matrix in   */
    /* rijndael specification is in big endian format with  */
    /* bit 0 as the most significant bit. In the remainder  */
    /* of the specification the bits are numbered from the  */
    /* least significant end of a byte.                     */

    for(i = 0; i < 256; ++i)
    {   
        p = (i ? pow_tab[255 - log_tab[i]] : 0); q = p; 
        q = (q >> 7) | (q << 1); p ^= q; 
        q = (q >> 7) | (q << 1); p ^= q; 
        q = (q >> 7) | (q << 1); p ^= q; 
        q = (q >> 7) | (q << 1); p ^= q ^ 0x63; 
        sbx_tab[i] = (u08b)p; isb_tab[p] = (u08b)i;
    }

    for(i = 0; i < 256; ++i)
    {
        p = sbx_tab[i]; 

#ifdef  LARGE_TABLES        
        
        t = p; fl_tab[0][i] = t;
        fl_tab[1][i] = rotl(t,  8);
        fl_tab[2][i] = rotl(t, 16);
        fl_tab[3][i] = rotl(t, 24);
#endif
        t = ((u32b)ff_mult(2, p)) |
            ((u32b)p <<  8) |
            ((u32b)p << 16) |
            ((u32b)ff_mult(3, p) << 24);
        
        ft_tab[0][i] = t;
        ft_tab[1][i] = rotl(t,  8);
        ft_tab[2][i] = rotl(t, 16);
        ft_tab[3][i] = rotl(t, 24);

        p = isb_tab[i]; 

#ifdef  LARGE_TABLES        
        
        t = p; il_tab[0][i] = t; 
        il_tab[1][i] = rotl(t,  8); 
        il_tab[2][i] = rotl(t, 16); 
        il_tab[3][i] = rotl(t, 24);
#endif 
        t = ((u32b)ff_mult(14, p)) |
            ((u32b)ff_mult( 9, p) <<  8) |
            ((u32b)ff_mult(13, p) << 16) |
            ((u32b)ff_mult(11, p) << 24);
        
        it_tab[0][i] = t; 
        it_tab[1][i] = rotl(t,  8); 
        it_tab[2][i] = rotl(t, 16); 
        it_tab[3][i] = rotl(t, 24); 

#if _VERBOSE_
		if (i<4)	/* helpful for debugging on new platform */
			{		/* (compare with results from known platform) */
			if (i==0)
				printf("%8s   : %08X %08X %08X %08X\n","rco_tab",
					   rco_tab[0],rco_tab[1],rco_tab[2],rco_tab[3]);
#define _ShowTab(tName)	printf("%8s[%d]: %08X %08X %08X %08X\n",#tName,i,	\
				   tName[0][i],tName[1][i],tName[2][i],tName[3][i]);
			_ShowTab(it_tab);
			_ShowTab(ft_tab);
#ifdef LARGE_TABLES
			_ShowTab(il_tab);
			_ShowTab(fl_tab);
#endif
			}
#endif
    }

    tab_gen = 1;
};

#define star_x(x) (((x) & 0x7f7f7f7f) << 1) ^ ((((x) & 0x80808080) >> 7) * 0x1b)

#define imix_col(y,x)       \
    u   = star_x(x);        \
    v   = star_x(u);        \
    w   = star_x(v);        \
    t   = w ^ (x);          \
   (y)  = u ^ v ^ w;        \
   (y) ^= rotr(u ^ t,  8) ^ \
          rotr(v ^ t, 16) ^ \
          rotr(t,24)

/* initialise the key schedule from the user supplied key   */

#define loop4(i)                                    \
{   t = ls_box(rotr(t,  8)) ^ rco_tab[i];           \
    t ^= e_key[4 * i];     e_key[4 * i + 4] = t;    \
    t ^= e_key[4 * i + 1]; e_key[4 * i + 5] = t;    \
    t ^= e_key[4 * i + 2]; e_key[4 * i + 6] = t;    \
    t ^= e_key[4 * i + 3]; e_key[4 * i + 7] = t;    \
}

#define loop6(i)                                    \
{   t = ls_box(rotr(t,  8)) ^ rco_tab[i];           \
    t ^= e_key[6 * i];     e_key[6 * i + 6] = t;    \
    t ^= e_key[6 * i + 1]; e_key[6 * i + 7] = t;    \
    t ^= e_key[6 * i + 2]; e_key[6 * i + 8] = t;    \
    t ^= e_key[6 * i + 3]; e_key[6 * i + 9] = t;    \
    t ^= e_key[6 * i + 4]; e_key[6 * i + 10] = t;   \
    t ^= e_key[6 * i + 5]; e_key[6 * i + 11] = t;   \
}

#define loop8(i)                                    \
{   t = ls_box(rotr(t,  8)) ^ rco_tab[i];           \
    t ^= e_key[8 * i];     e_key[8 * i + 8] = t;    \
    t ^= e_key[8 * i + 1]; e_key[8 * i + 9] = t;    \
    t ^= e_key[8 * i + 2]; e_key[8 * i + 10] = t;   \
    t ^= e_key[8 * i + 3]; e_key[8 * i + 11] = t;   \
    t  = e_key[8 * i + 4] ^ ls_box(t);              \
    e_key[8 * i + 12] = t;                          \
    t ^= e_key[8 * i + 5]; e_key[8 * i + 13] = t;   \
    t ^= e_key[8 * i + 6]; e_key[8 * i + 14] = t;   \
    t ^= e_key[8 * i + 7]; e_key[8 * i + 15] = t;   \
}

u32b *AES_SetKey(const u32b in_key[], const u32b key_len)
{   u32b  i, t, u, v, w;

    if(!tab_gen)
        gen_tabs();

    k_len = (key_len + 31) / 32;

    for (i=0;i<k_len;i++)
		e_key[i] = bswap(in_key[i]);
    t = e_key[k_len-1];

    switch(k_len)
    {
        case 4: for(i = 0; i < 10; ++i) 
                    loop4(i);
                break;

        case 6: for(i = 0; i < 8; ++i) 
                    loop6(i);
                break;

        case 8: for(i = 0; i < 7; ++i) 
                    loop8(i);
                break;
    }

    d_key[0] = e_key[0]; d_key[1] = e_key[1];
    d_key[2] = e_key[2]; d_key[3] = e_key[3];

    for(i = 4; i < 4 * k_len + 24; ++i)
	{
		imix_col(d_key[i], e_key[i]);
    }
	ShowKeySched(e_key,4,"Encryption key schedule:");
	ShowKeySched(d_key,4,"Decryption key schedule:");

    return e_key;
};

/* encrypt a block of text  */
#define f_nround(bo, bi, k) \
    f_rn(bo, bi, 0, k);     \
    f_rn(bo, bi, 1, k);     \
    f_rn(bo, bi, 2, k);     \
    f_rn(bo, bi, 3, k);     \
	ShowBlk(bo,0);			\
    k += 4

#define f_lround(bo, bi, k) \
    f_rl(bo, bi, 0, k);     \
    f_rl(bo, bi, 1, k);     \
    f_rl(bo, bi, 2, k);     \
    f_rl(bo, bi, 3, k);		\
	ShowBlk(bo,1);

void AES_Encrypt(const u32b in_blk[4], u32b out_blk[4])
{   u32b  b0[4], b1[4], *kp;

    b0[0] = bswap(in_blk[0]) ^ e_key[0];
    b0[1] = bswap(in_blk[1]) ^ e_key[1];
    b0[2] = bswap(in_blk[2]) ^ e_key[2];
    b0[3] = bswap(in_blk[3]) ^ e_key[3];

	SetR(1); ShowBlk(b0,0);

    kp = e_key + 4;

    if(k_len > 6)
    {
        f_nround(b1, b0, kp); f_nround(b0, b1, kp);
    }

    if(k_len > 4)
    {
        f_nround(b1, b0, kp); f_nround(b0, b1, kp);
    }

    f_nround(b1, b0, kp); f_nround(b0, b1, kp);
    f_nround(b1, b0, kp); f_nround(b0, b1, kp);
    f_nround(b1, b0, kp); f_nround(b0, b1, kp);
    f_nround(b1, b0, kp); f_nround(b0, b1, kp);
    f_nround(b1, b0, kp); f_lround(b0, b1, kp);

    out_blk[0] = bswap(b0[0]);
    out_blk[1] = bswap(b0[1]);
    out_blk[2] = bswap(b0[2]);
    out_blk[3] = bswap(b0[3]);
};

/* decrypt a block of text  */

#define i_nround(bo, bi, k) \
    i_rn(bo, bi, 0, k);     \
    i_rn(bo, bi, 1, k);     \
    i_rn(bo, bi, 2, k);     \
    i_rn(bo, bi, 3, k);     \
    k -= 4

#define i_lround(bo, bi, k) \
    i_rl(bo, bi, 0, k);     \
    i_rl(bo, bi, 1, k);     \
    i_rl(bo, bi, 2, k);     \
    i_rl(bo, bi, 3, k)

void AES_Decrypt(const u32b in_blk[4], u32b out_blk[4])
{   u32b  b0[4], b1[4], *kp;

    b0[0] = bswap(in_blk[0]) ^ e_key[4 * k_len + 24];
    b0[1] = bswap(in_blk[1]) ^ e_key[4 * k_len + 25];
    b0[2] = bswap(in_blk[2]) ^ e_key[4 * k_len + 26];
    b0[3] = bswap(in_blk[3]) ^ e_key[4 * k_len + 27];

    kp = d_key + 4 * (k_len + 5);

    if(k_len > 6)
    {
        i_nround(b1, b0, kp); i_nround(b0, b1, kp);
    }

    if(k_len > 4)
    {
        i_nround(b1, b0, kp); i_nround(b0, b1, kp);
    }

    i_nround(b1, b0, kp); i_nround(b0, b1, kp);
    i_nround(b1, b0, kp); i_nround(b0, b1, kp);
    i_nround(b1, b0, kp); i_nround(b0, b1, kp);
    i_nround(b1, b0, kp); i_nround(b0, b1, kp);
    i_nround(b1, b0, kp); i_lround(b0, b1, kp);

    out_blk[0] = bswap(b0[0]);
    out_blk[1] = bswap(b0[1]);
    out_blk[2] = bswap(b0[2]);
    out_blk[3] = bswap(b0[3]);
};

