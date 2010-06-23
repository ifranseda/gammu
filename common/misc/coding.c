
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <wctype.h>

#ifdef WIN32
#  include "windows.h"
#endif

#include "misc.h"
#include "coding.h"

/* Convert Unicode char saved in src to dest */
unsigned int EncodeWithUnicodeAlphabet(const unsigned char *src, wchar_t *dest)
{
	char retval;

        switch (retval = mbtowc(dest, src, MB_CUR_MAX)) {
                case -1 :
		case  0 : return 1;
                default : return retval;
        }
}

/* Convert Unicode char saved in src to dest */
unsigned int DecodeWithUnicodeAlphabet(wchar_t src, unsigned char *dest)
{
        int retval;
        
        switch (retval = wctomb(dest, src)) {
                case -1:
                        *dest = '?';
                        return 1;
                default:
                        return retval;
        }
}

void DecodeUnicode (const unsigned char *src, unsigned char *dest)
{
 	int 		i=0,o=0;
 	wchar_t 	wc;

 	while (src[(2*i)+1]!=0x00 || src[2*i]!=0x00) {
 		wc = src[(2*i)+1] | (src[2*i] << 8);
		o += DecodeWithUnicodeAlphabet(wc, dest + o);
 		i++;
 	}
	dest[o]=0;
}

/* Decode Unicode string and return as function result */
unsigned char *DecodeUnicodeString (const unsigned char *src)
{
 	static char dest[500];

	DecodeUnicode(src,dest);
	return dest;
}

/* Encode string to Unicode. Len is number of input chars */
void EncodeUnicode (unsigned char *dest, const unsigned char *src, int len)
{
	int 		i_len = 0, o_len;
 	wchar_t 	wc;
 
	for (o_len = 0; i_len < len; o_len++) {
		i_len += EncodeWithUnicodeAlphabet(&src[i_len], &wc);
		dest[o_len*2]		= (wc >> 8) & 0xff;
		dest[(o_len*2)+1]	= wc & 0xff;
 	}
	dest[o_len*2]		= 0;
	dest[(o_len*2)+1]	= 0;
}

unsigned char EncodeWithBCDAlphabet(int value)
{
	div_t division;

	division=div(value,10);
	return ( ( (value-division.quot*10) & 0x0f) << 4) | (division.quot & 0xf);
}

int DecodeWithBCDAlphabet(unsigned char value)
{
	return 10*(value & 0x0f)+(value >> 4);
}

void DecodeBCD (unsigned char *dest, const unsigned char *src, int len)
{
	int i,current=0,digit;

	for (i = 0; i < len; i++) {
	        digit=src[i] & 0x0f;
                if (digit<10) dest[current++]=digit + '0';
	        digit=src[i] >> 4;
                if (digit<10) dest[current++]=digit + '0';
	}
	dest[current++]=0;
}

void EncodeBCD (unsigned char *dest, const unsigned char *src, int len, bool fill)
{
	int i,current=0;

	for (i = 0; i < len; i++) {
        	if (i & 0x01) {
			dest[current]=dest[current] | ((src[i]-'0') << 4);
			current++;
		} else {
			dest[current]=src[i]-'0';
		}
	}

        /* When fill is set: we fill in the most significant bits of the
           last byte with 0x0f (1111 binary) if the number is represented
           with odd number of digits. */
	if (fill && (len & 0x01)) {
		dest[current]=dest[current] | 0xf0;
        }
}

/* When char can be converted, convert it from Unicode to UTF8 */
bool EncodeWithUTF8Alphabet(unsigned char mychar1, unsigned char mychar2, unsigned char *ret1, unsigned char *ret2)
{
	unsigned char	mychar3,mychar4;
	int		j=0;
      
	if (mychar1>0x00 || mychar2>128) {
		mychar3=0x00;
		mychar4=128;
		while (true) {
			if (mychar3==mychar1) {
				if (mychar4+64>=mychar2) {
					*ret1=j+0xc2;
					*ret2=0x80+(mychar2-mychar4);
					return true;
				}
			}
			if (mychar4==192) {
				mychar3++;
				mychar4=0;
			} else {
				mychar4=mychar4+64;
			}
			j++;
		}
	}
	return false;
}

/* Decode UTF8 char to Unicode char */
wchar_t DecodeWithUTF8Alphabet(unsigned char mychar3, unsigned char mychar4)
{
	unsigned char	mychar1, mychar2;
	int		j;
	
	mychar1=0x00;
	mychar2=128;
	for(j=0;j<mychar3-0xc2;j++) {
		if (mychar2==192) {
			mychar1++;
			mychar2 = 0;
		} else {
			mychar2 = mychar2+64;
		}
	}
	mychar2 = mychar2+(mychar4-0x80);
	return mychar2 | (mychar1 << 8);
}

/* Make UTF8 string from Unicode input string */
void EncodeUTF8(unsigned char *dest, const unsigned char *src)
{
	int		i,j=0;
	unsigned char	mychar1, mychar2;
	
	for (i = 0; i < ((int)strlen(DecodeUnicodeString(src))); i++) {
	    if (EncodeWithUTF8Alphabet(src[i*2],src[i*2+1],&mychar1,&mychar2)) {
		sprintf(dest+j, "=%02X=%02X",mychar1,mychar2);
		j=j+6;
	    } else {
		j += DecodeWithUnicodeAlphabet(((wchar_t)(src[i*2]*256+src[i*2+1])), dest + j);
	    }
	}
	dest[j++]=0;
}

