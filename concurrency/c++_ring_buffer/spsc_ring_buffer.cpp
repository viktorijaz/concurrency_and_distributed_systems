#include <atomic>
#include <cstdint>
#include <cstdio>
#include <thread>

// Plain struct: copying it is just copying bytes, no hidden allocation
struct MarketTick
{
    uint32_t symbol_id;
    double price;
    uint32_t volume;
};

// Single-producer / single-consumer ring buffer, no locks
class LockFreeMarketQueue
{
private:
    static constexpr size_t CAPACITY = 1024; // fixed size, allocated once; power of two
    MarketTick buffer[CAPACITY];
    std::atomic<size_t> head{0}; // next slot to read  - only the consumer writes it
    std::atomic<size_t> tail{0}; // next slot to write - only the producer writes it

public:
    // Producer only
    bool enqueue(const MarketTick &tick)
    {
        size_t current_tail = tail.load(std::memory_order_relaxed); // own index: relaxed
        size_t next_tail = (current_tail + 1) % CAPACITY;           // wrap around
        if (next_tail == head.load(std::memory_order_acquire))
        {                 // pairs with consumer's release
            return false; // full: caller decides
        }
        buffer[current_tail] = tick;                      // 1. write the data
        tail.store(next_tail, std::memory_order_release); // 2. then publish it
        return true;
    }

    // Consumer only
    bool dequeue(MarketTick &result)
    {
        size_t current_head = head.load(std::memory_order_relaxed); // own index: relaxed
        if (current_head == tail.load(std::memory_order_acquire))
        {                 // pairs with producer's release
            return false; // empty: caller decides
        }
        result = buffer[current_head];                                        // 1. read the data
        head.store((current_head + 1) % CAPACITY, std::memory_order_release); // 2. then free the slot
        return true;
    }
};

LockFreeMarketQueue queue; // one shared object, used by both threads

// Producer: pretends to read ticks from the network card
void network_thread()
{
    for (uint32_t i = 1; i <= 5; ++i)
    {
        MarketTick tick{42, 100.0 + i, i * 10}; // symbol 42, price, volume
        while (!queue.enqueue(tick))
        {
        } // full? spin and retry
        std::printf("network: pushed price %.1f\n", tick.price);
    }
}

// Consumer: the trading strategy
void strategy_thread()
{
    for (int received = 0; received < 5;)
    {
        MarketTick tick;
        if (queue.dequeue(tick))
        { // empty? loop and retry
            std::printf("strategy: got price %.1f, volume %u\n", tick.price, tick.volume);
            ++received;
        }
    }
}

int main()
{
    std::thread producer(network_thread);
    std::thread consumer(strategy_thread);
    producer.join();
    consumer.join();
}
