#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

std::mutex m;
std::condition_variable cv;
std::queue<int> q;

void producer()
{
    {
        std::lock_guard<std::mutex> lock(m);
        q.push(42);
        std::cout << "produced 42\n";
    }
    cv.notify_one();
}

void consumer()
{
    std::unique_lock<std::mutex> lock(m);
    std::cout << "consumer: waiting...\n";
    cv.wait(lock, []
            { return !q.empty(); });
    std::cout << "got " << q.front() << "\n";
    q.pop();
}

int main()
{
    std::thread p(producer);
    std::thread c(consumer);

    p.join();
    c.join();
}
