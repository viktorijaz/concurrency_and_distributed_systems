#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

std::mutex m;
std::queue<int> q;
std::condition_variable cv;

void consumer()
{
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, []
            { return !q.empty(); });
    std::cout << "got " << q.front() << "\n";
    q.pop();
}

void producer()
{
    {
        std::lock_guard<std::mutex> lock(m);
        q.push(42);
        std::cout << "pushed 42\n";
    }
    cv.notify_one(); // fixes bug 1
}

int main()
{
    std::cout << "inside main\n";

    std::thread c(consumer);
    std::thread p(producer);

    p.join();
    c.join();
}