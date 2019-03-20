//
// Created by yjw on 8/14/18.
//

#ifndef FACE_BASE64_H
#define FACE_BASE64_H

class CBase64
{
public:
    CBase64(void);

    virtual ~CBase64(void);

    /// 编码
    /// \param pData [in]原始数据
    /// \param DataByte [in]输入的数据长度，以字节为单位
    /// \param strEncode [out]编码后存储空间
    static void Encode(const unsigned char *pData, int DataByte, unsigned char *strEncode);

    /// 解码
    /// \param pData [in]原始数据
    /// \param DataByte [in]输入的数据长度，以字节为单位
    /// \param strDecode [out]解码后存储空间
    /// \param OutByte  [out]输出的数据长度，以字节为单位，请不要通过返回值计算输出数据的长度
    static void Decode(const unsigned char *pData, int DataByte, unsigned char *strDecode, int &OutByte);

private:
    static const char EncodeTable[65];
    static const char DecodeTable[123];
};

#endif //FACE_BASE64_H
