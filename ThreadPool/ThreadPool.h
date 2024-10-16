#include <iostream>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <future>
using namespace std;

template <typename T>
class ThreadSafeQueue
{
    public:
        void push(T& x)
        {
            std::unique_lock<std::shared_mutex> lk(m);
            q.push(x);
        }
        bool pop(T& el)
        {
            std::unique_lock<std::shared_mutex> lk(m);
            if(q.empty())
            {
                return false;
            }
            el = q.front();
            q.pop();
            return true;
        }
        int size()
        {
            std::shared_lock<std::shared_mutex> lk(m);
            return q.size();
        }
        bool empty()
        {
            std::shared_lock<std::shared_mutex> lk(m);
            return q.empty();
        }

    private:
        std::queue<T> q;
        std::shared_mutex m;

};


class ThreadPool
{
    private:
        ThreadSafeQueue<std::function<void()>> tasks_;
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
                            std::unique_lock<std::mutex> lock(m_);
                            cv_.wait(lock,[this](){return (stop_ || !tasks_.empty());});
                            if(tasks_.empty())
                            {
                                break;
                            }
                            tasks_.pop(task);
                            task();
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
        template<typename F, typename ... Args>
        auto add(F&& f, Args&& ... args) -> std::future<decltype(f(args...))>
        {
            std::function<decltype(f(args...))()> func = [&f, args...]() {
			    return f(args...);
		    };
            auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args ...))()>>(func);
            std::function<void()> wrapper = [task_ptr]() {(*task_ptr)();};
            tasks_.push(wrapper);
            cv_.notify_one();
            return task_ptr->get_future();
        }

};