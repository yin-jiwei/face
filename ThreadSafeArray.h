/*
 * CThreadSafeArray.h
 *
 *  Created on: 2016年1月12日
 *      Author: feibaoding
 */

#ifndef CTHREADSAFEARRAY_H_
#define CTHREADSAFEARRAY_H_

#include <pthread.h>
#include <deque>

using namespace std;

template<typename T>
class CThreadSafeArray : public deque<T>
{
public:
    CThreadSafeArray();

    virtual ~CThreadSafeArray();

    void Lock();

    void Unlock();

    unsigned int GetSize();

    void Add(T newElement);

    T GetAt(unsigned int nIndex);

    T RemoveAt(unsigned int nIndex);

    T RemoveHead();

    T RemoveTail();

private:
    pthread_mutex_t m_mutex;
};

template<typename T>
CThreadSafeArray<T>::CThreadSafeArray() :
        deque<T>()
{
    //create recursive attribute
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);

    //set recursive attribute
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init(&m_mutex, &attr);

    //destroy recursive attribute
    pthread_mutexattr_destroy(&attr);
}

template<typename T>
CThreadSafeArray<T>::~CThreadSafeArray()
{
    pthread_mutex_destroy(&m_mutex);
}

template<typename T>
void CThreadSafeArray<T>::Lock()
{
    pthread_mutex_lock(&m_mutex);
}

template<typename T>
void CThreadSafeArray<T>::Unlock()
{
    pthread_mutex_unlock(&m_mutex);
}

template<typename T>
void CThreadSafeArray<T>::Add(T newElement)
{
    pthread_mutex_lock(&m_mutex);
    this->push_back(newElement);
    pthread_mutex_unlock(&m_mutex);
}

template<typename T>
T CThreadSafeArray<T>::GetAt(unsigned int nIndex)
{
    T ret = NULL;

    pthread_mutex_lock(&m_mutex);
    unsigned int size = GetSize();

    if (size > 0)
    {
        if (nIndex < size)
            ret = this->at(nIndex);
    }
    pthread_mutex_unlock(&m_mutex);

    return ret;
}

template<typename T>
T CThreadSafeArray<T>::RemoveAt(unsigned int nIndex)
{
    T ret = NULL;

    pthread_mutex_lock(&m_mutex);
    unsigned int size = GetSize();

    if (size > 0)
    {
        if (nIndex < size)
        {
            ret = this->at(nIndex);
            this->erase(this->begin() + nIndex);
        }
    }

    pthread_mutex_unlock(&m_mutex);

    return ret;
}

template<typename T>
T CThreadSafeArray<T>::RemoveHead()
{
    T ret = NULL;

    pthread_mutex_lock(&m_mutex);
    unsigned int size = GetSize();

    if (size > 0)
    {
        ret = this->at(0);
        this->pop_front();
    }

    pthread_mutex_unlock(&m_mutex);

    return ret;
}

template<typename T>
T CThreadSafeArray<T>::RemoveTail()
{
    T ret = NULL;

    pthread_mutex_lock(&m_mutex);
    unsigned int size = GetSize();

    if (size > 0)
    {
        ret = this->at(size - 1);
        this->pop_back();
    }

    pthread_mutex_unlock(&m_mutex);

    return ret;
}

template<typename T>
unsigned int CThreadSafeArray<T>::GetSize()
{
    unsigned int ret = 0;

    pthread_mutex_lock(&m_mutex);

    if (this->begin() != this->end())
        ret = this->end() - this->begin();

    pthread_mutex_unlock(&m_mutex);

    return ret;
}

#endif /* CTHREADSAFEARRAY_H_ */
