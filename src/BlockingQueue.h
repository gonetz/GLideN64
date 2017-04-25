#ifndef BLOCKINGQUEUE_H
#define BLOCKINGQUEUE_H
#pragma once

#include <mutex>
#include <condition_variable>
#include <deque>

template <typename T, typename  Container = std::deque<T>>
class BlockingQueue
{
private:
	std::mutex d_mutex;
	std::condition_variable d_condition;
	Container d_queue;
public:
	void push(T const& value) {
		{
			std::unique_lock<std::mutex> lock(this->d_mutex);
			d_queue.push_front(value);
		}
		this->d_condition.notify_one();
	}

	void pushBack(T const& value) {
		{
			std::unique_lock<std::mutex> lock(this->d_mutex);
			d_queue.push_back(value);
		}
		this->d_condition.notify_one();
	}

	T pop() {
		std::unique_lock<std::mutex> lock(this->d_mutex);
		this->d_condition.wait(lock, [this]{ return !this->d_queue.empty(); });
		T rc(std::move(this->d_queue.back()));
		this->d_queue.pop_back();
		return rc;
	}
	bool tryPop (T & v, std::chrono::milliseconds dur) {
		std::unique_lock<std::mutex> lock(this->d_mutex);
		if (!this->d_condition.wait_for(lock, dur, [this]{ return !this->d_queue.empty(); })) {
			return false;
		}
		v = std::move(this->d_queue.back());
		this->d_queue.pop_back();
		return true;
	}
	int size() {
		return d_queue.size();
	}
};
#endif //BLOCKINGQUEUE_H
