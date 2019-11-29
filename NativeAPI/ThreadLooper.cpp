
#include <string>
#include <mutex>
#include <condition_variable>

class ThreadLooper
{
public:
	explicit ThreadLooper()
	{
		
	}

	void wait() {
		if (!notified) {
			std::unique_lock<std::mutex> lock(mutex);
			cvPop.wait(lock);
		}
	}

	void notify() {
		cvPop.notify_one();
		notified = true;
	}

	void reset() {
		notified = false;
	}

private:
	std::condition_variable cvPop;
	std::mutex mutex;
	bool notified;

};