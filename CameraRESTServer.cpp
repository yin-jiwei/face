//
// Created by yjw on 8/9/18.
//

#include <time.h>
#include <sstream>
//#include <uuid/uuid.h>
#include "CameraRESTServer.h"
#include "threads.h"
#include "Logger.h"
#include "CfgData.h"
#include "Base64.h"

#define BACKLOG (100)    // Max. request backlog

using namespace std;

/* Don't need a namespace table. We put an empty one here to avoid link errors */
struct Namespace namespaces[] = {{nullptr, nullptr}};

CameraRESTServer::CameraRESTServer()
{
    thread_id_ = 0;
    shutdown_ = false;

    ip_ = "";
    port_ = 8091;

    soap_ctx_ = nullptr;
}

CameraRESTServer::~CameraRESTServer()
{

}

void CameraRESTServer::StartServer(string &ip, int port)
{
    ip_ = ip;
    port_ = port;

    StartThread();
}

void CameraRESTServer::StopServer()
{
    QuitThread();
}

bool CameraRESTServer::StartThread()
{
    if (thread_id_ > 0 || shutdown_)
    {
        return false;
    }

    int ret;
    ret = pthread_create(&thread_id_, nullptr, ThreadProc, this);
    if (ret != 0)
    {
        perror("CameraRESTServer Thread creation failed");
        return false;
    }

    return true;
}

void CameraRESTServer::QuitThread()
{
    if (thread_id_ == 0)
        return;

    shutdown_ = true;

    int ret = pthread_join(thread_id_, nullptr);
    if (ret != 0)
    {
        perror("Thread CameraRESTServer join failed");
    }

    thread_id_ = 0;
    shutdown_ = false;
}

void *CameraRESTServer::ThreadProc(void *arg)
{
    string log_message;

    log_message = "Thread CameraRESTServer start...";
    INFO_LOG(log_message);

    CameraRESTServer *camera_restserver = (CameraRESTServer *) arg;

    camera_restserver->soap_ctx_ = soap_new1(SOAP_C_UTFSTRING);
    camera_restserver->soap_ctx_->accept_timeout = 10; // 10 sec
    camera_restserver->soap_ctx_->send_timeout = 10; // 10 sec
    camera_restserver->soap_ctx_->recv_timeout = 10; // 10 sec

//    if (!soap_valid_socket(
//            soap_bind(camera_restserver->soap_ctx_, camera_restserver->ip_.c_str(), camera_restserver->port_, BACKLOG)))
    if (!soap_valid_socket(soap_bind(camera_restserver->soap_ctx_, nullptr, camera_restserver->port_, BACKLOG)))
    {
        stringstream error_message;
        soap_stream_fault(camera_restserver->soap_ctx_, error_message);
        log_message.clear();
        log_message = "rest failure, errInfo:" + error_message.str();
        WARNING_LOG(log_message);
    }

    while (!camera_restserver->shutdown_)
    {
        THREAD_TYPE tid;
        if (!soap_valid_socket(soap_accept(camera_restserver->soap_ctx_)))
        {
            /*stringstream error_message;
            soap_stream_fault(camera_restserver->soap_ctx_, error_message);
            log_message.clear();
            log_message = "rest failure, errInfo:" + error_message.str();

            RUNNING_LOGGER(log_message);
            WARNING_LOG(log_message);
            break;*/
        }
        else
        {
            THREAD_CREATE(&tid, ServeRequest, (void *) soap_copy(camera_restserver->soap_ctx_));
        }
    } // end while

    soap_destroy(camera_restserver->soap_ctx_);
    soap_end(camera_restserver->soap_ctx_);
    soap_free(camera_restserver->soap_ctx_);

    log_message.clear();
    log_message = "Thread CameraRESTServer exit.";
    INFO_LOG(log_message);

    pthread_exit(nullptr);
}

