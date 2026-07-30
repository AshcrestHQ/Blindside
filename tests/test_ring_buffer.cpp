#include "blindside/ring_buffer.hpp"
#include <cassert>
#include <iostream>
#include <thread>

void test_capacity_and_overflow() {
    blindside::RingBuffer<int, 3> buffer;
    assert(buffer.size() == 0);

    buffer.push(10);
    buffer.push(20);
    buffer.push(30);
    assert(buffer.size() == 3);

    // Push 4th element (overflows, drops oldest element '10')
    buffer.push(40);
    assert(buffer.size() == 3);

    auto item1 = buffer.try_pop();
    assert(item1.has_value() && item1.value() == 20);
    (void)item1;

    auto item2 = buffer.try_pop();
    assert(item2.has_value() && item2.value() == 30);
    (void)item2;

    auto item3 = buffer.try_pop();
    assert(item3.has_value() && item3.value() == 40);
    (void)item3;

    assert(buffer.size() == 0);
    std::cout << "[TEST PASSED] RingBuffer capacity and overflow test." << std::endl;
}

void test_concurrent_push_pop() {
    blindside::RingBuffer<int, 64> buffer;
    constexpr int total_items = 1000;

    std::thread producer([&]() {
        for (int i = 0; i < total_items; ++i) {
            buffer.push(i);
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });

    int popped_count = 0;
    std::thread consumer([&]() {
        while (popped_count < total_items) {
            auto opt = buffer.wait_pop_for(std::chrono::milliseconds(50));
            if (opt.has_value()) {
                popped_count++;
            }
        }
    });

    producer.join();
    consumer.join();

    assert(popped_count == total_items);
    std::cout << "[TEST PASSED] RingBuffer concurrent producer-consumer test." << std::endl;
}

int main() {
    test_capacity_and_overflow();
    test_concurrent_push_pop();
    std::cout << "All RingBuffer unit tests passed successfully!\n";
    return 0;
}
