//
// Created by yjw on 8/22/18.
//

#ifndef FACE_RESTCLIENT_H
#define FACE_RESTCLIENT_H

#include "CfgData.h"

class RESTClient
{
public:
    /// 发送人脸及比对信息
    /// \param p_face 人脸及比对信息
    /// \return 成功：true,失败：false.
    static bool SendFace(PFaceInfo p_face);
};

#endif //FACE_RESTCLIENT_H
