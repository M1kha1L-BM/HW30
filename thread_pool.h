#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>

class ThreadPool {
public:
    ThreadPool(size_t num_threads);
    ~ThreadPool();

    template<typename F>
    std::future<void> push_task(F&& f);

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop = false;

    void worker_loop();
};

inline ThreadPool::ThreadPool(size_t num_threads) {
    for (size_t i = 0; i < num_threads; ++i)
        workers.emplace_back([this] { worker_loop(); });
}

inline ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (auto& thread : workers)
        thread.join();
}

inline void ThreadPool::worker_loop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            condition.wait(lock, [this] { return stop || !tasks.empty(); });
            if (stop && tasks.empty()) return;
            task = std::move(tasks.front());
            tasks.pop();
        }
        task();
    }
}

template<typename F>
std::future<void> ThreadPool::push_task(F&& f) {
    auto task_ptr = std::make_shared<std::packaged_task<void()>>(std::forward<F>(f));
    std::future<void> res = task_ptr->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.emplace([task_ptr]() { (*task_ptr)(); });
    }
    condition.notify_one();
    return res;
}