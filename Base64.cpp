//
// Created by yjw on 8/14/18.
//

#include "Base64.h"

const char CBase64::EncodeTable[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char CBase64::DecodeTable[123] =
        {
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                62, // '+'
                0, 0, 0,
                63, // '/'
                52, 53, 54, 55, 56, 57, 58, 59, 60, 61, // '0'-'9'
                0, 0, 0, 0, 0, 0, 0,
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, // 'A'-'Z'
                0, 0, 0, 0, 0, 0,
                26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
                39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, // 'a'-'z'
        };

CBase64::CBase64(void)
{
}

CBase64::~CBase64(void)
{
}

void CBase64::Encode(const unsigned char *strData, int DataByte, unsigned char *strEncode)
{
    unsigned char Tmp[4] = {0};

    for (int i = 0; i < DataByte / 3; i++)
    {
        Tmp[1] = *strData++;
        Tmp[2] = *strData++;
        Tmp[3] = *strData++;
        *strEncode++ = EncodeTable[Tmp[1] >> 2];
        *strEncode++ = EncodeTable[((Tmp[1] << 4) | (Tmp[2] >> 4)) & 0x3F];
        *strEncode++ = EncodeTable[((Tmp[2] << 2) | (Tmp[3] >> 6)) & 0x3F];
        *strEncode++ = EncodeTable[Tmp[3] & 0x3F];
    }

    // 对剩余数据进行编码
    int Mod = DataByte % 3;
    if (Mod == 1)
    {
        Tmp[1] = *strData++;
        *strEncode++ = EncodeTable[(Tmp[1] & 0xFC) >> 2];
        *strEncode++ = EncodeTable[((Tmp[1] & 0x03) << 4)];
        *strEncode++ = '=';
        *strEncode++ = '=';
    }
    else if (Mod == 2)
    {
        Tmp[1] = *strData++;
        Tmp[2] = *strData++;
        *strEncode++ = EncodeTable[(Tmp[1] & 0xFC) >> 2];
        *strEncode++ = EncodeTable[((Tmp[1] & 0x03) << 4) | ((Tmp[2] & 0xF0) >> 4)];
        *strEncode++ = EncodeTable[((Tmp[2] & 0x0F) << 2)];
        *strEncode++ = '=';
    }
}

void CBase64::Decode(const unsigned char *strData, int DataByte, unsigned char *strDecode, int &OutByte)
{
    int nValue;
    int i = 0;
    while (i < DataByte)
    {
        if (*strData != '\r' && *strData != '\n')
        {
            nValue = DecodeTable[*strData++] << 18;
            nValue += DecodeTable[*strData++] << 12;
            *strDecode++ = (nValue & 0x00ff0000) >> 16;
            OutByte++;
            if (*strData != '=')
            {
                nValue += DecodeTable[*strData++] << 6;
                *strDecode++ = (nValue & 0x0000FF00) >> 8;
                OutByte++;
                if (*strData != '=')
                {
                    nValue += DecodeTable[*strData++];
                    *strDecode++ = nValue & 0x000000FF;
                    OutByte++;
                }
            }

            i += 4;
        }
        else
        {
            strData++;
            i++;
        }
    }
}
