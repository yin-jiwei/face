/*
 * CLogger.h
 *
 *  Created on: 2016年1月12日
 *      Author: feibaoding
 */

#ifndef CLOGGER_H_
#define CLOGGER_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fstream>

#include <sys/time.h>
#include <sys/stat.h>
#include<dirent.h>

#include "ThreadSafeArray.h"

using namespace std;


typedef enum _level_
{
    LEVEL_OFF,
    LEVEL_SEVERE,
    LEVEL_WARNING,
    LEVEL_INFO,
    LEVEL_CONFIG,
    LEVEL_FINE,
    LEVEL_FINER,
    LEVEL_FINEST,
} LEVEL;

const static char levelNames[][16] =
        {
                " [OFF]    ",
                " [SEVERE] ",
                " [WARNING]",
                " [INFO]   ",
                " [CONFIG] ",
                " [FINE]   ",
                " [FINER]  ",
                " [FINEST] ",
        };

typedef struct _LOGSTRUCT
{
    LEVEL m_level;
    string m_strTime;
    unsigned long m_dwThreadID;
    string m_strLog;
    string m_strFileTime;

    _LOGSTRUCT()
    {
        m_level = LEVEL_OFF;
        m_strTime = "";
        m_dwThreadID = 0;
        m_strLog = "";
        m_strFileTime = "";
    }

} LOGSTRUCT, *LPLOGSTRUCT;

class CLogger
{
public:
    virtual ~CLogger();

    static CLogger &GetpInstance()
    {
        return _Instance;
    }

    void Log(LEVEL level, string lpszLog);

    bool StartWriteThread();

    void QuitWriteThread();

private:
    CLogger();

    static void *ThreadWriteProc(void *lpParam);

    void CreateLogFile();

    void WriteLog();

    void CleanExpiredLogFile(string strdir);

private:
    static CLogger _Instance;
    CThreadSafeArray<LPLOGSTRUCT> m_paLog;

    string m_strCurFileTime;

    pthread_t m_thread = 0;
    sem_t m_SemLog;
    bool m_bShutdown = false;

    FILE *m_file = NULL;
};

#define RUNNING_LOGGER(message) \
    cout << message << endl;

#define START_LOGGER \
    CLogger::GetpInstance().StartWriteThread();

#define STOP_LOGGER \
    CLogger::GetpInstance().QuitWriteThread();


//message=null failed
#define SEVERE_LOG(message) \
do{ \
    string msg = message;\
    msg.append(",file:");\
    msg.append(__FILE__);\
    msg.append(",line:");\
    msg.append(to_string(__LINE__));\
    msg.append(",func:");\
    msg.append(__FUNCTION__);\
    msg.append(".");\
    CLogger::GetpInstance().Log(LEVEL_SEVERE, msg);\
}while(0)

#define WARNING_LOG(message) \
    CLogger::GetpInstance().Log(LEVEL_WARNING, message);

#define INFO_LOG(message) \
    CLogger::GetpInstance().Log(LEVEL_INFO, message);

#define CONFIG_LOG(message) \
    CLogger::GetpInstance().Log(LEVEL_CONFIG, message);

#define FINE_LOG(message) \
    CLogger::GetpInstance().Log(LEVEL_FINE, message);

#endif /* CLOGGER_H_ */
