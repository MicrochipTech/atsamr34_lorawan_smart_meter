#include <asf.h>
#include <stdio.h>
#include <ctype.h>
#include "utils.h"

void charArrayToHexArray(const char* in, char* out)
{
	for(uint8_t i = 0; i < strlen(in); i++)
	{
		sprintf(out + i * 2, "%02X", in[i]) ;
	}	
}

uint8_t charToInt(uint8_t in)
{
	uint8_t out = -1 ;
	if (in >= 48 && in <= 57)		out = in - 48 ;	// decimal value between 1 and 9
	else if (in >= 65 && in <= 70)	out = in - 55 ;	// hex value between A and F
	else if (in >= 97 && in <= 102) out = in - 87 ;	// hex value between a and f
	return out ;
}

/*
 * \brief Converts the input string consisting of hexadecimal digits into an integer value
 */ 
int xtoi(char *c)
{
  size_t szlen = strlen(c);
  int idx, ptr, factor,result =0;

  if(szlen > 0){
    if(szlen > 8) return 0;
    result = 0;
    factor = 1;

    for(idx = szlen-1; idx >= 0; --idx){
    if(isxdigit( *(c+idx))){
	if( *(c + idx) >= 97){
	  ptr = ( *(c + idx) - 97) + 10;
	}else if( *(c + idx) >= 65){
	  ptr = ( *(c + idx) - 65) + 10;
	}else{
	  ptr = *(c + idx) - 48;
	}
	result += (ptr * factor);
	factor *= 16;
    }else{
		return 4;
    }
    }
  }

  return result;
}

uint8_t Parser_HexAsciiToInt(uint16_t hexAsciiLen, char* pInHexAscii, uint8_t* pOutInt)
{
	uint16_t rxHexAsciiLen = strlen(pInHexAscii);
	uint16_t iCtr = 0;
	uint16_t jCtr = rxHexAsciiLen >> 1;
	uint8_t retValue = 0;
	char tempBuff[3];
	if(rxHexAsciiLen % 2 == 0)
	{
		jCtr --;
	}
	if(hexAsciiLen == rxHexAsciiLen)
	{
		while(rxHexAsciiLen > 0)
		{
			if(rxHexAsciiLen >= 2U)
			{
				tempBuff[iCtr] = *(((char*)pInHexAscii) + (rxHexAsciiLen - 2));
				iCtr ++;
				tempBuff[iCtr] = *(((char*)pInHexAscii) + (rxHexAsciiLen - 1));

				rxHexAsciiLen -= 2U;
			}
			else
			{
				tempBuff[iCtr] = '0';
				iCtr ++;
				tempBuff[iCtr] = *(((char*)pInHexAscii) + (rxHexAsciiLen - 1));

				rxHexAsciiLen --;
			}

			iCtr ++;
			tempBuff[iCtr] = '\0';
			*(pOutInt + jCtr) = xtoi(tempBuff);
			iCtr = 0;
			jCtr --;
		}

		retValue = 1;
	}
	return retValue;
}

bool isHexFormat(uint8_t val)
{
	if (((val >= '0') && (val <= '9')) || ((val >= 'a') && (val <= 'f')) || ((val >= 'A') && (val <= 'F')))
	{
		return true ;
	}
	return false ;
}

bool isHexLowerCase(uint8_t val)
{
	if ((val >= 'a') && (val <= 'f'))
	{
		return true ;
	}
	return false ;
}

bool isHexUpperCase(uint8_t val)
{
	if ((val >= 'A') && (val <= 'F'))
	{
		return true ;
	}
	return false ;
}

uint8_t convertHexToUpperCase(uint8_t in)
{
	uint8_t out = in ;
	if (isHexLowerCase(in))
	{
		out = in - 0x20 ;
	}
	return out ;
}

uint8_t convertHexToLowerCase(uint8_t in)
{
	uint8_t out = in ;
	if (isHexUpperCase(in))
	{
		out = in + 0x20 ;
	}
	return out ;
}

/*** print_array ****************************************************************
 \brief      Function to Print array of characters
 \param[in]  *array  - Pointer of the array to be printed
 \param[in]   length - Length of the array
********************************************************************************/
void print_array (uint8_t *array, uint8_t length)
{
	for (uint8_t i = 0; i < length; i++)
	{
		printf("%02x", *array) ;
		array++ ;
	}
	printf("\r\n") ;
}