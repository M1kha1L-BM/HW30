#pragma once
#include <vector>
#include <future>
#include <atomic>
#include <memory>
#include "thread_pool.h"

struct TaskCompletion {
    std::shared_ptr<std::atomic<int>> counter;
    std::shared_ptr<std::promise<void>> promise;

    TaskCompletion(int initial_count)
        : counter(std::make_shared<std::atomic<int>>(initial_count)),
        promise(std::make_shared<std::promise<void>>()) {
    }

    void add_task() {
        counter->fetch_add(1, std::memory_order_relaxed);
    }

    void task_done() {
        if (counter->fetch_sub(1, std::memory_order_acq_rel) == 1) {
            promise->set_value();
        }
    }

    std::future<void> get_future() {
        return promise->get_future();
    }
};

inline void quicksort(std::vector<int>& arr, int left, int right, ThreadPool& pool, TaskCompletion* completion = nullptr) {
    if (left >= right) {
        if (completion) completion->task_done();
        return;
    }

    int pivot = arr[(left + right) / 2];
    int i = left, j = right;
    while (i <= j) {
        while (arr[i] < pivot) ++i;
        while (arr[j] > pivot) --j;
        if (i <= j) std::swap(arr[i++], arr[j--]);
    }

    bool left_big = (j - left) > 1000000;
    bool right_big = (right - i) > 1000000;

    int task_count = left_big + right_big;
    TaskCompletion* sub_completion = nullptr;

    if (task_count > 0) {
        sub_completion = new TaskCompletion(task_count);
    }

    if (left_big) {
        if (sub_completion) sub_completion->add_task();
        pool.push_task([&arr, left, j, &pool, sub_completion]() {
            quicksort(arr, left, j, pool, sub_completion);
            });
    }
    else {
        quicksort(arr, left, j, pool);
    }

    if (right_big) {
        if (sub_completion) sub_completion->add_task();
        pool.push_task([&arr, i, right, &pool, sub_completion]() {
            quicksort(arr, i, right, pool, sub_completion);
            });
    }
    else {
        quicksort(arr, i, right, pool);
    }

    if (sub_completion) {
        sub_completion->get_future().wait();
        delete sub_completion;
    }

    if (completion) completion->task_done();
}
