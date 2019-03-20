#include "Face_1_N.h"
#include <iostream>
#include <exception>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <string.h>
#include <string>

using namespace cv;

void *FaceSDKLib::lib_ptr = NULL;

std::string FaceSDKLib::lib_path = "libHW_FACE_SDK_GPU.so";

#define _DEFINE_CALLBACKS(name) tag##name FaceSDKLib::name = NULL

_DEFINE_CALLBACKS(HWGetFaceSDKVersion);
_DEFINE_CALLBACKS(HWGetFaceFeatureSize);
_DEFINE_CALLBACKS(HWInitFaceSDK);
_DEFINE_CALLBACKS(HWReleaseFaceSDK);
_DEFINE_CALLBACKS(HWExtractFaceFeature);
_DEFINE_CALLBACKS(HWExtractImagesFaceFeature);
_DEFINE_CALLBACKS(HWSetFaceExtractParameter);
_DEFINE_CALLBACKS(HWSetFaceDetectParameter);
_DEFINE_CALLBACKS(HWSetFaceQualityCheckParameter);
_DEFINE_CALLBACKS(HWInitFaceFeatureCompareTemplate);
_DEFINE_CALLBACKS(HWInitFaceFeatureCompare);
_DEFINE_CALLBACKS(HWReleaseFaceFeatureCompare);
_DEFINE_CALLBACKS(HWCompareFaceFeature);
_DEFINE_CALLBACKS(HWCompareSingleFaceFeature);

int Feature_Extract::Get_feature_len()
{
    if (NULL == lib_ptr)
    {
        HWLoadFaceSDK();
    }
    int featureSize;
    FaceSDKLib::HWGetFaceFeatureSize(&featureSize);
    return featureSize;
}

Feature_Extract::Feature_Extract(char *model_path)
{
    if (NULL == lib_ptr)
    {
        HWLoadFaceSDK();
    }
    m_extrHandle = NULL;
    if (FaceSDKLib::HWInitFaceSDK(&m_extrHandle, model_path, 0) != HW_FACE_SDK_SUCCESS)
    {
        m_extrHandle = NULL;
        throw HWException("比对句柄未初始化成功！");
    }
}

Feature_Extract::~Feature_Extract()
{
    if (NULL == lib_ptr)
    {
        HWLoadFaceSDK();
    }
    if (NULL != m_extrHandle)
    {
        FaceSDKLib::HWReleaseFaceSDK(m_extrHandle);
    }
}

/*
void jpgmem_to_rgb_gray(unsigned char *jpg,int size, unsigned char **rgb, unsigned char **gray, int *w, int *h)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
     
    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo,jpg,size);
    
    jpeg_read_header(&cinfo,TRUE);
    jpeg_start_decompress(&cinfo);
    
    unsigned long width = cinfo.output_width;
    unsigned long height = cinfo.output_height;
    unsigned short depth = cinfo.output_components;
    cinfo.out_color_space = JCS_EXT_RGB;
    if(depth != 3)
    {
        throw HWException("jpg图片必须编码为3通道的！");
    } 
    *w = width;
    *h = height;
    *rgb = (unsigned char*)malloc(width*height*depth);
    *gray = (unsigned char*)malloc(width * height);
    memset(*rgb,0,width*height*depth);
    memset(*gray, 0, width*height);
    unsigned char* data_gray = *gray;
    unsigned char* data_rgb = *rgb;
    // JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, width*depth,1);
    JSAMPROW row_pointer[1];
    // unsigned char *point = (*rgb)+(height-cinfo.output_scanline-1)*(width*depth);
    while(cinfo.output_scanline<height)
    {
        size_t gray_line_idx = (height - cinfo.output_scanline - 1) * width;
        size_t rgb_line_idx = gray_line_idx * depth;
        row_pointer[0] = &data_rgb[rgb_line_idx];
        jpeg_read_scanlines(&cinfo,row_pointer, 1);
        for(size_t i = 0; i < width; i++)
        {
            data_gray[gray_line_idx + i] = (30 * data_rgb[rgb_line_idx + i * 3] + 
                59 * data_rgb[rgb_line_idx + i * 3 + 1] + 
                11 * data_rgb[rgb_line_idx + i * 3 + 2] + 50) / 100;
        }
        // jpeg_read_scanlines(&cinfo, buffer, 1);
        // for(size_t i=0; i < cinfo.output_width; i++)
        // {
        //     *rgb[(cinfo.output_scanline - 1)*width*3 + i*3] = buffer[0][i * 3];
        //     *rgb[(cinfo.output_scanline - 1)*width*3 + i*3 + 1] = buffer[0][i * 3 + 1];
        //     *rgb[(cinfo.output_scanline - 1)*width *3+ i*3 + 2] = buffer[0][i * 3 + 2];
        //     *rgb[(cinfo.output_scanline - 1)*width + i] = buffer[0][i];
        // }
        // memcpy(point, *buffer, width*depth);
        // point -= width*depth;
    }
     
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
}
*/

