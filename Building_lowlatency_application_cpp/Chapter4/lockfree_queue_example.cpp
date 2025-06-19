#include "thread_utils.h"
#include "lockfree_queue.h"

struct MyStruct{
    int d_[3];
};

using namespace thu;

auto consumeFunction(LFQueue<MyStruct>* lfq){
    std::this_thread::sleep_for(std::chrono::seconds(5));
    while(lfq->size()){
        const auto d = lfq->getNextToRead();
        lfq->updateReadIndex();
        std::cout << "consumeFunction read elem: " << d->d_[0]
                  << ", " << d->d_[1] << ", " << d->d_[2] << " lfq-size: " << lfq->size() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "consumeFunction exiting." << std::endl;
}

int main(){

    LFQueue<MyStruct> lfq(20);
    auto ct = createAndStartThread(-1, "", consumeFunction, &lfq);
    for(auto i=0; i<50; ++i){
        const MyStruct d{i, i*10, i*100};
        *(lfq.getNextToWriteTo()) = d;
        lfq.updateWriteIndex();

        std::cout << "main constructed elem: " << d.d_[0] << ", " << d.d_[1] << ", " << d.d_[2] << " lfq-size: " << lfq.size() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    ct->join();
    std::cout << "main exiting." << std::endl;

    return 0;
}