//
//================================================================
// Proposed AES CTR/CBC-MAC mode test vector generation
//
// 11-02-001r2-I-AES-Encryption & Authentication-Using-CTR-Mode-with-CBC-MAC
//
// Author:  Doug Whiting, Hifn  (dwhiting@hifn.com)
//
// This code is released to the public domain, on an as-is basis.
// 
//================================================================
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "aes_defs.h"       // AES calling interface
#include "aes_vect.h"       // NIST AES test vectors

#define Lo8(x) ((u08b) ((x) & 0xFF))

typedef int BOOL;           // boolean

enum
    {
    BLK_SIZE    =   16,     // # octets in an AES block
    MAX_PACKET  =   3*512,  // largest packet size
    N_RESERVED  =   0,      // reserved nonce octet value
    A_DATA      =   0x40,   // the Adata bit in the flags
    M_SHIFT     =   3,      // how much to shift the 3-bit M field
    L_SHIFT     =   0,      // how much to shift the 3-bit L field
    L_SIZE      =   2       // size of the l(m) length field (in octets)
    };

union block                 // AES cipher block
    {
    u32b  x[BLK_SIZE/4];    // access as 8-bit octets or 32-bit words
    u08b  b[BLK_SIZE];
    };

struct packet
    {
    BOOL    encrypted;      // TRUE if encrypted
    u08b    TA[6];          // xmit address
    int     micLength;      // # octets of MIC appended to plaintext (M)
    int     clrCount;       // # cleartext octets covered by MIC
    u32b    pktNum[2];      // unique packet sequence number (like WEP IV)
    block   key;            // the encryption key (K)
    int     length;         // # octets in data[]
    u08b    data[MAX_PACKET+2*BLK_SIZE];    // packet contents
    };

struct
    {
    int     cnt;            // how many words left in ct
    block   ptCntr;         // the counter input
    block   ct;             // the ciphertext (prng output)
    } prng;

void InitRand(u32b   seed)
    {
    memset(prng.ptCntr.b,0,BLK_SIZE);
    prng.ptCntr.x[(BLK_SIZE/4)-1]=bswap(seed*17);
    prng.cnt=0;             // the pump is dry
    }

// prng: does not use C rand(), so this should be repeatable across platforms
u32b   Random32(void)
    {
    int  i;
	u32b x;
    if (prng.cnt == 0)
        {   // use whatever key is currently defined
        prng.cnt=BLK_SIZE/4;
		for (i=0;i<16;i++)
			{
			prng.ptCntr.b[i]++;		// ripple carry
			if (prng.ptCntr.b[i])
				break;
			}
        AES_Encrypt(prng.ptCntr.x,prng.ct.x);
        }
    --prng.cnt;
	for (i=x=0;i<4;i++)				// return in big-endian format
		x = (x << 8) + prng.ct.b[4*prng.cnt+i];
    return x;
    }

// display a block
void ShowBlock(const block *blk,const char *prefix,const char *suffix,int a)
    {
    int i,blkSize = BLK_SIZE;
    printf(prefix,a);
    if (suffix == NULL) { suffix = "\n"; blkSize = a; }
    for (i=0;i<blkSize;i++)
        printf("%02X%s",blk->b[i],((i&3)==3)?"  ":" ");
    printf(suffix);
    }

void ShowAddr(const packet *p)
    {
    int i;

    printf("      TA = ");
    for (i=0;i<6;i++) printf("%02X%s",p->TA[i],(i==3)?"  ":" ");
    printf("  48-bit pktNum = %04X.%08X\n",p->pktNum[1],p->pktNum[0]);
    }

// display a packet
void ShowPacket(const packet *p,const char *pComment,int a)
    {
    int i;

    printf("Total packet length = %4d. ",p->length);
    printf(pComment,a);
    if (p->encrypted) printf("[Encrypted]");
    for (i=0;i<p->length;i++)
        {
        if ((i & 15) == 0) printf("\n%11s","");
        printf("%02X%s",p->data[i],((i&3)==3)?"  ":" ");
        }
    printf("\n");
    }