int DecodeWithHexBinAlphabet (unsigned char mychar)
{
	if (mychar>='A' && mychar<='F') return mychar-'A'+10;
	if (mychar>='a' && mychar<='f') return mychar-'a'+10;
	if (mychar>='0' && mychar<='9') return mychar-'0';
	return -1;
}

unsigned char EncodeWithHexBinAlphabet (int digit)
{
	if (digit >= 0 && digit <= 9) return '0'+(digit);
	if (digit >=10 && digit <=15) return 'A'+(digit-10);
	return 0;
}

void DecodeHexBin (unsigned char *dest, const unsigned char *src, int len)
{
	int i,current=0;

	for (i = 0; i < len/2 ; i++) {
		dest[current++] = DecodeWithHexBinAlphabet(src[i*2])*16+
				  DecodeWithHexBinAlphabet(src[i*2+1]);
	}
	dest[current++] = 0;
}

void EncodeHexBin (unsigned char *dest, const unsigned char *src, int len)
{
	int i,current=0;

	for (i = 0; i < len; i++) {
		dest[current++] = EncodeWithHexBinAlphabet(src[i] >> 0x04);
		dest[current++] = EncodeWithHexBinAlphabet(src[i] & 0x0f);
	}
	dest[current++] = 0;
}

/* ETSI GSM 03.38, version 6.0.1, section 6.2.1; Default alphabet */
static unsigned char GSM_DefaultAlphabetUnicode[128+1][2] =
{
	{0x00,0x40},{0x00,0xa3},{0x00,0x24},{0x00,0xA5},
	{0x00,0xE8},{0x00,0xE9},{0x00,0xF9},{0x00,0xEC},/*0x08*/
	{0x00,0xF2},{0x00,0xC7},{0x00,'\n'},{0x00,0xD8},
	{0x00,0xF8},{0x00,'\r'},{0x00,0xC5},{0x00,0xE5},
	{0x03,0x94},{0x00,0x5f},{0x03,0xA6},{0x03,0x93},
	{0x03,0x9B},{0x03,0xA9},{0x03,0xA0},{0x03,0xA8},
	{0x03,0xA3},{0x03,0x98},{0x03,0x9E},{0x00,0xb9},
	{0x00,0xC6},{0x00,0xE6},{0x00,0xDF},{0x00,0xC9},/*0x20*/
	{0x00,' ' },{0x00,'!' },{0x00,'\"'},{0x00,'#' },
	{0x00,0xA4},{0x00,'%' },{0x00,'&' },{0x00,'\''},
	{0x00,'(' },{0x00,')' },{0x00,'*' },{0x00,'+' },
	{0x00,',' },{0x00,'-' },{0x00,'.' },{0x00,'/' },/*0x30*/
	{0x00,'0' },{0x00,'1' },{0x00,'2' },{0x00,'3' },
	{0x00,'4' },{0x00,'5' },{0x00,'6' },{0x00,'7' },
	{0x00,'8' },{0x00,'9' },{0x00,':' },{0x00,';' },
	{0x00,'<' },{0x00,'=' },{0x00,'>' },{0x00,'?' },/*0x40*/
	{0x00,0xA1},{0x00,'A' },{0x00,'B' },{0x00,'C' },
	{0x00,'D' },{0x00,'E' },{0x00,'F' },{0x00,'G' },
	{0x00,'H' },{0x00,'I' },{0x00,'J' },{0x00,'K' },
	{0x00,'L' },{0x00,'M' },{0x00,'N' },{0x00,'O' },
	{0x00,'P' },{0x00,'Q' },{0x00,'R' },{0x00,'S' },
	{0x00,'T' },{0x00,'U' },{0x00,'V' },{0x00,'W' },
	{0x00,'X' },{0x00,'Y' },{0x00,'Z' },{0x00,0xC4},
	{0x00,0xD6},{0x00,0xD1},{0x00,0xDC},{0x00,0xA7},
	{0x00,0xBF},{0x00,'a' },{0x00,'b' },{0x00,'c' },
	{0x00,'d' },{0x00,'e' },{0x00,'f' },{0x00,'g' },
	{0x00,'h' },{0x00,'i' },{0x00,'j' },{0x00,'k' },
	{0x00,'l' },{0x00,'m' },{0x00,'n' },{0x00,'o' },
	{0x00,'p' },{0x00,'q' },{0x00,'r' },{0x00,'s' },
	{0x00,'t' },{0x00,'u' },{0x00,'v' },{0x00,'w' },
	{0x00,'x' },{0x00,'y' },{0x00,'z' },{0x00,0xE4},
	{0x00,0xF6},{0x00,0xF1},{0x00,0xFC},{0x00,0xE0},
	{0x00,0x00}
};

