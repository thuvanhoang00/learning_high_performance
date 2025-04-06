#include <thread>
#include <atomic>
#include <cassert>
#include <string>

std::atomic<std::string*> ptr;
int data;

void thread1()
{
    std::string* p = new std::string("Hello");// 1 - (1) “sequenced-before” (3) and (3) “inter-thread happens-before” (5), so (1) “inter-thread happens-before” (5)
    data = 42; // 2
    ptr.store(p, std::memory_order_release); // 3 - (3) “depends-ordered before” (4) and (4) “carries a dependency into” (5), so (3) “inter-thread happens-before” (5)
}

void thread2()
{
    std::string* p2;
    while(!(p2 = ptr.load(std::memory_order_consume))); // 4 - (4) “carries a dependency into” (5)
    assert(*p2 == "Hello"); // 5 - (1) “happens-before” (5)
    assert(data == 42); // 6 - But operation (6) does not depend on (4), so there is no inter-thread happens-before relationship between (3) and (6)
}

int main()
{
    std::thread t1(thread1);
    std::thread t2(thread2);
    t1.join();
    t2.join();

    return 0;
}