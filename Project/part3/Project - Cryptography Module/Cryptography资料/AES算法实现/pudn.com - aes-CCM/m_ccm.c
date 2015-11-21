#include <stdio.h>
#define MAX_PACKET 128
#define L 2
#define M 8
typedef unsigned int uint ;
typedef unsigned char  word8;	
typedef unsigned short word16;	
typedef unsigned long  word32;

int rijndaelKeySched (word8 k[4][4], word8 rk[10+1][4][4]);
int rijndaelEncrypt (word8 a[4][4],  word8 rk[10+1][4][4]);
void cbc_mac_get_T (word8 Bi[MAX_PACKET+2*16], word8 cnt, word8 out[M]);
void ctr_key_stream(word8 Ai[MAX_PACKET+16], word8 cnt, word8 k_stream[MAX_PACKET+16]);
void main_aes(uint first_key[],uint datain[],int enc_dec,uint dataout[]) ;
word8 mul(word8 a, word8 b);
void KeyAddition(word8 a[4][4], word8 rk[4][4], word8 BC);
void ShiftRow(word8 a[4][4], word8 d, word8 BC);
void Substitution(word8 a[4][4], word8 box[256], word8 BC);
void MixColumn(word8 a[4][4], word8 BC);



static word8 shifts[3][4][2] = {
   0, 0,
   1, 3,
   2, 2,
   3, 1,
   
   0, 0,
   1, 5,
   2, 4,
   3, 3,
   
   0, 0,
   1, 7,
   3, 5,
   4, 4
}; 

word8 Logtable[256] = {
  0,   0,  25,   1,  50,   2,  26, 198,  75, 199,  27, 104,  51, 238, 223,   3, 
100,   4, 224,  14,  52, 141, 129, 239,  76, 113,   8, 200, 248, 105,  28, 193, 
125, 194,  29, 181, 249, 185,  39, 106,  77, 228, 166, 114, 154, 201,   9, 120, 
101,  47, 138,   5,  33,  15, 225,  36,  18, 240, 130,  69,  53, 147, 218, 142, 
150, 143, 219, 189,  54, 208, 206, 148,  19,  92, 210, 241,  64,  70, 131,  56, 
102, 221, 253,  48, 191,   6, 139,  98, 179,  37, 226, 152,  34, 136, 145,  16, 
126, 110,  72, 195, 163, 182,  30,  66,  58, 107,  40,  84, 250, 133,  61, 186, 
 43, 121,  10,  21, 155, 159,  94, 202,  78, 212, 172, 229, 243, 115, 167,  87, 
175,  88, 168,  80, 244, 234, 214, 116,  79, 174, 233, 213, 231, 230, 173, 232, 
 44, 215, 117, 122, 235,  22,  11, 245,  89, 203,  95, 176, 156, 169,  81, 160, 
127,  12, 246, 111,  23, 196,  73, 236, 216,  67,  31,  45, 164, 118, 123, 183, 
204, 187,  62,  90, 251,  96, 177, 134,  59,  82, 161, 108, 170,  85,  41, 157, 
151, 178, 135, 144,  97, 190, 220, 252, 188, 149, 207, 205,  55,  63,  91, 209, 
 83,  57, 132,  60,  65, 162, 109,  71,  20,  42, 158,  93,  86, 242, 211, 171, 
 68,  17, 146, 217,  35,  32,  46, 137, 180, 124, 184,  38, 119, 153, 227, 165, 
103,  74, 237, 222, 197,  49, 254,  24,  13,  99, 140, 128, 192, 247, 112,   7, 
};

