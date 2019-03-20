//
// Created by yjw on 9/14/18.
//

#ifndef FACE_SEMAPHORE_H
#define FACE_SEMAPHORE_H

#include <mutex>
#include <condition_variable>

//class Semaphore
//{
//public:
//    Semaphore(int value = 0) : value_(value)
//    {
//    }
//
//    void Wait()
//    {
//        std::unique_lock <std::mutex> lock(mutex_);
//        while (value_ < 1)
//        {
//            cv_.wait(lock);
//        }
//        value_--;
//    }
//
//    bool TryWait()
//    {
//        std::unique_lock <std::mutex> lock(mutex_);
//        if (value_ < 1)
//        {
//            return false;
//        }
//
//        value_--;
//        return true;
//    }
//
//    void Post()
//    {
//        {
//            std::unique_lock <std::mutex> lock(mutex_);
//            value_++;
//        }
//
//        cv_.notify_one();
//    }
//
//    void PostAll()
//    {
//        {
//            std::unique_lock <std::mutex> lock(mutex_);
//            value_++;
//        }
//
//        cv_.notify_all();
//    }
//
//private:
//    std::condition_variable cv_;
//    std::mutex mutex_;
//    int value_;
//};

class Semaphore
{
public:
    Semaphore()
    {
    }

    void Wait()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock);
    }

    void Post()
    {
        // 信号阻塞的话，会丢失
        cv_.notify_one();
    }

    void PostAll()
    {
        cv_.notify_all();
    }

private:
    std::condition_variable cv_;
    std::mutex mutex_;
};

#endif //FACE_SEMAPHORE_H
