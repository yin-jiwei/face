//
// Created by yjw on 8/22/18.
//

#ifndef FACE_RESTCLIENT_H
#define FACE_RESTCLIENT_H

#include "CfgData.h"

class RESTClient
{
public:
    /// 发送人脸及比对信息,抓拍人脸比对库
    /// \param p_face 人脸及比对信息
    /// \return 成功：true,失败：false.
    static bool SendFace(PFaceInfo p_face);

    /// 发送人脸及比对信息,卡照片比对抓拍照片
    /// \param p_face 抓拍人脸
    /// \param p_photo_info 卡信息
    /// \param score 得分
    /// \return 成功：true,失败：false.
    static bool SendFace(PFaceInfo p_face, PPhotoInfo p_photo_info, int score);
};

#endif //FACE_RESTCLIENT_H
