//
// Created by yjw on 8/14/18.
//

#ifndef FACE_CFGDATA_H
#define FACE_CFGDATA_H

#include <string>
#include <vector>
#include <memory.h>
#include <sqlite3.h>
#include "ThreadSafeArray.h"
#include "Semaphore.h"
#include "ExtractFeatureThread.h"
#include "PhotoExtractFeature.h"

#define BMP_SIZE                        (1920*1200*3 + 100)

typedef struct PhotoInfo
{
    string photo_person_id;
    string photo_uuid;
    string photo_person_name;
    string photo_person_department;
    string photo_path;


    PhotoInfo()
    {
        photo_person_id.clear();
        photo_uuid.clear();
        photo_person_name.clear();
        photo_person_department.clear();
        photo_path.clear();

    }

    ~PhotoInfo()
    {
    }
} *PPhotoInfo;

typedef struct FaceInfo
{
    unsigned long person_id;  /// camera person id
    int new_person; /// 0:yes; 1:not.

    char *image_data;
    int image_size;

    int image_width;
    int image_height;

    float score;

    // photo information
    PPhotoInfo p_photo_info;

    // face create time
    time_t create_time = 0;
    unsigned char *image_feature = nullptr;

    FaceInfo()
    {
        person_id = 0;
        new_person = 0;

        image_data = new char[BMP_SIZE];
        memset(image_data, 0, BMP_SIZE);
        image_size = 0;

        image_width = 0;
        image_height = 0;

        score = 0;

        p_photo_info = nullptr;
    }

    ~FaceInfo()
    {
        if (nullptr != image_data)
        {
            delete[] image_data;
            image_data = nullptr;
        }

        if (nullptr != image_feature)
        {
            delete[] image_feature;
            image_feature = nullptr;
        }
    }
} *PFaceInfo;

typedef struct CameraInfo
{
    string ip;
    int heartbeat;

    CameraInfo()
    {
        ip = "";
        heartbeat = 0;
    }

    ~CameraInfo()
    {

    }
} *PCameraInfo;

class CCfgData
{
public:
    virtual ~CCfgData();

    static CCfgData &Instance()
    {
        return instance_;
    }

    void Fetch();

    void FetchFeature();

    void AddFeature(PhotoInfo pi, const unsigned char *photo_feature, int feature_size);

    const string &get_full_path()
    {
        return full_path_;
    }

    const string &get_program_name()
    {
        return program_name_;
    }

    bool get_log_verbose()
    {
        return log_verbose_;
    }

    const string &get_hwdata_path()
    {
        return hwdata_path_;
    }

    const string &get_local_server_ip()
    {
        return local_server_ip_;
    }

    int get_local_server_port()
    {
        return local_server_port_;
    }

    const string &get_gui_server_ip()
    {
        return gui_server_ip_;
    }

    int get_gui_server_port()
    {
        return gui_server_port_;
    }

    int get_pass_similarity()
    {
        return pass_similarity_;
    }

    int get_face_result_maxsize()
    {
        return face_result_maxsize_;
    }

    int get_face_valid_second()
    {
        return face_valid_second_;
    }

    int get_extract_thread_num()
    {
        return extract_thread_num_;
    }

    PCameraInfo FindCameraByIP(string &ip, bool is_remove = false);

    bool UpdateResult(PFaceInfo p_face);

private:
    CCfgData();

    void get_photo_feature_count();

public:
    CThreadSafeArray<PCameraInfo> camera_info_list_;
    CThreadSafeArray<PFaceInfo> face_info_list_;
    CThreadSafeArray<PFaceInfo> face_result_list_;

    Semaphore sem_face_avail_;

    sqlite3 *sql_ = nullptr;

    unsigned char *photo_features_ = nullptr;
    int photo_feature_size_ = 0;
    int photo_feature_count_ = 0;
    vector<PhotoInfo> photo_info_list_;
    pthread_rwlock_t photo_features_lock_;

    bool load_feature_flag_ = false;
    bool modify_feature_flag_ = false;

    // Global variable
    PhotoExtractFeature *p_pef_ = nullptr;
    CThreadSafeArray<ExtractFeatureThread *> extract_thread_list_;

private:
    static CCfgData instance_;

    string full_path_;
    string program_name_;

    string config_file_path_;
    bool log_verbose_ = true;
    string hwdata_path_;

    string local_server_ip_ = "192.168.10.10";
    int local_server_port_ = 8091;

    string gui_server_ip_ = "192.168.10.12";
    int gui_server_port_ = 19800;

    int pass_similarity_ = 60;
    int face_result_maxsize_ = 50;
    int face_valid_second_ = 5;
    int extract_thread_num_ = 1;
};


#endif //FACE_CFGDATA_H
