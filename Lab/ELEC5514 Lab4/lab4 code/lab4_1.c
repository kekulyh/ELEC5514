case '8':
					printf("\r\nPlease input two numbers from 0 to 9 with blank among them:");
					char char_1;
					char char_2;
					char blank_1;
					
					char_1 = ConsoleGet();
					ConsolePut(char_1);
					blank_1 = ConsoleGet();
					ConsolePut(blank_1);
					char_2 = ConsoleGet();
					ConsolePut(char_2);

					int sumDec = (char_1 - '0') + (char_2 - '0');//int of decimal sum
					int sumHex = sumDec;//int of hex sum
					
					printf("\r\nSum of decimal is: ");
					char sumDecStr[2] = {0};//decimal sum string
					int count_1 = 0;
					for(count_1 = 2; count_1>0; count_1--){
						sumDecStr[count_1-1] = sumDec%10;
						sumDec = sumDec/10;
					}

					int count_2 = 0;
					for(count_2 = 0; count_2<2; count_2++){
							ConsolePut(sumDecStr[count_2]+'0');
					}
					
					printf("\r\nSum of hex is: 0x");
					char sumHexStr[2];//hex sum string
					if(sumHex<10){
						sumHexStr[0] = sumHex+'0';
					}else{
						if(sumHex<16){
							sumHexStr[0] = sumHex-10+'A';
						}else{
							sumHexStr[0] = 1+'0';
							sumHexStr[1] = sumHex%16 + '0';
						}
						
					}
					
					int count_3 = 0;
					for(count_3 = 0; count_3<2; count_3++){
						ConsolePut(sumHexStr[count_3]);	
					}

					break;