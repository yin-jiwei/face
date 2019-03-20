//
// Created by yjw on 8/14/18.
//

#include <libxml/parser.h>
#include "CfgData.h"
#include "Logger.h"

CCfgData CCfgData::instance_;

CCfgData::CCfgData()
{
    // full path
    char c_path[1024];
    int len = readlink("/proc/self/exe", c_path, 1024);
    if (len < 0 || len >= 1024)
    {
        return;
    }

    c_path[len] = '\0';

    // program name
    char *p_name = strrchr(c_path, '/') + 1;
    program_name_ = p_name;

    // absolute path
    *p_name = '\0';
    full_path_ = c_path;

    // config path
    config_file_path_ = full_path_ + "configure.xml";
    hwdata_path_ = full_path_ + "HWFaceData.bin";

//    // create camera_photos directory
//    string camera_photos_path = full_path_ + "camera_photos";
//    if (access(camera_photos_path.c_str(), F_OK) == -1)
//    {
//        if (mkdir(camera_photos_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
//        {
//            cout << "can't create photos directory:" << camera_photos_path << endl;
//        }
//    }

    // create photos directory
    string photos_path = full_path_ + "photos";
    if (access(photos_path.c_str(), F_OK) == -1)
    {
        if (mkdir(photos_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
        {
            cout << "can't create photos directory:" << photos_path << endl;
        }
    }

    string sqlite_file_path = full_path_ + "face.sqlite";

    // init photo_features_lock_
    pthread_rwlock_init(&photo_features_lock_, nullptr);

    // open database sqlite3
    int rtn_code = sqlite3_open(sqlite_file_path.c_str(), &sql_);
    if (SQLITE_OK != rtn_code)
    {
        cout << "can't open database:" << sqlite3_errmsg(sql_) << endl;
    }
    else
    {
        cout << "open database successfully" << endl;
    }
}

CCfgData::~CCfgData()
{
    sqlite3_close(sql_);

    face_result_list_.Lock();
    while (face_result_list_.GetSize() > 0)
    {
        delete face_result_list_.RemoveHead();
    }
    face_result_list_.Unlock();

    face_info_list_.Lock();
    while (face_info_list_.GetSize() > 0)
    {
        delete face_info_list_.RemoveHead();
    }
    face_info_list_.Unlock();

    camera_info_list_.Lock();
    while (camera_info_list_.GetSize() > 0)
    {
        delete camera_info_list_.RemoveHead();
    }
    camera_info_list_.Unlock();

    pthread_rwlock_wrlock(&photo_features_lock_);
    if (nullptr != photo_features_)
    {
        delete[] photo_features_;
        photo_features_ = nullptr;
    }
    pthread_rwlock_unlock(&photo_features_lock_);

    pthread_rwlock_destroy(&photo_features_lock_);
}

void CCfgData::Fetch()
{
    xmlDocPtr xml_doc = nullptr;
    xmlNodePtr cur_node = nullptr;
    xmlChar *node_text = nullptr;

    //获取树形结构
    xml_doc = xmlParseFile(config_file_path_.c_str());
    if (xml_doc == nullptr)
    {
        SEVERE_LOG("Failed to parse xml file");
        return;
    }

    //获取根节点
    cur_node = xmlDocGetRootElement(xml_doc);
    if (cur_node == nullptr)
    {
        SEVERE_LOG("Root is empty");
        xmlFreeDoc(xml_doc);
        return;
    }

    if (0 != xmlStrcmp(cur_node->name, BAD_CAST "Config"))
    {
        SEVERE_LOG("The root is not Config");
        xmlFreeDoc(xml_doc);
        return;
    }

    //遍历处理根节点的每一个子节点
    cur_node = cur_node->xmlChildrenNode;
    while (cur_node != nullptr)
    {
        if (0 == xmlStrcmp(cur_node->name, BAD_CAST "LocalServerIP"))
        {
            node_text = xmlNodeGetContent(cur_node);
            local_server_ip_ = (char *) node_text;
        }
        else if (0 == xmlStrcmp(cur_node->name, BAD_CAST "LocalServerPort"))
        {
            node_text = xmlNodeGetContent(cur_node);
            local_server_port_ = atoi((char *) node_text);
        }
        else if (0 == xmlStrcmp(cur_node->name, BAD_CAST "GUIServerIP"))
        {
            node_text = xmlNodeGetContent(cur_node);
            gui_server_ip_ = (char *) node_text;
        }
        else if (0 == xmlStrcmp(cur_node->name, BAD_CAST "GUIServerPort"))
        {
            node_text = xmlNodeGetContent(cur_node);
            gui_server_port_ = atoi((char *) node_text);
        }
        else if (0 == xmlStrcmp(cur_node->name, BAD_CAST "PassSimilarity"))
        {
            node_text = xmlNodeGetContent(cur_node);
            pass_similarity_ = atoi((char *) node_text);
        }
        else if (0 == xmlStrcmp(cur_node->name, BAD_CAST "FaceListMaxSize"))
        {
            node_text = xmlNodeGetContent(cur_node);
            face_result_maxsize_ = atoi((char *) node_text);
        }
        else if (0 == xmlStrcmp(cur_node->name, BAD_CAST "FaceValidSecond"))
        {
            node_text = xmlNodeGetContent(cur_node);
            face_valid_second_ = atoi((char *) node_text);
        }
        else if (0 == xmlStrcmp(cur_node->name, BAD_CAST "ExtractThreadNum"))
        {
            node_text = xmlNodeGetContent(cur_node);
            extract_thread_num_ = atoi((char *) node_text);
        }

        if (node_text != nullptr)
        {
            xmlFree(node_text);
            node_text = nullptr;
        }

        cur_node = cur_node->next;
    }

    //
    xmlFreeDoc(xml_doc);
}

void CCfgData::FetchFeature()
{
    string log_message;

    log_message.clear();
    log_message = "FetchFeature start...";
    INFO_LOG(log_message);

    // clear memory
    if (nullptr != photo_features_)
    {
        delete[] photo_features_;
        photo_features_ = nullptr;
    }

    photo_info_list_.clear();

    // get feature count
    get_photo_feature_count();
    if (photo_feature_count_ == 0)
    {
        return;
    }

    photo_features_ = new unsigned char[photo_feature_size_ * photo_feature_count_];
    unsigned char *p_feature_index = photo_features_;

    string sql_command;
    sqlite3_stmt *select_stmt;
    int rtn_code;

    // prepare select_stmt
    sql_command.clear();
    sql_command = "SELECT * FROM person WHERE feature IS NOT NULL;";
    rtn_code = sqlite3_prepare_v2(CCfgData::Instance().sql_, sql_command.c_str(), -1, &select_stmt, nullptr);
    if (SQLITE_OK != rtn_code)
    {
        log_message.clear();
        log_message = sqlite3_errmsg(CCfgData::Instance().sql_);
        WARNING_LOG(log_message);
        return;
    }

    // select data
    while (sqlite3_step(select_stmt) == SQLITE_ROW)
    {
        PhotoInfo pi;
        pi.photo_person_id = (char *) sqlite3_column_text(select_stmt, 0);
        pi.photo_uuid = (char *) sqlite3_column_text(select_stmt, 1);
        pi.photo_person_name = (char *) sqlite3_column_text(select_stmt, 2);
        pi.photo_person_department = (char *) sqlite3_column_text(select_stmt, 3);
        pi.photo_path = (char *) sqlite3_column_text(select_stmt, 4);

        const void *photo_feature = sqlite3_column_blob(select_stmt, 5);
        int feature_size = sqlite3_column_bytes(select_stmt, 5);

        photo_info_list_.push_back(pi);
        memcpy(p_feature_index, photo_feature, feature_size);
        p_feature_index += feature_size;
    }

    sqlite3_finalize(select_stmt);

    log_message.clear();
    log_message = "FetchFeature end...";
    INFO_LOG(log_message);
}

void CCfgData::AddFeature(PhotoInfo pi, const unsigned char *photo_feature, int feature_size)
{
    string log_message;

    log_message.clear();
    log_message = "AddFeature start...";
    INFO_LOG(log_message);

    // add feature count
    photo_feature_count_++;

    unsigned char *p_temp = photo_features_;

    photo_features_ = new unsigned char[photo_feature_size_ * photo_feature_count_];
    memcpy(photo_features_, p_temp, photo_feature_size_ * (photo_feature_count_ - 1));

    unsigned char *p_feature_index = photo_features_ + photo_feature_size_ * (photo_feature_count_ - 1);
    memcpy(p_feature_index, photo_feature, feature_size);

    photo_info_list_.push_back(pi);

    // clear memory
    if (nullptr != p_temp)
    {
        delete[] p_temp;
        p_temp = nullptr;
    }

    log_message.clear();
    log_message = "AddFeature end...";
    INFO_LOG(log_message);
}

void CCfgData::get_photo_feature_count()
{
    string log_message;

    string sql_command;
    sqlite3_stmt *select_stmt;
    int rtn_code;

    // prepare select_stmt
    sql_command.clear();
    sql_command = "SELECT COUNT(*) FROM person WHERE feature IS NOt NULL;";
    rtn_code = sqlite3_prepare_v2(CCfgData::Instance().sql_, sql_command.c_str(), -1, &select_stmt, nullptr);
    if (SQLITE_OK != rtn_code)
    {
        log_message.clear();
        log_message = sqlite3_errmsg(CCfgData::Instance().sql_);
        WARNING_LOG(log_message);
        return;
    }

    // select data
    while (sqlite3_step(select_stmt) == SQLITE_ROW)
    {
        photo_feature_count_ = sqlite3_column_int(select_stmt, 0);
    }

    sqlite3_finalize(select_stmt);
}

PCameraInfo CCfgData::FindCameraByIP(string &ip, bool is_remove /* = false */)
{
    PCameraInfo p_ret = nullptr;

    camera_info_list_.Lock();
    for (unsigned int i = 0; i < camera_info_list_.GetSize(); i++)
    {
        PCameraInfo p_camera = nullptr;

        p_camera = camera_info_list_.GetAt(i);
        if (p_camera->ip == ip)
        {
            p_ret = p_camera;

            if (is_remove)
            {
                camera_info_list_.RemoveAt(i);
            }

            break;
        }
    }
    camera_info_list_.Unlock();

    return p_ret;
}

bool CCfgData::UpdateResult(PFaceInfo p_face)
{
    bool update_flag = true;

    face_result_list_.Lock();
    if (face_result_list_.GetSize() >= get_face_result_maxsize())
    {
        delete face_result_list_.RemoveHead();
    }

    for (unsigned int i = 0; i < face_result_list_.GetSize(); i++)
    {
        PFaceInfo p_find = face_result_list_.GetAt(i);
        if (p_find->person_id == p_face->person_id)
        {
            if (p_find->score >= p_face->score)
            {
                update_flag = false;
            }
            else
            {
                face_result_list_.RemoveAt(i);
                delete p_find;
            }

            break;
        }
    }

    if (update_flag)
    {
        // add result list
        face_result_list_.Add(p_face);
    }
    else
    {
        delete p_face;
    }
    face_result_list_.Unlock();

    return update_flag;
}