word8 Alogtable[256] = {
  1,   3,   5,  15,  17,  51,  85, 255,  26,  46, 114, 150, 161, 248,  19,  53, 
 95, 225,  56,  72, 216, 115, 149, 164, 247,   2,   6,  10,  30,  34, 102, 170, 
229,  52,  92, 228,  55,  89, 235,  38, 106, 190, 217, 112, 144, 171, 230,  49, 
 83, 245,   4,  12,  20,  60,  68, 204,  79, 209, 104, 184, 211, 110, 178, 205, 
 76, 212, 103, 169, 224,  59,  77, 215,  98, 166, 241,   8,  24,  40, 120, 136, 
131, 158, 185, 208, 107, 189, 220, 127, 129, 152, 179, 206,  73, 219, 118, 154, 
181, 196,  87, 249,  16,  48,  80, 240,  11,  29,  39, 105, 187, 214,  97, 163, 
254,  25,  43, 125, 135, 146, 173, 236,  47, 113, 147, 174, 233,  32,  96, 160, 
251,  22,  58,  78, 210, 109, 183, 194,  93, 231,  50,  86, 250,  21,  63,  65, 
195,  94, 226,  61,  71, 201,  64, 192,  91, 237,  44, 116, 156, 191, 218, 117, 
159, 186, 213, 100, 172, 239,  42, 126, 130, 157, 188, 223, 122, 142, 137, 128, 
155, 182, 193,  88, 232,  35, 101, 175, 234,  37, 111, 177, 200,  67, 197,  84, 
252,  31,  33,  99, 165, 244,   7,   9,  27,  45, 119, 153, 176, 203,  70, 202, 
 69, 207,  74, 222, 121, 139, 134, 145, 168, 227,  62,  66, 198,  81, 243,  14, 
 18,  54,  90, 238,  41, 123, 141, 140, 143, 138, 133, 148, 167, 242,  13,  23, 
 57,  75, 221, 124, 132, 151, 162, 253,  28,  36, 108, 180, 199,  82, 246,   1, 
};

word8 S[256] = {
 99, 124, 119, 123, 242, 107, 111, 197,  48,   1, 103,  43, 254, 215, 171, 118, 
202, 130, 201, 125, 250,  89,  71, 240, 173, 212, 162, 175, 156, 164, 114, 192, 
183, 253, 147,  38,  54,  63, 247, 204,  52, 165, 229, 241, 113, 216,  49,  21, 
  4, 199,  35, 195,  24, 150,   5, 154,   7,  18, 128, 226, 235,  39, 178, 117, 
  9, 131,  44,  26,  27, 110,  90, 160,  82,  59, 214, 179,  41, 227,  47, 132, 
 83, 209,   0, 237,  32, 252, 177,  91, 106, 203, 190,  57,  74,  76,  88, 207, 
208, 239, 170, 251,  67,  77,  51, 133,  69, 249,   2, 127,  80,  60, 159, 168, 
 81, 163,  64, 143, 146, 157,  56, 245, 188, 182, 218,  33,  16, 255, 243, 210, 
205,  12,  19, 236,  95, 151,  68,  23, 196, 167, 126,  61, 100,  93,  25, 115, 
 96, 129,  79, 220,  34,  42, 144, 136,  70, 238, 184,  20, 222,  94,  11, 219, 
224,  50,  58,  10,  73,   6,  36,  92, 194, 211, 172,  98, 145, 149, 228, 121, 
231, 200,  55, 109, 141, 213,  78, 169, 108,  86, 244, 234, 101, 122, 174,   8, 
186, 120,  37,  46,  28, 166, 180, 198, 232, 221, 116,  31,  75, 189, 139, 138, 
112,  62, 181, 102,  72,   3, 246,  14,  97,  53,  87, 185, 134, 193,  29, 158, 
225, 248, 152,  17, 105, 217, 142, 148, 155,  30, 135, 233, 206,  85,  40, 223, 
140, 161, 137,  13, 191, 230,  66, 104,  65, 153,  45,  15, 176,  84, 187,  22, 
};

word32 rcon[30] = {    //RC[j] for key expansion
  0x01,0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab,
  0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d,
  0xfa, 0xef, 0xc5, 0x91, };


