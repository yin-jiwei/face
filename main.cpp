#include <iostream>
#include <signal.h>
#include "Logger.h"
#include "CameraRESTServer.h"
#include "CfgData.h"

#define TIMER_INTERVAL 100000 // 100ms
#define TIMEOUT_COUNT 3

int g_camera_timer_count = 0;
int g_feature_timer_count = 0;

void TimerFunction(int sig)
{
    string log_message;

    if (SIGALRM == sig)
    {
        // camera offline 30s
        g_camera_timer_count++;
        if (g_camera_timer_count > 299)
        {
            g_camera_timer_count = 0;

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
        }

        // load feature 1s
        g_feature_timer_count++;
        if (g_feature_timer_count > 9)
        {
            g_feature_timer_count = 0;

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
        }

        // snapshots retrieval 100ms
        //        CCfgData::Instance().retrieval_photo_list_.Lock();
        while (CCfgData::Instance().retrieval_photo_list_.GetSize() > 0)
        {
            PPhotoInfo photo_info = CCfgData::Instance().retrieval_photo_list_.GetAt(0);

            struct timeval now_tv;
            gettimeofday(&now_tv, NULL);
            int interval = (now_tv.tv_sec - photo_info->create_time.tv_sec) * 1000 +
                           (now_tv.tv_usec - photo_info->create_time.tv_usec) / 1000;

            if (interval < CCfgData::Instance().get_delay_retrieval_millisecond())
            {
                break;
            }
            else
            {
                photo_info = CCfgData::Instance().retrieval_photo_list_.RemoveHead();
                CCfgData::Instance().p_pef_->RetrieveFace(photo_info);
                delete photo_info;
            }
        }
        //        CCfgData::Instance().retrieval_photo_list_.Unlock();

        //        alarm(TIMER_INTERVAL);
    }
}

int main()
{
    START_LOGGER;

    string log_message;

    INFO_LOG("..............................face start..............................")

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
    //    signal(SIGALRM, TimerFunction);
    ////    alarm(TIMER_INTERVAL);
    //
    //    struct itimerval new_value, old_value;
    //    new_value.it_value.tv_sec = 0;
    //    new_value.it_value.tv_usec = TIMER_INTERVAL;
    //    new_value.it_interval.tv_sec = 0;
    //    new_value.it_interval.tv_usec = TIMER_INTERVAL;
    //    setitimer(ITIMER_REAL, &new_value, &old_value);

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

        //        sleep(TIMER_INTERVAL);

        struct timeval timeout_value;
        timeout_value.tv_sec = 0;
        timeout_value.tv_usec = TIMER_INTERVAL;

        select(0, NULL, NULL, NULL, &timeout_value);
        TimerFunction(SIGALRM);
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

    INFO_LOG("..............................face exit...............................")
    STOP_LOGGER;

    return 0;
}