void *CameraRESTServer::ServeRequest(void *arg)
{
    THREAD_DETACH(THREAD_ID);

    string log_message;

    soap *ctx = (soap *) arg;
    value request(ctx);

    // HTTP keep-alive max number of iterations
    unsigned int k = ctx->max_keep_alive;

    do
    {
        if (ctx->max_keep_alive > 0 && !--k)
        {
            ctx->keep_alive = 0;
        }

        value response(ctx);

        // receive HTTP header (optional) and JSON content
        if (soap_begin_recv(ctx)
            || json_recv(ctx, request)
            || soap_end_recv(ctx))
        {
            soap_send_fault(ctx);
        }
        else
        {
            string camera_ip;
            camera_ip = ctx->host;
            string request_path = ctx->path;
            string camera_id;

            char time_now[64] = {0};

            struct timeval tv;
            struct timezone tz;
            gettimeofday(&tv, &tz);
            struct tm *p = localtime(&tv.tv_sec);

            sprintf(time_now, "%04d-%02d-%02d %02d:%02d:%02d",
                    1900 + p->tm_year,
                    1 + p->tm_mon,
                    p->tm_mday,
                    p->tm_hour,
                    p->tm_min,
                    p->tm_sec);

            if (request_path == "/VIID/System/Register") //视图库注册
            {
                if (request.has("DeviceID"))
                {
                    camera_id = (char *) request["DeviceID"];
                }
                else if (request.has("RegisterObject") && request["RegisterObject"].has("DeviceID"))
                {
                    camera_id = (char *) request["RegisterObject"]["DeviceID"];
                }

                PCameraInfo p_camera = CCfgData::Instance().FindCameraByIP(camera_ip);
                if (p_camera == nullptr)
                {
                    p_camera = new CameraInfo();
                    p_camera->ip = camera_ip;
                    CCfgData::Instance().camera_info_list_.Add(p_camera);
                }

                p_camera->heartbeat = 0;

                log_message.clear();
                log_message = "Camera registration is successful. IP: " + camera_ip + ", DeviceID: " + camera_id + ".";
                INFO_LOG(log_message);

                response["RequestURL"] = request_path;
                response["StatusCode"] = 0;
                response["StatusString"] = "Camera registration is successful.";
                response["Id"] = camera_id;
                response["LocalTime"] = time_now;
            }
            else if (request_path == "/VIID/System/UnRegister") //视图库注销
            {
                if (request.has("DeviceID"))
                {
                    camera_id = (char *) request["DeviceID"];
                }
                else if (request.has("UnRegisterObject") && request["UnRegisterObject"].has("DeviceID"))
                {
                    camera_id = (char *) request["UnRegisterObject"]["DeviceID"];
                }

                PCameraInfo p_camera = CCfgData::Instance().FindCameraByIP(camera_ip, true);
                if (p_camera != nullptr)
                {
                    delete p_camera;
                    p_camera = nullptr;
                }

                log_message.clear();
                log_message = "Camera unregister is successful. IP: " + camera_ip + ", DeviceID: " + camera_id + ".";
                INFO_LOG(log_message);

                response["RequestURL"] = request_path;
                response["StatusCode"] = 0;
                response["StatusString"] = "Camera unregister is successful.";
                response["Id"] = camera_id;
                response["LocalTime"] = time_now;
            }
            else if (request_path == "/VIID/System/Keepalive") //视图库心跳
            {
                if (request.has("DeviceID"))
                {
                    camera_id = (char *) request["DeviceID"];
                }
                else if (request.has("KeepaliveObject") && request["KeepaliveObject"].has("DeviceID"))
                {
                    camera_id = (char *) request["KeepaliveObject"]["DeviceID"];
                }

                PCameraInfo p_camera = CCfgData::Instance().FindCameraByIP(camera_ip);
                if (p_camera != nullptr)
                {
                    p_camera->heartbeat = 0;
                    response["StatusCode"] = 0;
                }
                else
                {
                    response["StatusCode"] = 4;
                }

                response["RequestURL"] = request_path;
                response["StatusString"] = "Camera Keepalive is successful.";
                response["Id"] = camera_id;
                response["LocalTime"] = time_now;
            }
            else if (request_path == "/VIID/Images") //视图库图片
            {
                int image_size = request["ImageListObject"]["Image"].size();
                for (int i = 0; i < image_size; i++)
                {
                    camera_id = (char *) request["ImageListObject"]["Image"][i]["ImageInfo"]["DeviceID"];
                    int image_width = request["ImageListObject"]["Image"][i]["ImageInfo"]["Width"];
                    int image_height = request["ImageListObject"]["Image"][i]["ImageInfo"]["Height"];

                    // get person_id
                    unsigned long person_id = 0;
                    int new_person = 0;
                    if (request["ImageListObject"]["Image"][i]["ImageInfo"].has("TitleNote"))
                    {
                        string title_note = (char *) request["ImageListObject"]["Image"][i]["ImageInfo"]["TitleNote"];
                        unsigned long pos1 = title_note.find('_', 10);
                        unsigned long pos2 = title_note.find('_', pos1 + 1);

                        person_id = atol(title_note.substr(10, pos1 - 10).c_str());
                        new_person = atoi(title_note.substr(pos1 + 1, pos2 - pos1 - 1).c_str());
                    }

                    // get image_base64
                    if (request["ImageListObject"]["Image"][i].has("Data"))
                    {
                        char *image_data = request["ImageListObject"]["Image"][i]["Data"];

                        //auto p_face = new FaceInfo();
                        PFaceInfo p_face = new FaceInfo();

                        p_face->person_id = person_id;
                        p_face->new_person = new_person;

                        memcpy(p_face->image_base64, image_data, strlen(image_data));
                        p_face->image_size = strlen(image_data);

                        p_face->image_width = image_width;
                        p_face->image_height = image_height;

                        p_face->create_time = tv.tv_sec;

                        CCfgData::Instance().face_info_list_.Lock();
                        if (CCfgData::Instance().face_info_list_.GetSize() > 0)
                        {
                            delete CCfgData::Instance().face_info_list_.RemoveHead();
                        }

                        CCfgData::Instance().face_info_list_.Add(p_face);

                        CCfgData::Instance().face_info_list_.Unlock();

                        CCfgData::Instance().sem_face_avail_.Post();
                    }
                    else
                    {
                        int faceObject_size = request["ImageListObject"]["Image"][i]["FaceList"]["FaceObject"].size();
                        for (int j = 0; j < faceObject_size; j++)
                        {
                            int subImageInfoObject_size = request["ImageListObject"]["Image"][i]["FaceList"]["FaceObject"][j]["SubImageList"]["SubImageInfoObject"].size();
                            for (int k = 0; k < subImageInfoObject_size; k++)
                            {
                                if (request["ImageListObject"]["Image"][i]["FaceList"]["FaceObject"][j]["SubImageList"]["SubImageInfoObject"][k].has(
                                        "Data"))
                                {
                                    char *image_data = request["ImageListObject"]["Image"][i]["FaceList"]["FaceObject"][j]["SubImageList"]["SubImageInfoObject"][k]["Data"];

                                    //auto p_face = new FaceInfo();
                                    PFaceInfo p_face = new FaceInfo();

                                    p_face->person_id = person_id;
                                    p_face->new_person = new_person;

                                    memcpy(p_face->image_base64, image_data, strlen(image_data));
                                    p_face->image_size = strlen(image_data);

                                    p_face->image_width = image_width;
                                    p_face->image_height = image_height;

                                    p_face->create_time = tv.tv_sec;

                                    CCfgData::Instance().face_info_list_.Lock();
                                    if (CCfgData::Instance().face_info_list_.GetSize() > 0)
                                    {
                                        delete CCfgData::Instance().face_info_list_.RemoveHead();
                                    }

                                    CCfgData::Instance().face_info_list_.Add(p_face);

                                    CCfgData::Instance().face_info_list_.Unlock();

                                    CCfgData::Instance().sem_face_avail_.Post();
                                }
                            }
                        }
                    }
                }

                response["RequestURL"] = request_path;
                response["StatusCode"] = 0;
                response["StatusString"] = "Images accepted";
                response["Id"] = camera_id;
                response["LocalTime"] = time_now;
            }
            else if (request_path == "/EmployeeService/EmployeeCreate") //比对人员注册
            {
                string photo_id = request["employeeID"];
                string compare_id = request["compareID"];
                string photo_name = request["name"];
                string photo_department = request["department"];
                char *image_data = request["imageBase64"];

                // compare_id
            //    uuid_t uuid;
            //    char c_uuid[36];
            //    uuid_generate(uuid);
            //    uuid_unparse(uuid, c_uuid);
            //    string compare_id = c_uuid;

                log_message.clear();
                log_message = "EmployeeCreate employeeID: " + photo_id + ", name: " + photo_name +
                              ", compareID: " + compare_id + ".";
                INFO_LOG(log_message);

                // extract feature
                bool rtn = CCfgData::Instance().p_pef_->ExtractFeature(photo_id,
                                                                       compare_id,
                                                                       photo_name,
                                                                       photo_department,
                                                                       image_data);
                if (rtn)
                {
                //    pthread_rwlock_wrlock(&CCfgData::Instance().photo_features_lock_);
                //    // fetch feature
                //    CCfgData::Instance().FetchFeature();

                //    CCfgData::Instance().p_pef_->Load_Features_N();

                //    // update ExtractFeatureThread
                //    for (int i = 0; i < CCfgData::Instance().extract_thread_list_.GetSize(); ++i)
                //    {
                //        ExtractFeatureThread *extract_thread = CCfgData::Instance().extract_thread_list_.GetAt(i);
                //        extract_thread->Load_Features_N();
                //    }
                //    pthread_rwlock_unlock(&CCfgData::Instance().photo_features_lock_);

                    CCfgData::Instance().modify_feature_flag_ = true;

                    response["status"] = 0;
                    response["message"] = "成功";
                    response["out"]["employeeInfo"]["compareID"] = compare_id;
                    response["out"]["employeeInfo"]["department"] = photo_department;
                }
                else
                {
                    response["status"] = 1;
                    response["message"] = "失败";
                    response["out"] = "";
                }
            }
            else if (request_path == "/EmployeeService/EmployeeDelete") //比对人员删除
            {
                string compare_id = request["compareID"];

                log_message.clear();
                log_message = "EmployeeDelete compareID: " + compare_id + ".";
                INFO_LOG(log_message);

                // insert into database
                string sql_command;
                char *p_err_msg;
                int rtn_code;

                pthread_rwlock_wrlock(&CCfgData::Instance().photo_features_lock_);

                // UnregistAllBlacklists
                if (compare_id == "UnregistAllBlacklists")
                {
                    // DELETE
                    sql_command = "DELETE FROM person;";

                    // rm *.jpg
                    DIR *dp = nullptr;
                    struct dirent *entry = nullptr;
                    string photo_path_dir = CCfgData::Instance().get_full_path() + "photos/";

                    if ((dp = opendir(photo_path_dir.c_str())) != nullptr)
                    {
                        while ((entry = readdir(dp)) != nullptr)
                        {
                            if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
                            {
                                continue;
                            }

                            string file_name = photo_path_dir + entry->d_name;
                            remove(file_name.c_str());
                        }
                        closedir(dp);
                    }
                    else
                    {
                        log_message.clear();
                        log_message = "cannot open directory:" + photo_path_dir;
                        WARNING_LOG(log_message);
                    }
                }
                else
                {
                    // DELETE
                    sql_command = "DELETE FROM person WHERE compare_id = '" + compare_id + "';";

                    // remove file
                    string photo_path = CCfgData::Instance().get_full_path() + "photos/" + compare_id + ".jpg";
                    remove(photo_path.c_str());
                }

                rtn_code = sqlite3_exec(CCfgData::Instance().sql_, sql_command.c_str(), nullptr, nullptr, &p_err_msg);
                if (SQLITE_OK != rtn_code)
                {
                    log_message.clear();
                    log_message = p_err_msg;
                    WARNING_LOG(log_message);

                    sqlite3_free(p_err_msg);
                }

                // fetch feature
                CCfgData::Instance().FetchFeature();

                CCfgData::Instance().p_pef_->Load_Features_N();

                // update ExtractFeatureThread
                for (int i = 0; i < CCfgData::Instance().extract_thread_list_.GetSize(); ++i)
                {
                    ExtractFeatureThread *extract_thread = CCfgData::Instance().extract_thread_list_.GetAt(i);
                    extract_thread->Load_Features_N();
                }

                CCfgData::Instance().load_feature_flag_ = false;
                pthread_rwlock_unlock(&CCfgData::Instance().photo_features_lock_);

                response["status"] = 0;
                response["message"] = "成功";
            }
            else if (request_path == "/EmployeeService/EmployeeQuery") //比对人员查询
            {
                string photo_id = request["employeeID"];
                int current_index = request["currentIndex"];
                int limit_num = request["limit"];

                log_message.clear();
                log_message = "EmployeeQuery employeeID: " + photo_id +
                              ", currentIndex: " + to_string(current_index) +
                              ", limit: " + to_string(limit_num) + ".";
                INFO_LOG(log_message);

                // select from database
                string sql_command;
                sqlite3_stmt *select_count_stmt;
                sqlite3_stmt *select_stmt;
                int rtn_code;

                if (photo_id.empty())
                {
                    // SELECT
                    sql_command.clear();
                    sql_command = "SELECT COUNT(*) FROM person ORDER BY id ASC;";
                }
                else
                {
                    // SELECT
                    sql_command.clear();
                    sql_command = "SELECT COUNT(*) FROM person WHERE id = '" + photo_id + "' ORDER BY id ASC;";
                }

                // prepare select_stmt
                rtn_code = sqlite3_prepare_v2(CCfgData::Instance().sql_,
                                              sql_command.c_str(),
                                              -1,
                                              &select_count_stmt,
                                              nullptr);
                if (SQLITE_OK != rtn_code)
                {
                    log_message.clear();
                    log_message = sqlite3_errmsg(CCfgData::Instance().sql_);
                    WARNING_LOG(log_message);

                    response["status"] = 1;
                    response["message"] = "失败";
                    response["out"] = "";
                }

                // all
                if (photo_id.empty())
                {
                    // SELECT
                    sql_command.clear();
                    sql_command = "SELECT * FROM person ORDER BY id ASC LIMIT "
                                  + to_string(limit_num) + " OFFSET " + to_string(current_index) + ";";
                }
                else
                {
                    // SELECT
                    sql_command.clear();
                    sql_command = "SELECT * FROM person WHERE id = '" + photo_id + "' ORDER BY id ASC LIMIT "
                                  + to_string(limit_num) + " OFFSET " + to_string(current_index) + ";";
                }

                // prepare select_stmt
                rtn_code = sqlite3_prepare_v2(CCfgData::Instance().sql_,
                                              sql_command.c_str(),
                                              -1,
                                              &select_stmt,
                                              nullptr);
                if (SQLITE_OK != rtn_code)
                {
                    log_message.clear();
                    log_message = sqlite3_errmsg(CCfgData::Instance().sql_);
                    WARNING_LOG(log_message);

                    response["status"] = 1;
                    response["message"] = "失败";
                    response["out"] = "";
                }
                else
                {
                    int index = 0;

                    // select data
                    pthread_rwlock_rdlock(&CCfgData::Instance().photo_features_lock_);
                    while (sqlite3_step(select_count_stmt) == SQLITE_ROW)
                    {
                        response["out"]["Total"] = to_string(sqlite3_column_int(select_count_stmt, 0));
                    }

                    // select data
                    while (sqlite3_step(select_stmt) == SQLITE_ROW)
                    {
                        response["out"]["EmployeeList"][index]["employeeID"] =
                                (char *) sqlite3_column_text(select_stmt, 0);

                        response["out"]["EmployeeList"][index]["compareID"] =
                                (char *) sqlite3_column_text(select_stmt, 1);

                        response["out"]["EmployeeList"][index]["name"] =
                                (char *) sqlite3_column_text(select_stmt, 2);

                        response["out"]["EmployeeList"][index]["department"] =
                                (char *) sqlite3_column_text(select_stmt, 3);

                        index++;
                    }
                    pthread_rwlock_unlock(&CCfgData::Instance().photo_features_lock_);

                    response["status"] = 0;
                    response["message"] = "成功";
                }

                sqlite3_finalize(select_count_stmt);
                sqlite3_finalize(select_stmt);
            }
            else if (request_path == "/EmployeeService/EmployeeFaceImage") //查询比对人员照片
            {
                string compare_id = request["compareID"];

                log_message.clear();
                log_message = "EmployeeFaceImage compareID: " + compare_id + ".";
                INFO_LOG(log_message);

                // select from database
                string sql_command;
                sqlite3_stmt *select_stmt;
                int rtn_code;

                // prepare select_stmt
                sql_command.clear();
                sql_command = "SELECT * FROM person WHERE compare_id = '" + compare_id + "';";
                rtn_code = sqlite3_prepare_v2(CCfgData::Instance().sql_,
                                              sql_command.c_str(),
                                              -1,
                                              &select_stmt,
                                              nullptr);
                if (SQLITE_OK != rtn_code)
                {
                    log_message.clear();
                    log_message = sqlite3_errmsg(CCfgData::Instance().sql_);
                    WARNING_LOG(log_message);

                    response["status"] = 1;
                    response["message"] = "失败";
                    response["out"] = "";
                }
                else
                {
                    // select data
                    pthread_rwlock_rdlock(&CCfgData::Instance().photo_features_lock_);
                    while (sqlite3_step(select_stmt) == SQLITE_ROW)
                    {
                        string photo_name = (char *) sqlite3_column_text(select_stmt, 4);

                        // read photo data
                        string photo_path = CCfgData::Instance().get_full_path() + "photos/" + photo_name;
                        ifstream photo_stream(photo_path, ifstream::in | ifstream::binary);
                        if (photo_stream)
                        {
                            unsigned char *jpeg_data = new unsigned char[BMP_SIZE];
                            int jpeg_size = 0;
                            memset(jpeg_data, 0, BMP_SIZE);

                            unsigned char *base64_data = new unsigned char[BMP_SIZE];
                            memset(base64_data, 0, BMP_SIZE);

                            photo_stream.seekg(0, ios::end);
                            jpeg_size = photo_stream.tellg();
                            photo_stream.seekg(0, ios::beg);
                            photo_stream.read((char *) jpeg_data, jpeg_size);

                            // base64 encode
                            CBase64::Encode(jpeg_data, jpeg_size, base64_data);
                            response["out"]["FaceBase64"] = (char *) base64_data;

                            delete[] jpeg_data;
                            delete[] base64_data;
                        }
                    }
                    pthread_rwlock_unlock(&CCfgData::Instance().photo_features_lock_);

                    response["status"] = 0;
                    response["message"] = "成功";
                }

                sqlite3_finalize(select_stmt);
            }
            else if (request_path == "/face/retrieval") //闸机人员比对
            {
                char *image_data = request["image_content"];

                log_message.clear();
                log_message = "/face/retrieval.";
                INFO_LOG(log_message);

                float score = 0;
                PPhotoInfo p_photo_info = nullptr;
                char *face_data = new char[BMP_SIZE];
                memset(face_data, 0, BMP_SIZE);

                // retrieval
                bool rtn = CCfgData::Instance().p_pef_->RetrieveFace(image_data, score, &p_photo_info, face_data);
                if (rtn)
                {
                    response["rtn"] = 0;
                    response["message"] = "成功";
                    response["similarity"] = score;
                    response["photo_id"] = p_photo_info->id;
                    response["photo_compare_id"] = p_photo_info->uuid;
                    response["photo_name"] = p_photo_info->name;
                    response["photo_department"] = p_photo_info->department;
                    response["photo_img"] = face_data;
                }
                else
                {
                    response["rtn"] = 1;
                    response["message"] = "失败";
                }

                delete[] face_data;
            }
            else if (request_path == "/snapshots/retrieval") //卡照片比对抓拍照片
            {
                string photo_id = request["employeeID"];
                string photo_name = request["name"];
                string photo_department = request["department"];
                char *image_base64 = request["imageBase64"];

                log_message.clear();
                log_message = "/snapshots/retrieval employeeID: " + photo_id + ", name: " + photo_name + ".";
                INFO_LOG(log_message);

                PPhotoInfo photo_info = new PhotoInfo();
                photo_info->id = photo_id;
                photo_info->name = photo_name;
                photo_info->department = photo_department;
                photo_info->create_time = tv;

                photo_info->image_base64 = new char[BMP_SIZE];
                memset(photo_info->image_base64, 0, BMP_SIZE);

                memcpy(photo_info->image_base64, image_base64, strlen(image_base64));
                photo_info->image_size = strlen(image_base64);

                CCfgData::Instance().retrieval_photo_list_.Lock();
                if (CCfgData::Instance().retrieval_photo_list_.GetSize() <
                    CCfgData::Instance().get_face_result_maxsize())
                {
                    CCfgData::Instance().retrieval_photo_list_.Add(photo_info);
                }
                else
                {
                    delete photo_info;
                }
                CCfgData::Instance().retrieval_photo_list_.Unlock();
            }
            else if (request_path == "/")
            {
                if (request.has("fun"))
                {
                    if (0 == strcmp(request["fun"], "getScore")) // 燕京比对
                    {
                        char *image_data = request["img"];

                        log_message.clear();
                        log_message = "getScore.";
                        INFO_LOG(log_message);

                        int score = 0;
                        char *face_data = new char[BMP_SIZE];
                        memset(face_data, 0, BMP_SIZE);

                        // retrieval
                        bool rtn = CCfgData::Instance().p_pef_->RetrieveFace(image_data, score, face_data);

                        response["score"] = score;
                        response["img"] = face_data;

                        // print result
                        log_message.clear();
                        log_message = "getScore result： " + to_string(score)
                                      + ", image size: " + to_string(strlen(face_data)) + ".";

                        INFO_LOG(log_message);

                        delete[] face_data;
                    }
                    else if (0 == strcmp(request["fun"], "getStatus")) // 获得状态
                    {
                        int n = (CCfgData::Instance().camera_info_list_.GetSize() > 0) ? 1 : 0;
                        response["status"] = n;
                    }
                    else if (0 == strcmp(request["fun"], "sendPicture")) // 央视接口
                    {
                        string photo_id = request["id"];
                        string photo_name = request["name"];
                        string photo_department = request["department"];
                        char *image_base64 = request["img"];

                        log_message.clear();
                        log_message = "/snapshots/retrieval employeeID: " + photo_id + ", name: " + photo_name + ".";
                        INFO_LOG(log_message);

                        PPhotoInfo photo_info = new PhotoInfo();
                        photo_info->id = photo_id;
                        photo_info->name = photo_name;
                        photo_info->department = photo_department;
                        photo_info->create_time = tv;

                        photo_info->image_base64 = new char[BMP_SIZE];
                        memset(photo_info->image_base64, 0, BMP_SIZE);

                        memcpy(photo_info->image_base64, image_base64, strlen(image_base64));
                        photo_info->image_size = strlen(image_base64);

                        CCfgData::Instance().retrieval_photo_list_.Lock();
                        if (CCfgData::Instance().retrieval_photo_list_.GetSize() <
                            CCfgData::Instance().get_face_result_maxsize())
                        {
                            CCfgData::Instance().retrieval_photo_list_.Add(photo_info);
                        }
                        else
                        {
                            delete photo_info;
                        }
                        CCfgData::Instance().retrieval_photo_list_.Unlock();

                        response["status"] = "OK";
                    }
                }
            }

            // http content type
            ctx->http_content = "application/json; charset=utf-8";
            // http content length
            if (soap_begin_count(ctx) ||
                ((ctx->mode & SOAP_IO_LENGTH) && json_send(ctx, response)) ||
                soap_end_count(ctx) ||
                soap_response(ctx, SOAP_FILE) ||
                json_send(ctx, response) ||
                soap_end_send(ctx))
            {
                stringstream error_message;
                soap_stream_fault(ctx, error_message);
                log_message.clear();
                log_message = "rest failure, errInfo:" + error_message.str();
                WARNING_LOG(log_message);
            }
        }
        // close (keep-alive may keep socket open when client supports it)
        soap_closesock(ctx);

    } while (ctx->keep_alive);

    int err = ctx->error;

    // clean up
    soap_destroy(ctx);
    soap_end(ctx);
    soap_free(ctx);

    //pthread_exit((void *) err);
    pthread_exit(nullptr);
}