//word8 last_data[16]={00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00};/
//word8 initial_data[16]={0x01, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 03, 02, 01 ,0, 06, 0, 0};
word8 initial_key[16] = {0xC0, 0xC1, 0xC2, 0xC3,  0xC4, 0xC5, 0xC6, 0xC7,  0xC8, 0xC9, 0xCA, 0xCB,  0xCC, 0xCD, 0xCE, 0xCF};
word8 nounceN[15-L] = {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0x03, 02, 01, 00, 06};
word8 auth_data[16] = {0x00, 0x08, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};  //8 bytes addition authenrization data
word8 la = 10;
word8 msg[MAX_PACKET] = {0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e};
word8 lm = 23;

int aes_cnt=0;
int main()
{  
  word8 i, j;
  //printf("\n**** Key length is : %d\n",key_num) ;
  //printf("\n**** Data length is : %d\n",data_num) ;
  //word8 initial_data[16];
  //word8 last_data[16];     //for AES encryption
  //main_aes(initial_key,initial_data,1,last_data);   //for AES encryption

  word8 cnt_cbc_mac;
  word8 cnt_ctr;
  word8 Si[MAX_PACKET + 1*16];
  word8 tag_out[M];
  word8 Em_out[MAX_PACKET];
  word8 U_out[M];
  word8 Ciper_out[MAX_PACKET+M];
  word8 ctr_cbc_mac_b[MAX_PACKET + 2*16]={0};  //MPDU = 8x16 byts  plus B0, B1 , total 10 packets---MPDU<127bytes
  word8 cbc_mac_b0[16]={0};
  word8 cbc_mac_b1[16]={0};
  word8 ctr_a0[16]={0};

  
  
  cbc_mac_b0[0] = 0x59;
  for(i=1; i<14; i++)
      cbc_mac_b0[i] = nounceN[i-1] ;
  cbc_mac_b0[15] = lm;

  for(i=0; i<la; i++)
	  cbc_mac_b1[i] = auth_data[i];


  for(j=0; j<16; j++)ctr_cbc_mac_b[j] =  cbc_mac_b0[j];   //B0
  for(j=0; j<16; j++)ctr_cbc_mac_b[16+j] = cbc_mac_b1[j];//B1 
  for(i=2; i<10; i++)                                //Bi
     for(j=0; j<16; j++)
        ctr_cbc_mac_b[i*16+j] = msg[(i-2)*16+j];

  //CBC-MAC to get T
  cnt_cbc_mac = (la+lm)/16 + 1+1;
  cbc_mac_get_T(ctr_cbc_mac_b, cnt_cbc_mac, tag_out); 
  printf("mic tag out is:\n");
  for(i=0; i<M ;i++)
	  printf("%x ", tag_out[i]); 
  printf("\n"); 

  //CTR encryption
  //still use ctr_cbc_mac_b matrix to store Ai
    ctr_a0[0] = 0x01;   //Ai flags
    for(i=1; i<14; i++)
      ctr_a0[i] = nounceN[i-1] ;
	ctr_a0[14] = 0x00;
	ctr_a0[15] = 0x00; //CTR value
    for(j=0; j<16; j++)ctr_cbc_mac_b[j] =  ctr_a0[j];   //A0
	for(i=0; i<10; i++)                                 //Ai 
    {
	 ctr_a0[15] += 1; //CTR value
	 for(j=0; j<16; j++)
              ctr_cbc_mac_b[i*16+j] = ctr_a0[j] ;
    }
	cnt_ctr = lm/16 +  1 + 1;  
    // caculate Si--key stream
	ctr_key_stream(ctr_cbc_mac_b, cnt_ctr, Si);
  //(1)msg encryption
    for(i=1; i<lm; i++)
    {
	  Em_out[i] = msg[i] ^ Si[16+i];
      Ciper_out[i] = Em_out[i];
    }
  //(2)T encryption
    for(i=0; i<M; i++)
	{
		U_out[i] = tag_out[i] ^ Si[i];
        Ciper_out[lm+i] = U_out[i]; 
    }
	//result ciper text out
	printf("ciper result:\n");
	 for(i=0; i<(lm+M); i++)
		 printf("%x ", Ciper_out[i]);
	 printf("\n");

printf("/*************************************************************************************/\n");
//following is CCM decryption and authentication checking
     
		 


printf("count of aes = %d\n", aes_cnt);
  



  return 0 ;
}

