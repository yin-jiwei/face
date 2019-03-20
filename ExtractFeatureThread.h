//
// Created by yjw on 9/14/18.
//

#ifndef FACE_EXTRACTFEATURETHREAD_H
#define FACE_EXTRACTFEATURETHREAD_H

#include <thread>
#include "Face_1_N.h"

class ExtractFeatureThread
{
public:
    ExtractFeatureThread();

    virtual ~ExtractFeatureThread();

    void Load_Features_N();

private:
    void ThreadFunc();

    void ExtractFeature();

    void ExtractFeatureYJ();

private:
    std::thread *thread_ = nullptr;
    bool shutdown_ = false;

    Feature_Extract feature_extract_;
    Face_1_N face_1_n_;

    unsigned char *jpeg_data_ = nullptr;
    int jpeg_size_ = 0;

    unsigned char *jpeg_feature_ = nullptr;
    int feature_size_ = 0;
};


#endif //FACE_EXTRACTFEATURETHREAD_H