/* ETSI GSM 3.38
 * Some sequences of 2 default alphabet chars (for example,
 * 0x1b, 0x65) are visible as one single additional char (for example,
 * 0x1b, 0x65 gives Euro char saved in Unicode as 0x20, 0xAC)
 * This table contains:
 * 1. two first chars means sequence of chars from GSM default alphabet
 * 2. two second is target (encoded) char saved in Unicode
 */
static unsigned char GSM_DefaultAlphabetCharsExtension[][4] =
{
	{0x1b,0x14,0x00,0x5e},	/* ^	*/
	{0x1b,0x28,0x00,0x7b},	/* {	*/
	{0x1b,0x29,0x00,0x7d},	/* }	*/
	{0x1b,0x2f,0x00,0x5c},	/* \	*/
	{0x1b,0x3c,0x00,0x5b},	/* [	*/
	{0x1b,0x3d,0x00,0x7E},	/* ~	*/
	{0x1b,0x3e,0x00,0x5d},	/* ]	*/
	{0x1b,0x40,0x00,0x7C},	/* |	*/
	{0x1b,0x65,0x20,0xAC},	/* Euro */
	{0x00,0x00,0x00,0x00}
};

void DecodeDefault (unsigned char *dest, const unsigned char *src, int len, bool UseExtensions, unsigned char *ExtraAlphabet)
{
	int 	i,current=0,j;
	bool	FoundSpecial = false;

#ifdef DEBUG
	if (di.dl == DL_TEXTALL || di.dl == DL_TEXTALLDATE) DumpMessage(di.df, src, len);
#endif

	for (i = 0; i < len; i++) {
		FoundSpecial = false;
		if ((i < (len-1)) && UseExtensions) {
			j=0;
			while (GSM_DefaultAlphabetCharsExtension[j][0]!=0x00) {
				if (GSM_DefaultAlphabetCharsExtension[j][0]==src[i] && 
				    GSM_DefaultAlphabetCharsExtension[j][1]==src[i+1]) {
					FoundSpecial = true;
					dest[current++] = GSM_DefaultAlphabetCharsExtension[j][2];
					dest[current++] = GSM_DefaultAlphabetCharsExtension[j][3];
					i++;
					break;
				}
				j++;
			}
		}
       		if (ExtraAlphabet!=NULL && !FoundSpecial) {
			j = 0;
			while (ExtraAlphabet[j] != 0x00 || ExtraAlphabet[j+1] != 0x00 || ExtraAlphabet[j+2] != 0x00) {
				if (ExtraAlphabet[j] == src[i]) {
					dest[current++] = ExtraAlphabet[j+1];
					dest[current++] = ExtraAlphabet[j+2];
					FoundSpecial 	= true;
                            		break;
                        	}
                        	j=j+3;
                    	}
                }
		if (!FoundSpecial) {
			dest[current++] = GSM_DefaultAlphabetUnicode[src[i]][0];
			dest[current++] = GSM_DefaultAlphabetUnicode[src[i]][1];
		}
	}
	dest[current++]=0;
	dest[current++]=0;
#ifdef DEBUG
	if (di.dl == DL_TEXTALL || di.dl == DL_TEXTALLDATE) DumpMessage(di.df, dest, strlen(DecodeUnicodeString(dest))*2);
#endif
}

/* There are many national chars with "adds". In phone they're normally
 * changed to "plain" Latin chars. We have such functionality too.
 * This table is automatically created from convert.txt file (see
 * /docs/developers) using --makeconverttable. It contains such chars
 * to replace in order:
 * 1. original char (Unicode) 2. destination char (Unicode)
 */