void ctr_key_stream(word8 Ai[MAX_PACKET+16], word8 cnt, word8 k_stream[MAX_PACKET+16])
{
  word8 i,j;
  word8 aes_in[16];
  word8 aes_out[16];
  for(i=0; i<cnt; i++)
  {
    for(j=0; j<16; j++)  
    { 
	  aes_in[j]	= Ai[i*16+j];  
    }
	main_aes(initial_key,aes_in,1,aes_out) ;  
	for(j=0; j<16; j++)
	   k_stream[16*i+j] = aes_out[j];
  }
}

void cbc_mac_get_T(word8 Bi[160], word8 cnt, word8 out[M])
{
    word8 i,j;
	word8 aes_in[16];
	word8 aes_out[16]={0};
    for(i=0; i<cnt; i++)
	{
	  for(j=0; j<16; j++)
	  {
		  aes_in[j] = Bi[i*16+j]^aes_out[j];		
	  }

      main_aes(initial_key,aes_in,1,aes_out) ;
	}
     
	for(j=0; j<M; j++)
	{
		  out[j] = aes_out[j];
	}

}


void main_aes(word8 first_key[],word8 datain[],int enc_dec,word8 dataout[]) 
{
  int i,j ;
  word8 data_sta[4][4]={
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0
    } ;
  word8 initial_key[4][4]={ 
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,  
      } ;
  word8 keys[10+1][4][4] ;   //key  matrix after expansion
  
  printf("this is initial key:\n");
  for(i=0; i<16; i++)
	  printf("%x", first_key[i]);
  printf("\n");

  printf("this is datain:\n");
  for(i=0; i<16; i++)
    printf("%x", datain[i]);
  printf("\n");
  // give to data_sta[4][4] , initial_key[4][4]  and printf them 4x4

  for(j=0; j<4; j++)
	  for(i=0; i<4; i++)
	  {
	    data_sta[i][j] = datain[4*j+i];
		initial_key[i][j] = first_key[4*j+i];
	  }

  printf("\nFirst_key is : \n") ;
  for(i=0 ; i < 4 ; i++) 
    {
     for ( j=0 ; j < 4; j++)
      printf("  %x  ",initial_key[i][j]) ;
    printf("\n");
    }

  printf("Data_in is : \n") ; 
 for(i=0 ; i < 4 ; i++) 
   {
    for ( j=0 ; j < 4 ; j++)
       printf("  %x  ",data_sta[i][j]) ;
    printf("\n");
   }

   rijndaelKeySched ( initial_key , keys ) ;     //output expansion keys matrix
   
   if ( enc_dec == 1 )
        rijndaelEncrypt ( data_sta , keys ) ;	
	
   else
        //rijndaelDecrypt ( data_sta ,  keys ) ;  

  printf("Data after encry_decry is \n") ; 
  for(i=0 ; i < 4 ; i++) 
   {
    for ( j=0 ; j < 4; j++)
       printf("  %x  ",data_sta[i][j]) ;
     printf("\n");
    }
//give to dataout[] , and printf them
 printf("\nDataout is : \n") ;
  for(j=0; j<4; j++)
	  for(i=0; i<4; i++)
	  {
		  dataout[4*j + i] = data_sta[i][j];
	  printf("%x", dataout[4*j + i]);
      }
 printf("\n") ;
 aes_cnt++;

}

