#ifndef FACE_1_N_H
#define FACE_1_N_H

#include<vector>
#include<string>
#include<iostream>
#include "HW_FACE_SDK_GPU_dynamic.h"

#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>

#define _DEFINE_CALLBACK_MEMBER(name) tag##name name

struct FaceSDKLib
{
    static void *lib_ptr;
    static std::string lib_path;
    static _DEFINE_CALLBACK_MEMBER(HWGetFaceSDKVersion);
    static _DEFINE_CALLBACK_MEMBER(HWGetFaceFeatureSize);
    static _DEFINE_CALLBACK_MEMBER(HWInitFaceSDK);
    static _DEFINE_CALLBACK_MEMBER(HWReleaseFaceSDK);
    static _DEFINE_CALLBACK_MEMBER(HWExtractFaceFeature);
    static _DEFINE_CALLBACK_MEMBER(HWExtractImagesFaceFeature);
    static _DEFINE_CALLBACK_MEMBER(HWSetFaceExtractParameter);
    static _DEFINE_CALLBACK_MEMBER(HWSetFaceDetectParameter);
    static _DEFINE_CALLBACK_MEMBER(HWSetFaceQualityCheckParameter);
    static _DEFINE_CALLBACK_MEMBER(HWInitFaceFeatureCompareTemplate);
    static _DEFINE_CALLBACK_MEMBER(HWInitFaceFeatureCompare);
    static _DEFINE_CALLBACK_MEMBER(HWReleaseFaceFeatureCompare);
    static _DEFINE_CALLBACK_MEMBER(HWCompareFaceFeature);
    static _DEFINE_CALLBACK_MEMBER(HWCompareSingleFaceFeature);

    static void SetLibPath(std::string path)
    {
        FaceSDKLib::lib_path = path;
    }
    //tagHWInitCompareExtendDict_GPU HWInitCompareExtendDict_GPU;
    //tagHWCompareExtend_GPU         HWCompareExtend_GPU;
};

#define _ASSIGN_CALLBACK(x) do{FaceSDKLib::x = (tag##x)dlsym(lib, #x); if(!FaceSDKLib::x) return false; }while(0)

static bool HWLoadFaceSDK()
{
    void *lib = dlopen(FaceSDKLib::lib_path.c_str(), RTLD_LAZY);
    if (lib == NULL)
        return false;
    FaceSDKLib::lib_ptr = lib;
    _ASSIGN_CALLBACK(HWGetFaceSDKVersion);
    _ASSIGN_CALLBACK(HWGetFaceFeatureSize);
    _ASSIGN_CALLBACK(HWInitFaceSDK);
    _ASSIGN_CALLBACK(HWReleaseFaceSDK);
    _ASSIGN_CALLBACK(HWExtractFaceFeature);
    _ASSIGN_CALLBACK(HWExtractImagesFaceFeature);
    _ASSIGN_CALLBACK(HWSetFaceExtractParameter);
    _ASSIGN_CALLBACK(HWSetFaceDetectParameter);
    _ASSIGN_CALLBACK(HWSetFaceQualityCheckParameter);
    _ASSIGN_CALLBACK(HWInitFaceFeatureCompareTemplate);
    _ASSIGN_CALLBACK(HWInitFaceFeatureCompare);
    _ASSIGN_CALLBACK(HWReleaseFaceFeatureCompare);
    _ASSIGN_CALLBACK(HWCompareFaceFeature);
    _ASSIGN_CALLBACK(HWCompareSingleFaceFeature);
    return true;
}

#undef _ASSIGN_CALLBACK

static bool HWFreeFaceSDK()
{
    dlclose(FaceSDKLib::lib_ptr);
    //memset(&l, 0, sizeof(FaceSDKLib));
}

struct HWException : std::exception
{
    std::string m_message;

    HWException(std::string str) noexcept
    {
        m_message = str;
    }

    const char *what() const noexcept
    {
        return m_message.c_str();
    }
};


class Feature_Extract : public FaceSDKLib
{
    void *m_extrHandle;
public:
    static int Get_feature_len();

    Feature_Extract(char *model_path);

    ~Feature_Extract();

    //feat外部分配外部释放，大小必须大于等于feature_len
    //pfeat_num 输入，表示feat在外部分配的空间最多支持多少个特征，feat分配的长度为(*pfeat_num)*feature_len
    //          输出，表示从jpg图片中提取的特征数
    void Extract(
            unsigned char *jpg_buffer, const int len_jpg_buffer,
            unsigned char *feat, int *pfeat_num);
};


class Face_1_N : public FaceSDKLib
{
    void *m_compHandle;
    void *m_dictHandle;
public:
    Face_1_N();

    //N : N个特征，总长度是N×feature_len
    //外部分配，外部释放
    void Load_N(unsigned char *feats, int N);

    ~Face_1_N(void);

public:
    //所有的指针外部分配好
    //M 表示M个特征，
    //feats的大小是M×feature_len
    //p_idx M个最近特征的ID，
    //p_score M个最近特征的相似度
    void Compare(unsigned char *feats, int *p_idx, float *p_score, int M);
};

#endif// FACE_1_N_H