static unsigned char ConvertTable[] =
"\x00\xc0\x00\x41\x00\xe0\x00\x61\x00\xc1\x00\x41\x00\xe1\x00\x61\x00\xc2\x00\x41\x00\xe2\x00\x61\x00\xc3\x00\x41\x00\xe3\x00\x61\x1e\xa0\x00\x41\x1e\xa1\x00\x61\x1e\xa2\x00\x41\x1e\xa3\x00\x61\x1e\xa4\x00\x41\x1e\xa5\x00\x61\x1e\xa6\x00\x41\x1e\xa7\x00\x61\x1e\xa8\x00\x41\x1e\xa9\x00\x61\x1e\xaa\x00\x41\x1e\xab\x00\x61\x1e\xac\x00\x41\x1e\xad\x00\x61\x1e\xae\x00\x41\x1e\xaf\x00\x61\x1e\xb0\x00\x41\x1e\xb1\x00\x61\x1e\xb2\x00\x41\x1e\xb3\x00\x61\x1e\xb4\x00\x41\x1e\xb5\x00\x61\x1e\xb6\x00\x41\x1e\xb7\x00\x61\x01\xcd\x00\x41\x01\xce\x00\x61\x01\x00\x00\x41\x01\x01\x00\x61\x01\x02\x00\x41\x01\x03\x00\x61\x01\x04\x00\x41\x01\x05\x00\x61\x01\xfb\x00\x61\x01\x06\x00\x43\x01\x07\x00\x63\x01\x08\x00\x43\x01\x09\x00\x63\x01\x0a\x00\x43\x01\x0b\x00\x63\x01\x0c\x00\x43\x01\x0d\x00\x63\x00\xe7"\
"\x00\x63\x01\x0e\x00\x44\x01\x0f\x00\x64\x01\x10\x00\x44\x01\x11\x00\x64\x00\xc8\x00\x45\x00\xca\x00\x45\x00\xea\x00\x65\x00\xcb\x00\x45\x00\xeb\x00\x65\x1e\xb8\x00\x45\x1e\xb9\x00\x65\x1e\xba\x00\x45\x1e\xbb\x00\x65\x1e\xbc\x00\x45\x1e\xbd\x00\x65\x1e\xbe\x00\x45\x1e\xbf\x00\x65\x1e\xc0\x00\x45\x1e\xc1\x00\x65\x1e\xc2\x00\x45\x1e\xc3\x00\x65\x1e\xc4\x00\x45\x1e\xc5\x00\x65\x1e\xc6\x00\x45\x1e\xc7\x00\x65\x01\x12\x00\x45\x01\x13\x00\x65\x01\x14\x00\x45\x01\x15\x00\x65\x01\x16\x00\x45\x01\x17\x00\x65\x01\x18\x00\x45\x01\x19\x00\x65\x01\x1a\x00\x45\x01\x1b\x00\x65\x01\x1c\x00\x47\x01\x1d\x00\x67\x01\x1e\x00\x47\x01\x1f\x00\x67\x01\x20\x00\x47\x01\x21\x00\x67\x01\x22\x00\x47\x01\x23\x00\x67\x01\x24\x00\x48\x01\x25\x00\x68\x01\x26\x00\x48\x01\x27\x00\x68\x00\xcc\x00\x49\x00\xcd\x00\x49\x00\xed"\
"\x00\x69\x00\xce\x00\x49\x00\xee\x00\x69\x00\xcf\x00\x49\x00\xef\x00\x69\x01\x28\x00\x49\x01\x29\x00\x69\x01\x2a\x00\x49\x01\x2b\x00\x69\x01\x2c\x00\x49\x01\x2d\x00\x69\x01\x2e\x00\x49\x01\x2f\x00\x69\x01\x30\x00\x49\x01\x31\x00\x69\x01\xcf\x00\x49\x01\xd0\x00\x69\x1e\xc8\x00\x49\x1e\xc9\x00\x69\x1e\xca\x00\x49\x1e\xcb\x00\x69\x01\x34\x00\x4a\x01\x35\x00\x6a\x01\x36\x00\x4b\x01\x37\x00\x6b\x01\x39\x00\x4c\x01\x3a\x00\x6c\x01\x3b\x00\x4c\x01\x3c\x00\x6c\x01\x3d\x00\x4c\x01\x3e\x00\x6c\x01\x3f\x00\x4c\x01\x40\x00\x6c\x01\x41\x00\x4c\x01\x42\x00\x6c\x01\x43\x00\x4e\x01\x44\x00\x6e\x01\x45\x00\x4e\x01\x46\x00\x6e\x01\x47\x00\x4e\x01\x48\x00\x6e\x01\x49\x00\x6e\x00\xd2\x00\x4f\x00\xd3\x00\x4f\x00\xf3\x00\x6f\x00\xd4\x00\x4f\x00\xf4\x00\x6f\x00\xd5\x00\x4f\x00\xf5\x00\x6f\x01\x4c\x00\x4f\x01\x4d"\
"\x00\x6f\x01\x4e\x00\x4f\x01\x4f\x00\x6f\x01\x50\x00\x4f\x01\x51\x00\x6f\x01\xa0\x00\x4f\x01\xa1\x00\x6f\x01\xd1\x00\x4f\x01\xd2\x00\x6f\x1e\xcc\x00\x4f\x1e\xcd\x00\x6f\x1e\xce\x00\x4f\x1e\xcf\x00\x6f\x1e\xd0\x00\x4f\x1e\xd1\x00\x6f\x1e\xd2\x00\x4f\x1e\xd3\x00\x6f\x1e\xd4\x00\x4f\x1e\xd5\x00\x6f\x1e\xd6\x00\x4f\x1e\xd7\x00\x6f\x1e\xd8\x00\x4f\x1e\xd9\x00\x6f\x1e\xda\x00\x4f\x1e\xdb\x00\x6f\x1e\xdc\x00\x4f\x1e\xdd\x00\x6f\x1e\xde\x00\x4f\x1e\xdf\x00\x6f\x1e\xe0\x00\x4f\x1e\xe1\x00\x6f\x1e\xe2\x00\x4f\x1e\xe3\x00\x6f\x01\x54\x00\x52\x01\x55\x00\x72\x01\x56\x00\x52\x01\x57\x00\x72\x01\x58\x00\x52\x01\x59\x00\x72\x01\x5a\x00\x53\x01\x5b\x00\x73\x01\x5c\x00\x53\x01\x5d\x00\x73\x01\x5e\x00\x53\x01\x5f\x00\x73\x01\x60\x00\x53\x01\x61\x00\x73\x01\x62\x00\x54\x01\x63\x00\x74\x01\x64\x00\x54\x01\x65"\
"\x00\x74\x01\x66\x00\x54\x01\x67\x00\x74\x00\xd9\x00\x55\x00\xda\x00\x55\x00\xfa\x00\x75\x00\xdb\x00\x55\x00\xfb\x00\x75\x01\x68\x00\x55\x01\x69\x00\x75\x01\x6a\x00\x55\x01\x6b\x00\x75\x01\x6c\x00\x55\x01\x6d\x00\x75\x01\x6e\x00\x55\x01\x6f\x00\x75\x01\x70\x00\x55\x01\x71\x00\x75\x01\x72\x00\x55\x01\x73\x00\x75\x01\xaf\x00\x55\x01\xb0\x00\x75\x01\xd3\x00\x55\x01\xd4\x00\x75\x01\xd5\x00\x55\x01\xd6\x00\x75\x01\xd7\x00\x55\x01\xd8\x00\x75\x01\xd9\x00\x55\x01\xda\x00\x75\x01\xdb\x00\x55\x01\xdc\x00\x75\x1e\xe4\x00\x55\x1e\xe5\x00\x75\x1e\xe6\x00\x55\x1e\xe7\x00\x75\x1e\xe8\x00\x55\x1e\xe9\x00\x75\x1e\xea\x00\x55\x1e\xeb\x00\x75\x1e\xec\x00\x55\x1e\xed\x00\x75\x1e\xee\x00\x55\x1e\xef\x00\x75\x1e\xf0\x00\x55\x1e\xf1\x00\x75\x01\x74\x00\x57\x01\x75\x00\x77\x1e\x80\x00\x57\x1e\x81\x00\x77\x1e\x82"\
"\x00\x57\x1e\x83\x00\x77\x1e\x84\x00\x57\x1e\x85\x00\x77\x00\xdd\x00\x59\x00\xfd\x00\x79\x00\xff\x00\x79\x01\x76\x00\x59\x01\x77\x00\x79\x01\x78\x00\x59\x1e\xf2\x00\x59\x1e\xf3\x00\x75\x1e\xf4\x00\x59\x1e\xf5\x00\x79\x1e\xf6\x00\x59\x1e\xf7\x00\x79\x1e\xf8\x00\x59\x1e\xf9\x00\x79\x01\x79\x00\x5a\x01\x7a\x00\x7a\x01\x7b\x00\x5a\x01\x7c\x00\x7a\x01\x7d\x00\x5a\x01\x7e\x00\x7a\x01\xfc\x00\xc6\x01\xfd\x00\xe6\x01\xfe\x00\xd8\x01\xff\x00\xf8\x00\x00";

