#include "aes.c"

int main(void){

    unsigned char key[Nk*4] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    unsigned char exkey[Nb * (Nr + 1) * 4];
    unsigned char State[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 7, 7, 7, 7, 7, 7, 7 };
 
    char hexs[16 * 2 + 1];
    KeyExpansion(key, exkey);
 
    Encrypt(State, exkey);
    toHex(State, 16, hexs);
    printf("%s\n", hexs);
 
    Decrypt(State, exkey);
    toHex(State, 16, hexs);
    printf("%s\n", hexs);
/*
输出:
33D19245695D96C60CCB73DB7E0FB1E9
01020304050607080907070707070707
*/

}