// make sure that encrypt/decrypt work according to NIST vectors
void Validate_NIST_AES_Vectors(int verbose)
    {
    int     i;
    block   key,pt,ct,rt;

    printf("AES KAT Vectors:\n");   // known-answer tests
    // variable text (fixed-key) tests
    memcpy(key.b,VT_key,BLK_SIZE);
    AES_SetKey(key.x,BLK_SIZE*8);
    for (i=0;i<sizeof(VT_pt_ct_pairs);i+=2*BLK_SIZE)
         {
         memcpy(pt.b,VT_pt_ct_pairs+i,BLK_SIZE);
         AES_Encrypt(pt.x,ct.x);
         if (memcmp(ct.x,VT_pt_ct_pairs+i+BLK_SIZE,BLK_SIZE))
             {
             printf("Vector miscompare at VT test #%d",i);
             exit(1);
             }
         AES_Decrypt(ct.x,rt.x);    // sanity check on decrypt
         if (memcmp(pt.b,rt.b,BLK_SIZE))
             {
             printf("Decrypt miscompare at VT test #%d",i);
             exit(1);
             }
         if (verbose)   // only do a little if we're "debugging"
             {  printf("\n");    break; }
         else if (i==0)
             {          //  display the first vector
             ShowBlock(&key,"Key:       ","\n",0);
             ShowBlock(&pt ,"PT:        ","\n",0);
             ShowBlock(&ct ,"CT:        ","\n\n",0);
             }
         }

    // variable key (fixed-text) tests
    memcpy(pt.b,VK_pt,BLK_SIZE);
    for (i=0;i<sizeof(VK_key_ct_pairs);i+=2*BLK_SIZE)
         {
         memcpy(key.b,VK_key_ct_pairs+i,BLK_SIZE);
         AES_SetKey(key.x,BLK_SIZE*8);
         AES_Encrypt(pt.x,ct.x);
         if (memcmp(ct.x,VK_key_ct_pairs+i+BLK_SIZE,BLK_SIZE))
             {
             printf("Vector miscompare at VK test #%d",i);
             exit(1);
             }
         AES_Decrypt(ct.x,rt.x);    // sanity check on decrypt
         if (memcmp(pt.b,rt.b,BLK_SIZE))
             {
             printf("Decrypt miscompare at VK test #%d",i);
             exit(1);
             }
         if (verbose)   // only do a little if we're "debugging"
             {  printf("\n");    break; }
         else if (i==0)
             {          //  display the first vector
             ShowBlock(&key,"Key:       ","\n",0);
             ShowBlock(&pt ,"PT:        ","\n",0);
             ShowBlock(&ct ,"CT:        ","\n\n",0);
             }
         }
    printf("NIST AES Vectors: OK\n");   // if we got here, it's all cool
    }

// assumes AES_SetKey is called elsewhere
void Generate_CTR_CBC_Vector(packet *p,int verbose)
    {
    int     i,j,len,needPad,blkNum;
    block   m,x,T;
    assert(p->length    >= p->clrCount && p->length    <= MAX_PACKET);
    assert(p->micLength >  0           && p->micLength <= BLK_SIZE);
    len = p->length - p->clrCount;      // l(m)

    ShowPacket(p,"[Input (%d cleartext header octets)]",p->clrCount);
    
    //---- generate the first AES block for CBC-MAC
    m.b[ 0] =(u08b) ((L_SIZE-1) << L_SHIFT) +      // flags octet
                     ((p->clrCount)?A_DATA:0) + (((p->micLength-2)/2 << M_SHIFT));
    m.b[ 1] = N_RESERVED;               // reserved nonce octet 
    m.b[ 2] = Lo8(p->pktNum[1] >> 8);   // 48 bits of packet number ("IV")
    m.b[ 3] = Lo8(p->pktNum[1]);
    m.b[ 4] = Lo8(p->pktNum[0] >>24);
    m.b[ 5] = Lo8(p->pktNum[0] >>16);
    m.b[ 6] = Lo8(p->pktNum[0] >> 8);
    m.b[ 7] = Lo8(p->pktNum[0]);
    m.b[ 8] = p->TA[0];                 // 48 bits of transmitter address
    m.b[ 9] = p->TA[1];
    m.b[10] = p->TA[2];
    m.b[11] = p->TA[3];
    m.b[12] = p->TA[4];
    m.b[13] = p->TA[5];
    m.b[14] = Lo8(len >> 8);            // l(m) field
    m.b[15] = Lo8(len);

    //---- compute the CBC-MAC tag (MIC)
    AES_Encrypt(m.x,x.x);               // produce the CBC IV
    ShowBlock(&m,"CBC IV in: ","\n",0);
    if (verbose) ShowBlock(&x,"CBC IV out:","\n",0);
    j=0;                                // j = octet counter inside the AES block
    if (p->clrCount)                    // is there a header?
        {                               // if so, "insert" length field: l(a)
        assert(p->clrCount < 0xFFF0);   // [don't handle larger cases (yet)]
        x.b[j++]^=(p->clrCount >> 8) & 0xFF;
        x.b[j++]^= p->clrCount       & 0xFF;
        }
    for (i=blkNum=0;i<p->length;i++)    // do the CBC-MAC processing
        {
        x.b[j++] ^= p->data[i];         // perform the CBC xor
        needPad = (i == p->clrCount-1) || (i == p->length-1);
        if ((j == BLK_SIZE) || needPad) // full block, or hit pad boundary
            {
            if (verbose) ShowBlock(&x,"After xor: ",
                           (i >= p->clrCount) ? " [msg]\n" : " [hdr]\n",blkNum);
            AES_Encrypt(x.x,x.x);       // encrypt the CBC-MAC block, in place
            if (verbose) ShowBlock(&x,"After AES: ","\n",blkNum);
            blkNum++;                   // count the blocks
            j = 0;                      // the block is now empty
            }
        }      
    memcpy(T.b,x.b,p->micLength);       // save the MIC tag 
    ShowBlock(&T,"MIC tag  : ",NULL,p->micLength);

    //---- encrypt the data packet using CTR mode
    m.b[0] &= ~ (A_DATA | (7<<M_SHIFT));// clear flag fields for counter mode
    for (i=blkNum=0;i+p->clrCount < p->length;i++)
        {
        if ((i % BLK_SIZE) == 0)
            {                           // generate new keystream block
            blkNum++;                   // start data with block #1
            m.b[14] = blkNum/256;
            m.b[15] = blkNum%256;
            AES_Encrypt(m.x,x.x);       // then encrypt the counter
            if (verbose && i==0) ShowBlock(&m,"CTR Start: ","\n",0);
            if (verbose) ShowBlock(&x,"CTR[%04X]: " ,"\n",blkNum);
            }
        p->data[i+p->clrCount] ^= x.b[i % BLK_SIZE];    // merge in the keystream
        }

    //---- truncate, encrypt, and append MIC to packet
    m.b[14] = m.b[15] = 0;              // use block counter value zero for tag
    AES_Encrypt(m.x,x.x);               // encrypt the counter
    if (verbose) ShowBlock(&x,"CTR[MIC ]: " ,NULL,p->micLength);
    for (i=0;i<p->micLength;i++)  
        p->data[p->length+i]=T.b[i] ^ x.b[i];
    p->length+=p->micLength;            // adjust packet length accordingly

    p->encrypted = 1;
    ShowPacket(p,"",0);                 // show the final encrypted packet
    }

