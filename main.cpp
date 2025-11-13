#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include "thread_pool.h"
#include "quicksort.h"

int main() {
    const int N = 1000000;
    std::vector<int> data(N);
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, N);

    for (int& x : data) x = dist(rng);

    ThreadPool pool(4); // Используем ровно 4 потока

    auto start = std::chrono::high_resolution_clock::now();

    TaskCompletion completion(1);
    quicksort(data, 0, N - 1, pool, &completion);
    completion.get_future().wait();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    std::cout << "Sorted " << N << " elements in " << duration.count() << " seconds.\n";

    return 0;
}