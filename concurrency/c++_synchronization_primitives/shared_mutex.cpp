#include <cstdio>
#include <mutex>
#include <shared_mutex>
#include <thread>

std::shared_mutex m;
int x = 1;

void reader()
{
    std::shared_lock lock(m); // many readers at once
    std::printf("read x = %d\n", x);
}

void writer()
{
    std::unique_lock lock(m); // one writer, alone
    x = 2;
    std::printf("wrote x = 2\n");
}

int main()
{
    std::thread r1(reader);
    std::thread r2(reader);
    std::thread w(writer);
    r1.join();
    r2.join();
    w.join();
}
