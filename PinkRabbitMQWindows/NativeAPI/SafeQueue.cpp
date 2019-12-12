// A simple thread-safe queue implementation based on std::list<>::splice
// after a tip in a talk by Sean Parent of Adobe.
//
// Uses standard library threading and synchronization primitives together
// with a std::list<> container for implementing a thread-safe queue.  The
// only special thing is that the queue uses std::list<>::splice in the
// locked region to minimize locked time.
//
// Also implements a maximal size and can thus be used as a buffer between
// the elements in a pipeline with limited buffer bloat.
//
// Author: Manuel Holtgrewe <manuel.holtgrewe@fu-berlin.de>
// License: 3-clause BSD

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <stdexcept>
#include <iostream>
#include <list>
#include <queue> 
#include <vector>

template<class T>
class SafeQueue {

	std::queue<T*> q;
	std::mutex m;

public:

	SafeQueue() {}

	void push(T elem) {

		m.lock();
		if (elem != nullptr) {
			q.push(&elem);
		}
		m.unlock();

	}

	T pop() {

		T* elem = nullptr;

		m.lock();
		if (!q.empty()) {
			elem = q.front();
			q.pop();
		}
		m.unlock();

		return *elem;

	}

	bool empty() {
		return q.empty();
	}

	int size() {
		return q.size();
	}

};