void EncodeDefault(unsigned char *dest, const unsigned char *src, int *len, bool UseExtensions, unsigned char *ExtraAlphabet)
{
	int 	i,current=0,j,z;
	char 	ret;
	bool	FoundSpecial,FoundNormal;

#ifdef DEBUG
	if (di.dl == DL_TEXTALL || di.dl == DL_TEXTALLDATE) DumpMessage(di.df, src, (*len)*2);
#endif

	for (i = 0; i < *len; i++) {
		FoundSpecial = false;
		j = 0;
		while (GSM_DefaultAlphabetCharsExtension[j][0]!=0x00 && UseExtensions) {
			if (src[i*2] 	== GSM_DefaultAlphabetCharsExtension[j][2] &&
			    src[i*2+1] 	== GSM_DefaultAlphabetCharsExtension[j][3]) {
				dest[current++] = GSM_DefaultAlphabetCharsExtension[j][0];
				dest[current++] = GSM_DefaultAlphabetCharsExtension[j][1];
				FoundSpecial 	= true;
				break;
			}
			j++;
		}
       		if (ExtraAlphabet!=NULL && !FoundSpecial) {
 			j = 0;
        		while (ExtraAlphabet[j] != 0x00 || ExtraAlphabet[j+1] != 0x00 || ExtraAlphabet[j+2] != 0x00) {
                		if (ExtraAlphabet[j+1] == src[i*2] &&
				    ExtraAlphabet[j+2] == src[i*2 + 1])
				{
                    			dest[current++] = ExtraAlphabet[j];
                    			FoundSpecial 	= true;
                    			break;
                		}
                		j=j+3;
            		}
        	}
		if (!FoundSpecial) {
			ret 		= '?';
			FoundNormal 	= false;
			j 		= 0;
			while (GSM_DefaultAlphabetUnicode[j][1]!=0x00)
			{
				if (src[i*2]	== GSM_DefaultAlphabetUnicode[j][0] &&
				    src[i*2+1]	== GSM_DefaultAlphabetUnicode[j][1])
				{
					ret 		= j;
					FoundNormal 	= true;
					break;
				}
				j++;
			}
			if (!FoundNormal) {
				j = 0;
				FoundNormal = false;
				while (ConvertTable[j*4]   != 0x00 ||
				       ConvertTable[j*4+1] != 0x00) {
					if (src[i*2]   == ConvertTable[j*4] &&
					    src[i*2+1] == ConvertTable[j*4+1]) {
						z = 0;
						while (GSM_DefaultAlphabetUnicode[z][1]!=0x00)
						{
							if (ConvertTable[j*4+2]	== GSM_DefaultAlphabetUnicode[z][0] &&
							    ConvertTable[j*4+3]	== GSM_DefaultAlphabetUnicode[z][1])
							{
								ret 		= z;
								FoundNormal 	= true;
								break;
							}
							z++;
						}
						if (FoundNormal) break;
					}
					j++;
				}
			}
			dest[current++]=ret;
		}
	}
	dest[current]=0;
#ifdef DEBUG
	if (di.dl == DL_TEXTALL || di.dl == DL_TEXTALLDATE) DumpMessage(di.df, dest, current);
#endif

	*len = current;
}

