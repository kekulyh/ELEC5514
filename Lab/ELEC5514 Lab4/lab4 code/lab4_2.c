case '0':
					printf("\r\nPlease input a number from 256 to 999:");
					char char_1_new;
					char char_2_new;
					char char_3_new;
					
					char_1_new = ConsoleGet();
					ConsolePut(char_1_new);
					char_2_new = ConsoleGet();
					ConsolePut(char_2_new);
					char_3_new = ConsoleGet();
					ConsolePut(char_3_new);

					int decimal = (char_1_new-'0')*100 + (char_2_new-'0')*10 + (char_3_new-'0')*1;//input decimal number			
					char hexStr[8]={0};//hex number string
					
					int count_1_new = 0;
					for (count_1_new = 8; count_1_new>0; count_1_new--){
						hexStr[count_1_new-1] = decimal%16;
						if (decimal%16<10){
							hexStr[count_1_new-1] = decimal%16 +'0';
						}else{
							hexStr[count_1_new-1] = decimal%16 -10 +'A';
						}
						decimal = decimal/16;
					}

					printf("\r\nHex value of input is: 0x");
					int count_2_new = 0;
					for(count_2_new = 0; count_2_new <8; count_2_new++){
						ConsolePut(hexStr[count_2_new]);
					}
					break;