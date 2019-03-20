//
// Created by yjw on 9/14/18.
//

#include "ExtractFeatureThread.h"
#include "Logger.h"
#include "RESTClient.h"
#include "Base64.h"
#include "CfgData.h"

ExtractFeatureThread::ExtractFeatureThread() : feature_extract_((char *) CCfgData::Instance().get_hwdata_path().c_str())
{
    jpeg_data_ = new unsigned char[BMP_SIZE];
    memset(jpeg_data_, 0, BMP_SIZE);
    jpeg_size_ = 0;

    feature_size_ = feature_extract_.Get_feature_len();
    jpeg_feature_ = new unsigned char[feature_size_];
    memset(jpeg_feature_, 0, feature_size_);

    // load feature
    Load_Features_N();

    // start thread
    thread_ = new thread(&ExtractFeatureThread::ThreadFunc, this);
}

ExtractFeatureThread::~ExtractFeatureThread()
{
    // quit thread
    shutdown_ = true;
    CCfgData::Instance().sem_face_avail_.PostAll();
    thread_->join();
    delete thread_;

    if (nullptr != jpeg_data_)
    {
        delete[] jpeg_data_;
        jpeg_data_ = nullptr;
    }

    if (nullptr != jpeg_feature_)
    {
        delete[] jpeg_feature_;
        jpeg_feature_ = nullptr;
    }
}

void ExtractFeatureThread::Load_Features_N()
{
    string log_message;
    if (CCfgData::Instance().photo_feature_count_ == 0)
    {
        log_message.clear();
        log_message = "比对库没有数据，请导入比对库...";
        WARNING_LOG(log_message);

        return;
    }

    // load feature
    try
    {
        face_1_n_.Load_N(CCfgData::Instance().photo_features_, CCfgData::Instance().photo_feature_count_);
    }
    catch (HWException hwe)
    {
        log_message.clear();
        log_message = hwe.what();
        WARNING_LOG(log_message);
    }
}

void ExtractFeatureThread::ThreadFunc()
{
    string log_message;

    log_message = "ExtractFeature thread start...";
    INFO_LOG(log_message);

    while (!shutdown_)
    {
        CCfgData::Instance().sem_face_avail_.Wait();

//        ExtractFeature();
        ExtractFeatureYJ();

    } // end while

    log_message.clear();
    log_message = "ExtractFeature thread exit.";
    INFO_LOG(log_message);
}

void ExtractFeatureThread::ExtractFeature()
{
    string log_message;

    while (true)
    {
        PFaceInfo p_face = CCfgData::Instance().face_info_list_.RemoveHead();
        if (p_face == nullptr)
        {
            break;
        }

        pthread_rwlock_rdlock(&CCfgData::Instance().photo_features_lock_);
        if (CCfgData::Instance().photo_feature_count_ == 0)
        {
            log_message.clear();
            log_message = "比对库没有数据，请导入比对库...";
            WARNING_LOG(log_message);
        }
        else
        {
            // base64 decode
            memset(jpeg_data_, 0, BMP_SIZE);
            jpeg_size_ = 0;
            CBase64::Decode((unsigned char *) p_face->image_data, p_face->image_size, jpeg_data_, jpeg_size_);

            log_message = "ExtractFeature start...";
            INFO_LOG(log_message);

            // extract feature
            int feature_num = 1;
            memset(jpeg_feature_, 0, feature_size_);
            try
            {
                feature_extract_.Extract(jpeg_data_, jpeg_size_, jpeg_feature_, &feature_num);
            }
            catch (HWException hwe)
            {
                feature_num = 0;

                log_message.clear();
                log_message = hwe.what();
                WARNING_LOG(log_message);
            }

            log_message = "ExtractFeature end...";
            INFO_LOG(log_message);

            if (feature_num > 0)
            {
                // 1:N
                int index = 0;
                try
                {
                    face_1_n_.Compare(jpeg_feature_, &index, &p_face->score, 1);
                    p_face->p_photo_info = &CCfgData::Instance().photo_info_list_.at(index);

                    // print result
                    log_message.clear();
                    log_message = p_face->p_photo_info->photo_person_id
                                  + " " + p_face->p_photo_info->photo_person_name
                                  + " " + p_face->p_photo_info->photo_person_department
                                  + " " + to_string(p_face->score * 100);
                    INFO_LOG(log_message);
                }
                catch (HWException hwe)
                {
                    log_message.clear();
                    log_message = hwe.what();
                    WARNING_LOG(log_message);
                }
            }
        }

        // update result
        bool update_flag = CCfgData::Instance().UpdateResult(p_face);

        // send result to gui
        if (update_flag)
        {
            // send face to gui
            RESTClient::SendFace(p_face);
        }
        pthread_rwlock_unlock(&CCfgData::Instance().photo_features_lock_);
    }
}

void ExtractFeatureThread::ExtractFeatureYJ()
{
    string log_message;

    while (true)
    {
        PFaceInfo p_face = CCfgData::Instance().face_info_list_.RemoveHead();
        if (p_face == nullptr)
        {
            break;
        }

        // base64 decode
        memset(jpeg_data_, 0, BMP_SIZE);
        jpeg_size_ = 0;
        CBase64::Decode((unsigned char *) p_face->image_data, p_face->image_size, jpeg_data_, jpeg_size_);

//        // write photo
//        char time_now[64] = {0};
//
//        struct timeval tv;
//        struct timezone tz;
//        gettimeofday(&tv, &tz);
//        struct tm *p = localtime(&tv.tv_sec);
//
//        sprintf(time_now, "%04d%02d%02d%02d%02d%02d%03ld",
//                1900 + p->tm_year,
//                1 + p->tm_mon,
//                p->tm_mday,
//                p->tm_hour,
//                p->tm_min,
//                p->tm_sec,
//                tv.tv_usec / 1000);
//        string photo_path = CCfgData::Instance().get_full_path() + "camera_photos/" + time_now + ".jpg";
//        ofstream photo_stream(photo_path);
//        photo_stream.write((char *) jpeg_data_, jpeg_size_);
//        photo_stream.close();

        log_message = "ExtractFeature start...";
        INFO_LOG(log_message);

        // extract feature
        int feature_num = 1;
        p_face->image_feature = new unsigned char[feature_size_];
        memset(p_face->image_feature, 0, feature_size_);

        try
        {
            feature_extract_.Extract(jpeg_data_, jpeg_size_, p_face->image_feature, &feature_num);
        }
        catch (HWException hwe)
        {
            feature_num = 0;

            log_message.clear();
            log_message = hwe.what();
            WARNING_LOG(log_message);
        }

        log_message = "ExtractFeature end...";
        INFO_LOG(log_message);

        if (feature_num > 0)
        {
            CCfgData::Instance().face_result_list_.Add(p_face);
        }
        else
        {
            delete p_face;
            p_face = nullptr;
        }

        CCfgData::Instance().face_result_list_.Lock();
        if (CCfgData::Instance().face_result_list_.GetSize() >= CCfgData::Instance().get_face_result_maxsize())
        {
            delete CCfgData::Instance().face_result_list_.RemoveHead();
        }
        CCfgData::Instance().face_result_list_.Unlock();
    }
}