/************************************************************************/      
int rijndaelEncrypt (word8 a[4][4],  word8 rk[10+1][4][4])
{
	/* Encryption of one block. 
	 */
	int  r; 

	KeyAddition(a,rk[0],4); 

        /* ROUNDS-1 ordinary rounds
	 */
	for(r = 1; r < 10; r++) {
		Substitution(a,S,4);
		ShiftRow(a,0,4);
		MixColumn(a,4);
		KeyAddition(a,rk[r],4);
	}
	
	/* Last round is special: there is no MixColumn
	 */
	Substitution(a,S,4);
	ShiftRow(a,0,4);
	KeyAddition(a,rk[10],4);
        
	return 0;
}  

/************************************************************************/
word8 mul(word8 a, word8 b) {
   /* multiply two elements of GF(2^m)
    * needed for MixColumn and InvMixColumn
    */
	if (a && b) return Alogtable[(Logtable[a] + Logtable[b])%255];
	else return 0;
}
/************************************************************************/
void KeyAddition(word8 a[4][4], word8 rk[4][4], word8 BC) {
	/* Exor corresponding text input and round key input bytes
	 */
	int i, j;
	
	for(i = 0; i < 4; i++)
   		for(j = 0; j < BC; j++) a[i][j] ^= rk[i][j];
}
/************************************************************************/
void ShiftRow(word8 a[4][4], word8 d, word8 BC) {
	/* Row 0 remains unchanged
	 * The other three rows are shifted a variable amount
	 */
	word8 tmp[4];
	int i, j;
	
	for(i = 1; i < 4; i++) {
	    for(j = 0; j < BC; j++) tmp[j] = a[i][(j + shifts[0][i][d]) % BC];
	    for(j = 0; j < BC; j++) a[i][j] = tmp[j];
	}
}
/************************************************************************/
void Substitution(word8 a[4][4], word8 box[256], word8 BC) {
	/* Replace every byte of the input by the byte at that place
	 * in the nonlinear S-box
	 */
	int i, j;
	
	for(i = 0; i < 4; i++)
		for(j = 0; j < BC; j++) a[i][j] = box[a[i][j]] ;
}
/************************************************************************/   
void MixColumn(word8 a[4][4], word8 BC) {
        /* Mix the four bytes of every column in a linear way
	 */
	word8 b[4][4];
	int i, j;
		
	for(j = 0; j < BC; j++)
		for(i = 0; i < 4; i++)
			b[i][j] = mul(2,a[i][j])
				^ mul(3,a[(i + 1) % 4][j])
				^ a[(i + 2) % 4][j]
				^ a[(i + 3) % 4][j];
	for(i = 0; i < 4; i++)
		for(j = 0; j < BC; j++) a[i][j] = b[i][j];
}

/************************************************************************/   
int rijndaelKeySched (word8 k[4][4], word8 W[10+1][4][4]) {
	/* Calculate the necessary round keys
	 * The number of calculations depends on keyBits and blockBits
	 */
	//int KC, BC, ROUNDS;
	int i, j, t, rconpointer = 0;
	word8 tk[4][4];   



 	for(j = 0; j < 4; j++)
		for(i = 0; i < 4; i++)
			tk[i][j] = k[i][j];
	t = 0;
	/* copy values into round key array */
	for(j = 0; (j < 4) && (t < (10+1)*4); j++, t++)
		for(i = 0; i < 4; i++) W[t / 4][i][t % 4] = tk[i][j];
		
	while (t < (10+1)*4) { /* while not enough round key material calculated */
		/* calculate new values */
		for(i = 0; i < 4; i++)
			tk[i][0] ^= S[tk[(i+1)%4][4-1]];
		tk[0][0] ^= rcon[rconpointer++];

	
			for(j = 1; j < 4; j++)
				for(i = 0; i < 4; i++) tk[i][j] ^= tk[i][j-1];
	
	/* copy values into round key array */
	for(j = 0; (j < 4) && (t < (10+1)*4); j++, t++)
		for(i = 0; i < 4; i++) W[t / 4][i][t % 4] = tk[i][j];
	}		

	return 0;
}