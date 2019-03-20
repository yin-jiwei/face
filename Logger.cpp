/*
 * CLogger.cpp
 *
 *  Created on: 2016年1月12日
 *      Author: fbd
 */

#include "Logger.h"
#include "CfgData.h"
#include <sys/syscall.h>

#define LOG_QUEUE_MAX_SIZE        4096
//#define LOG_FILE_NAME			"/Log/DataSync" //Do not add the extern file name


CLogger CLogger::_Instance;

CLogger::CLogger()
{
    // TODO Auto-generated constructor stub
    /*	char buffer[512];
     if ( NULL != getcwd(buffer, 512)) {
     m_strExeFullPath = buffer;
     }*/
}

CLogger::~CLogger()
{
    // TODO Auto-generated destructor stub
}

void CLogger::Log(LEVEL level, string lpszLog)
{
    if (m_thread == 0)
    {
        return;
    }

    if (level == LEVEL_FINE || level == LEVEL_FINER)
    {
        if (!CCfgData::Instance().get_log_verbose())
        {
            return;
        }
    }

    char strTime[128] = {0};
    char strDate[128] = {0};

    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    struct tm *p = localtime(&tv.tv_sec);
    sprintf(strTime, "%02d:%02d:%02d.%03ld", p->tm_hour, p->tm_min, p->tm_sec, tv.tv_usec / 1000);
    sprintf(strDate, "%04d-%02d-%02d", 1900 + p->tm_year, 1 + p->tm_mon,
            p->tm_mday);

    char strTime1[128] = {0};
    sprintf(strTime1, "%04d-%02d-%02d %02d:%02d:%02d.%03ld ", 1900 + p->tm_year,
            1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec,
            tv.tv_usec / 1000);

    RUNNING_LOGGER(strTime1 + lpszLog);

    LPLOGSTRUCT pLog = new LOGSTRUCT;
    pLog->m_strTime = strTime;
    pLog->m_dwThreadID = syscall(__NR_gettid);//pthread_self();
    pLog->m_strFileTime = strDate;
    pLog->m_level = level;
    pLog->m_strLog = lpszLog;

    if (m_paLog.GetSize() < LOG_QUEUE_MAX_SIZE)
        m_paLog.Add(pLog);
    else
        delete pLog;

    sem_post(&m_SemLog);
}

bool CLogger::StartWriteThread()
{
    if (m_thread > 0)
        return false;

    int res;
    res = sem_init(&m_SemLog, 0, 0);
    if (res != 0)
    {
        perror("Semaphore initiallization failed");
    }

    res = pthread_create(&m_thread, NULL, ThreadWriteProc, this);
    if (res != 0)
    {
        perror("CLogger Thread creation failed");
        return false;
    }

    return true;
}

void CLogger::QuitWriteThread()
{
    if (m_thread == 0)
        return;

    m_bShutdown = true;
    sem_post(&m_SemLog);
    int res = pthread_join(m_thread, NULL);
    if (res != 0)
    {
        perror("Thread CLogger join failed");
    }

    sem_destroy(&m_SemLog);

    if (m_file != NULL)
        fclose(m_file);

    while (m_paLog.GetSize())
        delete m_paLog.RemoveHead();
}

void *CLogger::ThreadWriteProc(void *lpParam)
{
    cout << "Thread Clogger start..." << endl;
    CLogger *p = (CLogger *) lpParam;

    while (!p->m_bShutdown)
    {
        sem_wait(&p->m_SemLog);
        GetpInstance().WriteLog();
    }

    cout << "Thread Clogger exit." << endl;
    pthread_exit(NULL);
}

void CLogger::CreateLogFile()
{
    string strLogdir = CCfgData::Instance().get_full_path() + "log/";
    if (access(strLogdir.c_str(), F_OK) == -1)
        if (mkdir(strLogdir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
            perror("create log dir failure.");

    CleanExpiredLogFile(strLogdir);

    if (m_file != NULL)
    {
        fclose(m_file);
        m_file = NULL;
    }

    /*	string strFileName = m_strExeFullPath + LOG_FILE_NAME + m_strCurFileTime
    		+ ".txt";*/
    string strFileName = strLogdir + CCfgData::Instance().get_program_name() + m_strCurFileTime
                         + ".txt";

    m_file = fopen(strFileName.c_str(), "a");
    if (m_file == NULL)
        perror("File open failed");
}

void CLogger::WriteLog()
{
    LPLOGSTRUCT pLog = NULL;

    while (m_paLog.GetSize() > 0)
    {
        pLog = m_paLog.RemoveHead();
        if (pLog == NULL)
            continue;

        if (pLog->m_strFileTime != m_strCurFileTime)
        {
            m_strCurFileTime = pLog->m_strFileTime;
            CreateLogFile();
        }

        if (m_file != NULL)
        {
            fprintf(m_file, "%s [%04lu]%s:%s\n", pLog->m_strTime.c_str(),
                    pLog->m_dwThreadID, levelNames[pLog->m_level],
                    pLog->m_strLog.c_str());
            fflush(m_file);
        }

        delete pLog;
        pLog = NULL;
    }
}

void CLogger::CleanExpiredLogFile(string strdir)
{
    time_t curTime = time(NULL);
    time_t oldTime = (time_t) difftime(curTime, 30 * 24 * 3600);
    struct tm *p = localtime(&oldTime);

    char cOld[256] = {0};
    sprintf(cOld, "%s%04d-%02d-%02d.txt", CCfgData::Instance().get_program_name().c_str(),
            1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday);

    string strOld(cOld);

    DIR *dp = NULL;
    struct dirent *entry = NULL;

    if ((dp = opendir(strdir.c_str())) == NULL)
    {
        return;
    }

    if (chdir(strdir.c_str()) != 0)
    {
        return;
    }

    while ((entry = readdir(dp)) != NULL)
    {
        string tmpName(entry->d_name);
        size_t dotPos = tmpName.find_last_of(".");
        if (dotPos >= tmpName.length() || tmpName.substr(dotPos, 4).compare(".txt"))
            continue;
        if (tmpName.compare(strOld) < 0)
        {
            string tmp = strdir + "/" + tmpName;
            remove(tmp.c_str());
        }
    }
    closedir(dp);

    //sudo find /home/zd/DataSync_Gate/bin/Debug/Log/ -mtime +30 -name "*.txt" -exec rm -rf {} \;
}

