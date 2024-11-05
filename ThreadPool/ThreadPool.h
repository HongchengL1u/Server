#ifndef SERVER_THREADPOOL_THREADPOOL_H
#define SERVER_THREADPOOL_THREADPOOL_H
#ifndef SERVER_THREADPOOL_THREADPOOL_H
#define SERVER_THREADPOOL_THREADPOOL_H
#include <iostream>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include "util/alluse.h"
#include "util/alluse.h"
template <typename T>
class ThreadSafeDeque // 不怎么好用
{
    public:
        void push_back(T& x)
        {
            std::unique_lock<std::shared_mutex> lk(m);
            q.push_back(x);
        }
        bool pop_front(T& el)
        {
            // unique_lock 管理写锁
            std::unique_lock<std::shared_mutex> lk(m);
            if(q.empty())
            {
                return false;
            }
            el = q.front();
            q.pop_front();
            return true;
        }
        int size()
        {
            // shared_lock 管理读锁
            std::shared_lock<std::shared_mutex> lk(m);
            return q.size();
        }
        bool empty()
        {
            std::shared_lock<std::shared_mutex> lk(m);
            return q.empty();
        }

    private:
        std::deque<T> q;
        std::shared_mutex m;
        std::condition_variable cv_;
};


class ThreadPool
{
    private:
        std::deque<std::function<void()>> tasks_;
        std::vector<std::thread> threads_;
        bool stop_ = false;
        std::mutex m_;
        std::condition_variable cv_;
    public:
        ThreadPool(int num)
        {
            for(int i=0;i<num;i++)
            {
                threads_.emplace_back(
                    std::thread([this](){
                        while(true)
                        {
                            std::function<void()> task;
                            {
                                std::unique_lock<std::mutex> lock(m_);
                                cv_.wait(lock,[this](){return (stop_ || !tasks_.empty());});
                                if(tasks_.empty())
                                {
                                    break;
                                }
                                task = tasks_.front();
                                tasks_.pop_front();
                                // if(!tasks_.pop_front(task)) continue;
                            }
                            task();
                            LOG(INFO) << "one task done!";
                            LOG(INFO) << "one task done!";
                        }
                }));
            }
        };
        ~ThreadPool()
        {
            {
                std::lock_guard<std::mutex> lock(m_);
                stop_ = true;
            }
            cv_.notify_all();
            for(auto& t : threads_)
            {
                if(t.joinable()) t.join();
            }

        }
        void addfunc(std::function<void()>& func)
        {
            tasks_.push_back(func);
            cv_.notify_one();
        }
        template<typename F, typename ... Args>
        auto add(F&& f, Args&& ... args) -> std::future<decltype(f(args...))>
        {
            std::function<decltype(f(args...))()> func = [&f, args...]() {
			    return f(args...);
		    };
            auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args ...))()>>(func);
            std::function<void()> wrapper = [task_ptr]() {(*task_ptr)();};
            tasks_.push_back(wrapper);
            cv_.notify_one();
            return task_ptr->get_future();
        }

};


#endif


#endif