/* You don't have to use ConvertTable here - 1 char is replaced there by 1 char */
void FindDefaultAlphabetLen(const unsigned char *src, int *srclen, int *smslen, int maxlen)
{
	int 	current=0,j,i;
	bool	FoundSpecial;

	i = 0;
	while (src[i*2] != 0x00 || src[i*2+1] != 0x00)
	{
		FoundSpecial = false;
		j = 0;
		while (GSM_DefaultAlphabetCharsExtension[j][0]!=0x00) {
			if (src[i*2] 	== GSM_DefaultAlphabetCharsExtension[j][2] &&
			    src[i*2+1] 	== GSM_DefaultAlphabetCharsExtension[j][3]) {
				FoundSpecial = true;
				if (current+2 > maxlen) {
					*srclen = i;
					*smslen = current;
					return;
				}
				current+=2;
				break;
			}
			j++;
		}
		if (!FoundSpecial) {
			if (current+1 > maxlen) {
				*srclen = i;
				*smslen = current;
				return;
			}
			current++;
		}
		i++;
	}
	*srclen = i;
	*smslen = current;
}

#define ByteMask ((1 << Bits) - 1)

int GSM_UnpackEightBitsToSeven(int offset, int in_length, int out_length,
                           unsigned char *input, unsigned char *output)
{
        unsigned char *OUTPUT 	= output; /* Current pointer to the output buffer */
        unsigned char *INPUT  	= input;  /* Current pointer to the input buffer */
        unsigned char Rest 	= 0x00;
        int	      Bits;

        Bits = offset ? offset : 7;

        while ((INPUT - input) < in_length) {

                *OUTPUT = ((*INPUT & ByteMask) << (7 - Bits)) | Rest;
                Rest = *INPUT >> Bits;

                /* If we don't start from 0th bit, we shouldn't go to the
                   next char. Under *OUTPUT we have now 0 and under Rest -
                   _first_ part of the char. */
                if ((INPUT != input) || (Bits == 7)) OUTPUT++;
                INPUT++;

                if ((OUTPUT - output) >= out_length) break;

                /* After reading 7 octets we have read 7 full characters but
                   we have 7 bits as well. This is the next character */
                if (Bits == 1) {
                        *OUTPUT = Rest;
                        OUTPUT++;
                        Bits = 7;
                        Rest = 0x00;
                } else {
                        Bits--;
                }
        }

        return OUTPUT - output;
}

int GSM_PackSevenBitsToEight(int offset, unsigned char *input, unsigned char *output, int length)
{
        unsigned char 	*OUTPUT = output; /* Current pointer to the output buffer */
        unsigned char 	*INPUT  = input;  /* Current pointer to the input buffer */
        int		Bits;             /* Number of bits directly copied to
                                           * the output buffer */
        Bits = (7 + offset) % 8;

        /* If we don't begin with 0th bit, we will write only a part of the
           first octet */
        if (offset) {
                *OUTPUT = 0x00;
                OUTPUT++;
        }

        while ((INPUT - input) < length) {
                unsigned char Byte = *INPUT;

                *OUTPUT = Byte >> (7 - Bits);
                /* If we don't write at 0th bit of the octet, we should write
                   a second part of the previous octet */
                if (Bits != 7)
                        *(OUTPUT-1) |= (Byte & ((1 << (7-Bits)) - 1)) << (Bits+1);

                Bits--;

                if (Bits == -1) Bits = 7; else OUTPUT++;

                INPUT++;
        }
        return (OUTPUT - output);
}

void GSM_UnpackSemiOctetNumber(unsigned char *retval, unsigned char *Number, bool semioctet)
{
	unsigned char	Buffer[20]	= "";
	int		length		= Number[0];

	if (semioctet) {
		/* Convert number of semioctets to number of chars */
		if (length % 2) length++;
		length=length / 2 + 1;
	}

	/*without leading byte with format of number*/  
	length--;

	switch (Number[1]) {
	case GNT_ALPHANUMERIC:
		if (length > 6) length++;
		dprintf("Alphanumeric number, length %i\n",length);
		GSM_UnpackEightBitsToSeven(0, length, length, Number+2, Buffer);
		Buffer[length]=0;
		break;
	case GNT_INTERNATIONAL:
		dprintf("International number\n");
		Buffer[0]='+';
		DecodeBCD(Buffer+1,Number+2, length);
		break;
	default:
		dprintf("Default number %02x\n",Number[1]);
		DecodeBCD (Buffer, Number+2, length);
		break;
	}

	EncodeUnicode(retval,Buffer,strlen(Buffer));
}

