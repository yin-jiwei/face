//
// Created by yjw on 8/9/18.
//

#ifndef FACE_CAMERARESTSERVER_H
#define FACE_CAMERARESTSERVER_H

#include <string>
#include <pthread.h>
#include "json.h"

class CameraRESTServer
{
public:
    CameraRESTServer();

    virtual ~CameraRESTServer();

    void StartServer(std::string &ip, int port);

    void StopServer();

private:
    bool StartThread();

    void QuitThread();

    static void *ThreadProc(void *arg);

    static void *ServeRequest(void *arg);

private:
    pthread_t thread_id_;
    bool shutdown_;

    std::string ip_;
    int port_;

    soap *soap_ctx_;
};


#endif //FACE_CAMERARESTSERVER_H
