//
// Created by yjw on 10/24/18.
//

#include <fstream>
#include "PhotoExtractFeature.h"
#include "Logger.h"
#include "CfgData.h"
#include "Base64.h"
#include "RESTClient.h"

PhotoExtractFeature::PhotoExtractFeature() : feature_extract_((char *) CCfgData::Instance().get_hwdata_path().c_str())
{
    jpeg_data_ = new unsigned char[BMP_SIZE];
    memset(jpeg_data_, 0, BMP_SIZE);
    jpeg_size_ = 0;

    feature_size_ = feature_extract_.Get_feature_len();
    jpeg_feature_ = new unsigned char[feature_size_];
    memset(jpeg_feature_, 0, feature_size_);

    CCfgData::Instance().photo_feature_size_ = feature_size_;
}

PhotoExtractFeature::~PhotoExtractFeature()
{
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

void PhotoExtractFeature::ExtractFeature()
{
    lock_guard<mutex> guard(safe_mutex_);

    string log_message;

    log_message.clear();
    log_message = "PhotoExtractFeature start...";
    INFO_LOG(log_message);

    string sql_command;
    sqlite3_stmt *select_stmt;
    sqlite3_stmt *update_stmt;
    char *p_err_msg;
    int rtn_code;

    // prepare select_stmt
    sql_command.clear();
    sql_command = "SELECT compare_id, photo FROM person WHERE feature IS NULL;";
    rtn_code = sqlite3_prepare_v2(CCfgData::Instance().sql_, sql_command.c_str(), -1, &select_stmt, nullptr);
    if (SQLITE_OK != rtn_code)
    {
        log_message.clear();
        log_message = sqlite3_errmsg(CCfgData::Instance().sql_);
        WARNING_LOG(log_message);
        return;
    }

    // prepare update_stmt
    sql_command.clear();
    sql_command = "UPDATE person SET feature = ? WHERE compare_id = ?;";
    rtn_code = sqlite3_prepare_v2(CCfgData::Instance().sql_, sql_command.c_str(), -1, &update_stmt, nullptr);
    if (SQLITE_OK != rtn_code)
    {
        log_message.clear();
        log_message = sqlite3_errmsg(CCfgData::Instance().sql_);
        WARNING_LOG(log_message);

        sqlite3_finalize(select_stmt);
        return;
    }

    rtn_code = sqlite3_exec(CCfgData::Instance().sql_, "BEGIN;", nullptr, nullptr, &p_err_msg);
    if (SQLITE_OK != rtn_code)
    {
        log_message.clear();
        log_message = p_err_msg;
        WARNING_LOG(log_message);

        sqlite3_free(p_err_msg);
    }

    // select data
    while (sqlite3_step(select_stmt) == SQLITE_ROW)
    {
        string compare_id((char *) sqlite3_column_text(select_stmt, 0));
        string photo_name((char *) sqlite3_column_text(select_stmt, 1));

        // read photo data
        memset(jpeg_data_, 0, BMP_SIZE);
        jpeg_size_ = 0;

        string photo_path = CCfgData::Instance().get_full_path() + "photos/" + photo_name;
        ifstream photo_stream(photo_path, ifstream::in | ifstream::binary);
        if (photo_stream)
        {
            photo_stream.seekg(0, ios::end);
            jpeg_size_ = photo_stream.tellg();
            photo_stream.seekg(0, ios::beg);
            photo_stream.read((char *) jpeg_data_, jpeg_size_);

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

            // update sql
            if (feature_num > 0)
            {
                sqlite3_bind_blob(update_stmt, 1, jpeg_feature_, feature_size_, nullptr);
                sqlite3_bind_text(update_stmt, 2, compare_id.c_str(), -1, SQLITE_TRANSIENT);
                rtn_code = sqlite3_step(update_stmt);
                if (SQLITE_DONE != rtn_code)
                {
                    log_message.clear();
                    log_message = sqlite3_errmsg(CCfgData::Instance().sql_);
                    WARNING_LOG(log_message);
                }

                sqlite3_reset(update_stmt);
            }
            else
            {
                log_message.clear();
                log_message = "feature extract failed: " + photo_path;
                WARNING_LOG(log_message);
            }
        }
    }

    rtn_code = sqlite3_exec(CCfgData::Instance().sql_, "COMMIT;", nullptr, nullptr, &p_err_msg);
    if (SQLITE_OK != rtn_code)
    {
        log_message.clear();
        log_message = p_err_msg;
        WARNING_LOG(log_message);

        sqlite3_free(p_err_msg);
    }

    sqlite3_finalize(update_stmt);
    sqlite3_finalize(select_stmt);

    log_message.clear();
    log_message = "PhotoExtractFeature end...";
    INFO_LOG(log_message);
}

bool PhotoExtractFeature::ExtractFeature(std::string &photo_id,
                                         std::string &compare_id,
                                         std::string &photo_name,
                                         std::string &photo_department,
                                         const char *image_data64)
{
    lock_guard<mutex> guard(safe_mutex_);
    bool rtn = true;
    string log_message;

    log_message.clear();
    log_message = "PhotoExtractFeature start...";
    INFO_LOG(log_message);

    string photo_path_name = compare_id + ".jpg";

    // base64 decode
    memset(jpeg_data_, 0, BMP_SIZE);
    jpeg_size_ = 0;

    CBase64::Decode((unsigned char *) image_data64, strlen(image_data64), jpeg_data_, jpeg_size_);

//    log_message.clear();
//    log_message = "compareID: " + compare_id + ", base64 size: " + to_string(strlen(image_data64))
//                  + ", jpeg size: " + to_string(jpeg_size_) + ".";
//    INFO_LOG(log_message);

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

        rtn = false;
    }
//    catch (exception e)
//    {
//        feature_num = 0;
//
//        log_message.clear();
//        log_message = e.what();
//        WARNING_LOG(log_message);
//
//        rtn = false;
//    }

    log_message = "ExtractFeature end...";
    INFO_LOG(log_message);

    // update sql
    if (feature_num > 0)
    {
        string sql_command;
        sqlite3_stmt *insert_stmt;
        int rtn_code;

        pthread_rwlock_wrlock(&CCfgData::Instance().photo_features_lock_);
        // prepare insert_stmt
        sql_command.clear();
        sql_command = "INSERT INTO person VALUES ( ?, ?, ?, ?, ?, ? );";
        rtn_code = sqlite3_prepare_v2(CCfgData::Instance().sql_, sql_command.c_str(), -1, &insert_stmt, nullptr);
        if (SQLITE_OK != rtn_code)
        {
            log_message.clear();
            log_message = sqlite3_errmsg(CCfgData::Instance().sql_);
            WARNING_LOG(log_message);
            return false;
        }

        sqlite3_bind_text(insert_stmt, 1, photo_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert_stmt, 2, compare_id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert_stmt, 3, photo_name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert_stmt, 4, photo_department.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(insert_stmt, 5, photo_path_name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_blob(insert_stmt, 6, jpeg_feature_, feature_size_, nullptr);

        rtn_code = sqlite3_step(insert_stmt);
        if (SQLITE_DONE != rtn_code)
        {
            log_message.clear();
            log_message = sqlite3_errmsg(CCfgData::Instance().sql_);
            WARNING_LOG(log_message);
            rtn = false;
        }
        else
        {
            // write photo
            string photo_path = CCfgData::Instance().get_full_path() + "photos/" + photo_path_name;
            ofstream photo_stream(photo_path);
            photo_stream.write((char *) jpeg_data_, jpeg_size_);
            photo_stream.close();
        }

        sqlite3_reset(insert_stmt);
        sqlite3_finalize(insert_stmt);

        pthread_rwlock_unlock(&CCfgData::Instance().photo_features_lock_);

        // add feature
//        PhotoInfo pi;
//        pi.id = id;
//        pi.uuid = compare_id;
//        pi.name = name;
//        pi.department = department;
//        pi.photo_path = photo_path_name;
//
//        pthread_rwlock_wrlock(&CCfgData::Instance().photo_features_lock_);
//        // add feature
//        CCfgData::Instance().AddFeature(pi, jpeg_feature_, feature_size_);
//
//        CCfgData::Instance().p_pef_->Load_Features_N();
//
//        // update ExtractFeatureThread
//        for (int i = 0; i < CCfgData::Instance().extract_thread_list_.GetSize(); ++i)
//        {
//            ExtractFeatureThread *extract_thread = CCfgData::Instance().extract_thread_list_.GetAt(i);
//            extract_thread->Load_Features_N();
//        }
//        pthread_rwlock_unlock(&CCfgData::Instance().photo_features_lock_);
    }
    else
    {
        log_message.clear();
        log_message = "feature extract failed: " + photo_id + "_" + photo_name;
        WARNING_LOG(log_message);

        rtn = false;
    }

    log_message.clear();
    log_message = "PhotoExtractFeature end...";
    INFO_LOG(log_message);

    return rtn;
}

bool PhotoExtractFeature::RetrieveFace(const char *image_data64,
                                       float &score,
                                       PhotoInfo **pp_photo_info,
                                       char *face_data)
{
    lock_guard<mutex> guard(safe_mutex_);
    bool rtn = true;
    string log_message;

    log_message.clear();
    log_message = "/face/retrieval RetrieveFace start...";
    INFO_LOG(log_message);

    *pp_photo_info = nullptr;

    if (CCfgData::Instance().photo_feature_count_ == 0)
    {
        log_message.clear();
        log_message = "比对库没有数据，请导入比对库...";
        WARNING_LOG(log_message);
        rtn = false;
    }
    else
    {
        // base64 decode
        memset(jpeg_data_, 0, BMP_SIZE);
        jpeg_size_ = 0;

        CBase64::Decode((unsigned char *) image_data64, strlen(image_data64), jpeg_data_, jpeg_size_);

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

            rtn = false;
        }

        log_message = "ExtractFeature end...";
        INFO_LOG(log_message);

        if (feature_num > 0)
        {
            // 1:N
            int index = 0;
            pthread_rwlock_rdlock(&CCfgData::Instance().photo_features_lock_);

            try
            {
                face_1_n_.Compare(jpeg_feature_, &index, &score, 1);
                PPhotoInfo p_photo_info = &CCfgData::Instance().photo_info_list_.at(index);
                *pp_photo_info = p_photo_info;

                // print result
                log_message.clear();
                log_message = p_photo_info->id
                              + " " + p_photo_info->uuid
                              + " " + p_photo_info->name
                              + " " + p_photo_info->department
                              + " " + to_string(score * 100);
                INFO_LOG(log_message);

                // read photo data
                string photo_path = CCfgData::Instance().get_full_path() + "photos/" + p_photo_info->photo_path;
                ifstream photo_stream(photo_path, ifstream::in | ifstream::binary);
                if (photo_stream)
                {
                    photo_stream.seekg(0, ios::end);
                    jpeg_size_ = photo_stream.tellg();
                    photo_stream.seekg(0, ios::beg);
                    photo_stream.read((char *) jpeg_data_, jpeg_size_);

                    // base64 encode
                    CBase64::Encode(jpeg_data_, jpeg_size_, (unsigned char *) face_data);
                }
            }
            catch (HWException hwe)
            {
                log_message.clear();
                log_message = hwe.what();
                WARNING_LOG(log_message);
                rtn = false;
            }

            pthread_rwlock_unlock(&CCfgData::Instance().photo_features_lock_);
        }
        else
        {
            log_message.clear();
            log_message = "feature extract failed.";
            WARNING_LOG(log_message);

            rtn = false;
        }
    }

    log_message.clear();
    log_message = "/face/retrieval RetrieveFace end...";
    INFO_LOG(log_message);

    return rtn;
}

bool PhotoExtractFeature::RetrieveFace(const char *image_data64, int &score, char *face_data)
{
    lock_guard<mutex> guard(safe_mutex_);
    bool rtn = true;
    string log_message;

    log_message.clear();
    log_message = "getScore RetrieveFace start...";
    INFO_LOG(log_message);

    // base64 decode
    memset(jpeg_data_, 0, BMP_SIZE);
    jpeg_size_ = 0;

    CBase64::Decode((unsigned char *) image_data64, strlen(image_data64), jpeg_data_, jpeg_size_);

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

        rtn = false;
    }

    log_message = "ExtractFeature end...";
    INFO_LOG(log_message);

    if (feature_num > 0)
    {
        time_t now_time = time(nullptr);

        // 1:N
        int max_score = 0;
        PFaceInfo p_face_result = nullptr;
        PFaceInfo p_face = nullptr;

        // load feature
        face_1_n_.Load_N(jpeg_feature_, 1);

        CCfgData::Instance().face_feature_list_.Lock();
        for (int i = 0; i < CCfgData::Instance().face_feature_list_.GetSize(); ++i)
        {
            p_face = CCfgData::Instance().face_feature_list_.GetAt(i);

            time_t interval_second = now_time - p_face->create_time;
            if (interval_second > CCfgData::Instance().get_face_valid_second())
            {
                continue;
            }

            try
            {
                int index = 0;
                float similarity = 0.0;

                // load feature
                face_1_n_.Compare(p_face->image_feature, &index, &similarity, 1);

                int percent_similarity = (int) (similarity * 100);
                if (max_score < percent_similarity)
                {
                    max_score = percent_similarity;
                    p_face_result = p_face;
                }
            }
            catch (HWException hwe)
            {
                log_message.clear();
                log_message = hwe.what();
                WARNING_LOG(log_message);
            }
        }

        if (nullptr != p_face_result)
        {
            score = max_score;
            memcpy(face_data, p_face_result->image_base64, p_face_result->image_size);
        }

        CCfgData::Instance().face_feature_list_.Unlock();

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
//
//        // base64
//        string base64_path = CCfgData::Instance().get_full_path() + "photos/" + time_now + ".txt";
//
//        ofstream base64_stream(base64_path);
//        base64_stream.write(face_data, strlen(face_data));
//        base64_stream.close();
//
//        // card jpg
//        string card_path = CCfgData::Instance().get_full_path() + "photos/" + time_now + "_card.jpg";
//
//        ofstream card_stream(card_path);
//        card_stream.write((char *) jpeg_data_, jpeg_size_);
//        card_stream.close();
//
//        // jpg
//        string photo_path = CCfgData::Instance().get_full_path() + "photos/" + time_now + ".jpg";
//
//        // base64 decode
//        memset(jpeg_data_, 0, BMP_SIZE);
//        jpeg_size_ = 0;
//        CBase64::Decode((unsigned char *) face_data, strlen(face_data), jpeg_data_, jpeg_size_);
//
//        ofstream photo_stream(photo_path);
//        photo_stream.write((char *) jpeg_data_, jpeg_size_);
//        photo_stream.close();
    }
    else
    {
        log_message.clear();
        log_message = "feature extract failed.";
        WARNING_LOG(log_message);

        rtn = false;
    }

    log_message.clear();
    log_message = "getScore RetrieveFace end...";
    INFO_LOG(log_message);

    return rtn;
}