/* This function implements packing of numbers (SMS Center number and
   destination number) for SMS sending function. */
/* See GSM 03.40 9.1.1:
   1 byte  - length of number given in semioctets or bytes (when given in bytes,
             includes one byte for byte with number format.
             Returned by function (set semioctet to true, if want result
             in semioctets).
   1 byte  - format of number. See GSM_NumberType; in gsm-common.h. Returned
             in unsigned char *Output.
   n bytes - 2n or 2n-1 semioctets with number. For some types of numbers
             in the most significant bits of the last byte with 0x0f
             (1111 binary) are filled if the number is represented
             with odd number of digits. Returned in unsigned char *Output. */
/* 1 semioctet = 4 bits = half of byte */
int GSM_PackSemiOctetNumber(unsigned char *Number, unsigned char *Output, bool semioctet)
{
	unsigned char	buffer[40];
	unsigned char	*OUTPUT=Output;		/* Pointer to the output */
	int		length=0,j;
	unsigned char	format=GNT_UNKNOWN;	/* format of number used by us */

	length=strlen(DecodeUnicodeString(Number));
	memcpy(buffer,DecodeUnicodeString(Number),length+1);

	/* Checking for format number */
	for (j=0;j<length;j++) {
		/* first byte is '+'. It can be international */
		if (j==0 && buffer[j]=='+') format=GNT_INTERNATIONAL;  
		else {
			/*char is not number. It must be alphanumeric*/
			if (!isdigit(buffer[j])) format=GNT_ALPHANUMERIC;
		}
	}

	/* The first byte in the Semi-octet representation of the address field is
	 * the Type-of-Address. This field is described in the official GSM
	 * specification 03.40 version 5.3.0, section 9.1.2.5, page 33.*/
	*OUTPUT++=format;

	/* The next field is the number. See GSM 03.40 section 9.1.2 */
	switch (format) {
		case GNT_ALPHANUMERIC:
			length=GSM_PackSevenBitsToEight(0, buffer, OUTPUT, strlen(buffer))*2;
			if (strlen(buffer)==7) length--;
			break;
		case GNT_INTERNATIONAL:
			length--;
			EncodeBCD (OUTPUT, buffer+1, length, true);
			break;
		default:
			EncodeBCD (OUTPUT, buffer, length, true);
			break;
	}
	if (semioctet) return length;
	/* Convert number of semioctets to number of chars */
	if (length % 2) length++;
	return length / 2 + 1;
}

void CopyUnicodeString(unsigned char *Dest, unsigned char *Source)
{
	int j = 0;

	while (Source[j]!=0x00 || Source[j+1]!=0x00) {
		Dest[j]		= Source[j];
		Dest[j+1]	= Source[j+1];
		j=j+2;
	}
	Dest[j]		= 0;
	Dest[j+1]	= 0;
}

/* Changes minor/major order in Unicode string */
void ReverseUnicodeString(unsigned char *String)
{
	int 		j = 0;
	unsigned char	byte1, byte2;

	while (String[j]!=0x00 || String[j+1]!=0x00) {
		byte1		= String[j];
		byte2		= String[j+1];
		String[j+1]	= byte1;
		String[j]	= byte2;
		j=j+2;
	}
	String[j]	= 0;
	String[j+1]	= 0;
}

/* All input is in Unicode. First char can show Unicode minor/major order.
   Output is Unicode string in Gammu minor/major order */
void ReadUnicodeFile(unsigned char *Dest, unsigned char *Source)
{
	int j = 0, current = 0;

	if (Source[0] == 0xFF && Source[1] == 0xFE) j = 2;
	if (Source[0] == 0xFE && Source[1] == 0xFF) j = 2;

	while (Source[j]!=0x00 || Source[j+1]!=0x00) {
		if (Source[0] == 0xFF) {
			Dest[current++] = Source[j+1];
			Dest[current++]	= Source[j];
		} else {
			Dest[current++] = Source[j];
			Dest[current++]	= Source[j+1];
		}
		j=j+2;
	}
	Dest[current++] = 0;
	Dest[current++]	= 0;
}

int OctetAlign(unsigned char *Dest, int CurrentBit)
{
	int i=0;
	while((CurrentBit+i)%8) {
		ClearBit(Dest, CurrentBit+i);
		i++;
	}
	return CurrentBit+i;
}

int OctetAlignNumber(int CurrentBit)
{
	int i=0;
	while((CurrentBit+i)%8) { i++; }
	return CurrentBit+i;
}

int BitPack(unsigned char *Dest, int CurrentBit, unsigned char *Source, int Bits)
{
	int i;
	for (i=0; i<Bits; i++)
		if (GetBit(Source, i))
			SetBit(Dest, CurrentBit+i);
		else
			ClearBit(Dest, CurrentBit+i);
	return CurrentBit+Bits;
}

int BitPackByte(unsigned char *Dest, int CurrentBit, unsigned char Command, int Bits)
{
	unsigned char Byte[] = {0x00};

	Byte[0] = Command;
	return BitPack(Dest, CurrentBit, Byte, Bits);
}

