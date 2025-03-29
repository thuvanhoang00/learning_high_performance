#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <condition_variable>
#define N 100
std::condition_variable p_cv;
std::condition_variable c_cv;
std::mutex mtx, producedAllMtx;
bool producedAll{false};
std::queue<int> q;

void produce()
{	
	// std::unique_lock<std::mutex> plk(mtx); // the problem here
	for(int idx=0; idx<N; idx++)
	{
		std::unique_lock<std::mutex> plk(mtx);
		p_cv.wait(plk, [&](){return q.empty();});
		q.push(idx);
		std::cout << "Produce: " << idx << std::endl;
		c_cv.notify_all();
		// plk.unlock(); // Why crash here? if move plk insde for loop then it is ok
		plk.unlock();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	{
		std::lock_guard<std::mutex> guard(producedAllMtx);
		producedAll = true;
	}
}

void consume()
{
	
	while(true){
		std::unique_lock<std::mutex> lk(mtx);
		c_cv.wait(lk, [&](){return !q.empty() || producedAll;});
		int val = q.front();
		q.pop();
		std::cout << "Consume: " << val << std::endl;
		// lk.unlock(); // Why crash here?
		lk.unlock();
		p_cv.notify_all();
		
		if(producedAll) break;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

int main()
{
	std::thread t1(produce);
	std::thread t2(consume);
	t1.join();
	t2.join();
	return 0;
}