bool PhotoExtractFeature::RetrieveFace(PhotoInfo *p_photo_info)
{
    lock_guard<mutex> guard(safe_mutex_);
    bool rtn = true;
    string log_message;

    log_message.clear();
    log_message = "snapshots/retrieval RetrieveFace start...";
    INFO_LOG(log_message);

    // base64 decode
    memset(jpeg_data_, 0, BMP_SIZE);
    jpeg_size_ = 0;

    CBase64::Decode((unsigned char *) p_photo_info->image_base64, p_photo_info->image_size, jpeg_data_, jpeg_size_);

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

        rtn = false;
    }

    log_message = "ExtractFeature end...";
    INFO_LOG(log_message);

    if (feature_num > 0)
    {
        time_t now_time = time(nullptr);

        // 1:N
        int max_score = 0;
        PFaceInfo p_face_result = nullptr;
        PFaceInfo p_face = nullptr;

        // load feature
        face_1_n_.Load_N(jpeg_feature_, 1);

        CCfgData::Instance().face_feature_list_.Lock();
        for (int i = 0; i < CCfgData::Instance().face_feature_list_.GetSize(); ++i)
        {
            p_face = CCfgData::Instance().face_feature_list_.GetAt(i);

            time_t interval_second = now_time - p_face->create_time;
            if (interval_second > CCfgData::Instance().get_face_valid_second())
            {
                continue;
            }

            try
            {
                int index = 0;
                float similarity = 0.0;

                // load feature
                face_1_n_.Compare(p_face->image_feature, &index, &similarity, 1);

                int percent_similarity = (int) (similarity * 100);
                if (max_score < percent_similarity)
                {
                    max_score = percent_similarity;
                    p_face_result = p_face;
                }
            }
            catch (HWException hwe)
            {
                log_message.clear();
                log_message = hwe.what();
                WARNING_LOG(log_message);
            }
        }

        RESTClient::SendFace(p_face_result, p_photo_info, max_score);

        CCfgData::Instance().face_feature_list_.Unlock();

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
//
//        // base64
//        string base64_path = CCfgData::Instance().get_full_path() + "photos/" + time_now + ".txt";
//
//        ofstream base64_stream(base64_path);
//        base64_stream.write(face_data, strlen(face_data));
//        base64_stream.close();
//
//        // card jpg
//        string card_path = CCfgData::Instance().get_full_path() + "photos/" + time_now + "_card.jpg";
//
//        ofstream card_stream(card_path);
//        card_stream.write((char *) jpeg_data_, jpeg_size_);
//        card_stream.close();
//
//        // jpg
//        string photo_path = CCfgData::Instance().get_full_path() + "photos/" + time_now + ".jpg";
//
//        // base64 decode
//        memset(jpeg_data_, 0, BMP_SIZE);
//        jpeg_size_ = 0;
//        CBase64::Decode((unsigned char *) face_data, strlen(face_data), jpeg_data_, jpeg_size_);
//
//        ofstream photo_stream(photo_path);
//        photo_stream.write((char *) jpeg_data_, jpeg_size_);
//        photo_stream.close();
    }
    else
    {
        log_message.clear();
        log_message = "feature extract failed.";
        WARNING_LOG(log_message);

        rtn = false;
    }

    log_message.clear();
    log_message = "snapshots/retrieval RetrieveFace end...";
    INFO_LOG(log_message);

    return rtn;
}

void PhotoExtractFeature::Load_Features_N()
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