void Feature_Extract::Extract(unsigned char *jpg_buffer, const int len_jpg_buffer, unsigned char *feat, int *pfeat_num)
{
    if (NULL == lib_ptr)
    {
        HWLoadFaceSDK();
    }

    int ftr_len = 0;
    FaceSDKLib::HWGetFaceFeatureSize(&ftr_len);
    // unsigned char* rgb = NULL, *gray = NULL;
    // int size, w, h, bit=24;
    // jpgmem_to_rgb_gray(jpg_buffer, len_jpg_buffer, &rgb, &gray, &w, &h);
    cv::Mat jpgMat(1, len_jpg_buffer, CV_8UC1, (void *) jpg_buffer);
    cv::Mat imgColor = cv::imdecode(jpgMat, CV_LOAD_IMAGE_COLOR);
    cv::Mat imgGray;
    cv::cvtColor(imgColor, imgGray, CV_BGR2GRAY);
    //std::cout << m_extrHandle << ", height = " << imgColor.rows << ", width = "<< imgColor.cols << std::endl;
    HWFrame frame;
    frame.image_color = imgColor.data;
    frame.image_gray = imgGray.data;
    frame.face_result = new HWFaceRect[DEFAULT_MAX_DETECT_FACE_COUNT];
    frame.quality_result = new HWQualityScore[DEFAULT_MAX_DETECT_FACE_COUNT];
    frame.have_feature = new char[DEFAULT_MAX_DETECT_FACE_COUNT];
    frame.feature_result = new unsigned char[DEFAULT_MAX_DETECT_FACE_COUNT * ftr_len];
    int rtn = FaceSDKLib::HWExtractFaceFeature(m_extrHandle, &frame, 1, imgColor.rows, imgColor.cols,
                                               FRAME_IMAGE_FORMAT_InterleavedBGR8u);

    // std::cout<< rtn << std::endl;
    // if(frame.face_count > 0)
    // {

    //     memcpy(feat, frame.m_pbFtr, ftr_len);
    // }
    int count = 0;
    for (size_t i = 0; i < frame.face_count && i < *pfeat_num; i++)
    {
        if (frame.have_feature[i] == 1)
        {
            memcpy(feat + count * ftr_len, frame.feature_result + i * ftr_len, ftr_len);
            count++;
        }
    }
    *pfeat_num = count;
    // std::cout<< count << std::endl;
    //free(rgb);
    //free(gray);
    delete[] frame.face_result;
    delete[] frame.quality_result;
    delete[] frame.have_feature;
    delete[] frame.feature_result;

    if (rtn != HW_FACE_SDK_SUCCESS)
    {
        std::string mess = "提取特征异常 code id : ";
        mess += std::to_string(rtn);
        throw HWException(mess);
    }
}

////////////////////////////////////////////////////////////////////////////////////

Face_1_N::Face_1_N()
{
    if (NULL == lib_ptr)
    {
        HWLoadFaceSDK();
    }
    m_dictHandle = NULL;
    m_compHandle = NULL;
    if (FaceSDKLib::HWInitFaceFeatureCompare(&m_compHandle, 0) != HW_FACE_SDK_SUCCESS)
    {
        m_compHandle = NULL;
        throw HWException("比对句柄未初始化成功！");
    }
}


void Face_1_N::Load_N(unsigned char *feats, int N)
{
    if (NULL == lib_ptr)
    {
        HWLoadFaceSDK();
    }
    if (m_dictHandle != NULL)
    {
        HWReleaseFaceFeatureCompare(m_dictHandle);
        m_dictHandle = NULL;
    }
    int rtn = FaceSDKLib::HWInitFaceFeatureCompareTemplate(&m_dictHandle, 0, feats, N);
    if (rtn != HW_FACE_SDK_SUCCESS)
    {
        std::string mess = "建立Dict失败！ code id : ";
        mess += std::to_string(rtn);
        throw HWException(mess);
    }
}

Face_1_N::~Face_1_N()
{
    if (NULL == lib_ptr)
    {
        HWLoadFaceSDK();
    }
    if (m_dictHandle)
    {
        HWReleaseFaceFeatureCompare(m_dictHandle);
        m_dictHandle = NULL;
    }
    if (m_compHandle)
    {
        HWReleaseFaceFeatureCompare(m_compHandle);
        m_compHandle = NULL;
    }
}


void Face_1_N::Compare(unsigned char *feats, int *p_idx, float *p_score, int M)
{
    if (NULL == lib_ptr)
    {
        HWLoadFaceSDK();
    }

    int rtn = FaceSDKLib::HWCompareFaceFeature(
            m_compHandle,
            m_dictHandle,
            feats, M, 1, p_idx, p_score);
    if (rtn != HW_FACE_SDK_SUCCESS)
    {
        std::string mess = "比对M：N异常 code id : ";
        mess += std::to_string(rtn);
        std::cout << mess << std::endl;
        throw HWException(mess);
    }
}