int BitUnPack(unsigned char *Dest, int CurrentBit, unsigned char *Source, int Bits)
{
	int i;

	for (i=0; i<Bits; i++)
		if (GetBit(Dest, CurrentBit+i)) {
			SetBit(Source, i);
		} else {
			ClearBit(Source, i);
		}
	return CurrentBit+Bits;
}

int BitUnPackInt(unsigned char *Src, int CurrentBit, int *integer, int Bits)
{
	int l=0,z=128,i;

	for (i=0; i<Bits; i++) {
		if (GetBit(Src, CurrentBit+i)) l=l+z;
		z=z/2;
	}
	*integer=l;  
	return CurrentBit+i;
}

int OctetUnAlign(int CurrentBit)
{
	int i=0;

	while((CurrentBit+i)%8) i++;
	return CurrentBit+i;
}

/* Unicode char 0x00 0x01 makes blinking in some Nokia phones.
 * We replace single ~ chars into it. When user give double ~, it's replaced
 * to single ~
 */
void EncodeUnicodeSpecialNOKIAChars(unsigned char *dest, const unsigned char *src, int len)
{
	int 	i,current = 0;
	bool 	special=false;

	for (i = 0; i < len; i++) {
		if (special) {
			if (src[i*2] == 0x00 && src[i*2+1] == '~') {
				dest[current++]	= 0x00;
				dest[current++]	= '~';
			} else {
				dest[current++]	= 0x00;
				dest[current++]	= 0x01;					
				dest[current++]	= src[i*2];
				dest[current++]	= src[i*2+1];
			}
			special = false;
		} else {
			if (src[i*2] == 0x00 && src[i*2+1] == '~') {
				special = true;
			} else {
				dest[current++]	= src[i*2];
				dest[current++]	= src[i*2+1];
			}
		}
	}
	if (special) {
		dest[current++]	= 0x00;
		dest[current++]	= 0x01;
	}
	dest[current++] = 0x00;
	dest[current++] = 0x00;
}

void DecodeUnicodeSpecialNOKIAChars(unsigned char *dest, const unsigned char *src, int len)
{
	int i=0,current=0;

	for (i=0;i<len;i++) {
		switch (src[2*i]) {
			case 0x00:
				switch (src[2*i+1]) {
					case 0x01:
						dest[current++] = 0x00;
						dest[current++] = '~';
						break;
					case '~':
						dest[current++] = 0x00;
						dest[current++] = '~';
						dest[current++] = 0x00;
						dest[current++] = '~';
						break;
					default:
						dest[current++] = src[i*2];
						dest[current++] = src[i*2+1];
				}
				break;
			default:
				dest[current++] = src[i*2];
				dest[current++] = src[i*2+1];
		}
	}
	dest[current++] = 0x00;
	dest[current++] = 0x00;
}

bool mystrncasecmp(unsigned char *a, unsigned char *b, int num)
{
	int i=0;
  
//	printf("comparing \"%s\" and \"%s\"\n",a,b);
	while (1) {
		if (a[i] == 0x00) {
			if (b[i] == 0x00) return true;
			return false;
		}
		if (tolower(a[i]) != tolower(b[i])) return false;
		i++;
		if (num == i) return true;
	}
}

/* Compares two Unicode strings without regarding to case.
 * Return true, when they're equal
 */
bool mywstrncasecmp(unsigned char *a, unsigned char *b, int num)
{
	int 		i=0;
 	wchar_t 	wc,wc2;
  
	while (1) {
		if (a[i*2] == 0x00 && a[i*2+1] == 0x00) {
			if (b[i*2] == 0x00 && b[i*2+1] == 0x00) return true;
			return false;
		}
		wc  = a[i*2+1] | (a[i*2] << 8);
		wc2 = b[i*2+1] | (b[i*2] << 8);
		if (mytowlower(wc) != mytowlower(wc2)) return false;
		i++;
		if (num == i) return true;
	}
}

/* wcscmp in Mandrake 9.0 is wrong */
bool mywstrncmp(unsigned char *a, unsigned char *b, int num)
{
	int i=0;
  
	while (1) {
		if (a[i*2] != b[i*2] || a[i*2+1] != b[i*2+1]) return false;
		if (a[i*2] == 0x00 && a[i*2+1] == 0x00) return true;
		i++;
		if (num == i) return true;
	}
}

/* FreeBSD boxes 4.7-STABLE does't have it, although it's ANSI standard */
bool myiswspace(unsigned char *src)
{
#ifndef HAVE_ISWSPACE
 	int 		o;
	unsigned char	dest[10];
#endif
 	wchar_t 	wc;

	wc = src[1] | (src[0] << 8);

#ifndef HAVE_ISWSPACE
	o = DecodeWithUnicodeAlphabet(wc, dest);
	if (o == 1) {
		if (isspace(((int)dest[0]))!=0) return true;
		return false;
	}
	return false;
#else
	return iswspace(wc);
#endif
}

/* FreeBSD boxes 4.7-STABLE does't have it, although it's ANSI standard */
int mytowlower(wchar_t c)
{
#ifndef HAVE_TOWLOWER
	unsigned char dest[10];

	DecodeWithUnicodeAlphabet(c, dest);
	return tolower(dest[0]);
#else
	return towlower(c);
#endif
}