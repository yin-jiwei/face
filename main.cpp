#include <iostream>
#include <signal.h>
#include "Logger.h"
#include "CameraRESTServer.h"
#include "CfgData.h"

#define TIMER_INTERVAL 1
#define TIMEOUT_COUNT 90

void TimerFunction(int sig)
{
    string log_message;

    if (SIGALRM == sig)
    {
        // camera
        CCfgData::Instance().camera_info_list_.Lock();
        for (unsigned int i = 0; i < CCfgData::Instance().camera_info_list_.GetSize(); i++)
        {
            PCameraInfo p_camera = CCfgData::Instance().camera_info_list_.GetAt(i);

            p_camera->heartbeat++;
            if (p_camera->heartbeat > TIMEOUT_COUNT)
            {
                log_message.clear();
                log_message = "IPC IP: " + p_camera->ip + " is offline!";
                INFO_LOG(log_message);

                CCfgData::Instance().camera_info_list_.RemoveAt(i--);
                delete p_camera;
            }
        }
        CCfgData::Instance().camera_info_list_.Unlock();

        if (CCfgData::Instance().modify_feature_flag_)
        {
            CCfgData::Instance().load_feature_flag_ = true;
            CCfgData::Instance().modify_feature_flag_ = false;
        }
        else if (CCfgData::Instance().load_feature_flag_)
        {
            pthread_rwlock_wrlock(&CCfgData::Instance().photo_features_lock_);
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
        }

        alarm(TIMER_INTERVAL);
    }
}

int main()
{
    START_LOGGER;

    string log_message;

    CCfgData::Instance().Fetch();

    // photo extract feature
    CCfgData::Instance().p_pef_ = new PhotoExtractFeature();
    CCfgData::Instance().p_pef_->ExtractFeature();

    pthread_rwlock_wrlock(&CCfgData::Instance().photo_features_lock_);
    // fetch feature
    CCfgData::Instance().FetchFeature();

    CCfgData::Instance().p_pef_->Load_Features_N();

    // start extract thread
    int extract_thread_num = CCfgData::Instance().get_extract_thread_num();
    for (int i = 0; i < extract_thread_num; ++i)
    {
        ExtractFeatureThread *extract_thread = new ExtractFeatureThread();
        CCfgData::Instance().extract_thread_list_.Add(extract_thread);
    }

    CCfgData::Instance().load_feature_flag_ = false;
    pthread_rwlock_unlock(&CCfgData::Instance().photo_features_lock_);

    // start camera server
    string local_server_ip = CCfgData::Instance().get_local_server_ip();
    int local_server_port = CCfgData::Instance().get_local_server_port();

    CameraRESTServer camera_server;
    camera_server.StartServer(local_server_ip, local_server_port);

    // timer
    signal(SIGALRM, TimerFunction);
    alarm(TIMER_INTERVAL);

    while (true)
    {
//        char input_char;
//
//        log_message.clear();
//        log_message = "q: 退出!";
//        RUNNING_LOGGER(log_message);
//
//        cin >> input_char;
//        if (input_char == 'q')
//        {
//            break;
//        }

        sleep(TIMER_INTERVAL);
    }

    // stop camera server
    camera_server.StopServer();

    // quit extract thread
    while (CCfgData::Instance().extract_thread_list_.GetSize() > 0)
    {
        delete CCfgData::Instance().extract_thread_list_.RemoveHead();
    }

    delete CCfgData::Instance().p_pef_;
    CCfgData::Instance().p_pef_ = nullptr;

    STOP_LOGGER;

    return 0;
}