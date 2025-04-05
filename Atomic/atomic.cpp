#include <atomic>
#include <thread>
#include <iostream>
#define MODIFICATION_ORDER

#ifdef MODIFICATION_ORDER
/*----Code below is used to see "Modification Order" on atomic variable----*/
/*----Ex: 
If add_odd store 9 before add_even store 8
Then all reading threads will see that "modification" on data
Means read 9 before read 8
----*/

std::atomic<int> data{0};
void add_odd(){
    for(int i=1; i<10; i+=2)
        data.store(i, std::memory_order_relaxed);
}

void add_even(){
    for(int i=0; i<10; i+=2)
        data.store(i, std::memory_order_relaxed);
}


int main()
{
    std::thread t1(add_odd);
    std::thread t2(add_even);
    t1.join();
    t2.join();

    std::cout << "Data: " << data << std::endl;

    return 0;
}
/*----END----*/
#endif // MODIFICATION_ORDER