int main(int argc,char *argv[])
    {
    int     i,j,k,len,pktNum,seed;
    packet  p;

    seed = (argc > 1) ? atoi(argv[1]) : (int) time(NULL);
    InitRand(seed);
    printf("%s C compiler [%s %s].%s\nRandom seed = %d\n",
           COMPILER_ID,__DATE__,__TIME__,
#ifdef LITTLE_ENDIAN
	   " Little-endian.",
#else
	   " Big-endian.",
#endif
	   seed);

    // first, make sure that our AES code matches NIST KAT vectors
    Validate_NIST_AES_Vectors(_VERBOSE_);

    // generate CTR-CBC vectors for various parameter settings
    for (k=pktNum=0;k<2;k++)
        {   // k==1 --> random vectors. k==0 --> "visually simple" vectors
        for (i=0;i<BLK_SIZE  ;i++)          
            p.key.b[i] = (k) ? (u08b) Random32() & 0xFF : i + 0xC0;
        for (i=0;i<6;i++)          
            p.TA[i]    = (k) ? (u08b) Random32() & 0xFF : i + 0xA0;
        AES_SetKey(p.key.x,BLK_SIZE*8);     // run the key schedule

        // now generate the vectors
        for (p.micLength  = 8;p.micLength  <12;p.micLength+=2)
        for (p.clrCount   = 8;p.clrCount   <16;p.clrCount+=4)
        for (len          =32;len          <64;len*=2)
        for (i            =-1;i            < 2;i++)
            {
            p.pktNum[0] = (k) ? Random32()          : pktNum*0x01010101 + 0x03020100;
            p.pktNum[1] = (k) ? Random32() & 0xFFFF : 0;    // 48-bit IV
            p.length    = len+i;           // len+i is packet length
            p.encrypted = 0;
            assert(p.length <= MAX_PACKET);
            for (j=0;j<p.length;j++)        // generate random packet contents
                p.data[j]=(k) ? (u08b  ) Random32() & 0xFF : j;
            pktNum++;
            printf("=============== Packet Vector #%d ==================\n",pktNum);
            ShowBlock(&p.key ,"AES Key:   ","\n",0);
            ShowAddr (&p);
            Generate_CTR_CBC_Vector(&p,1);
            }
        }
    return 0;
    }
