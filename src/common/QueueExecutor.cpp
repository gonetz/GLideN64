#include "QueueExecutor.h"
void QueueExecutor::start(bool allowSameThreadExec) {
    std::lock_guard lck(initMutex_);
    if (running_)
        return;

    running_ = true;
    allowSameThreadExec_ = allowSameThreadExec;
    tasks_.clear();
    executor_ = std::thread{ &QueueExecutor::loop, this };
}

void QueueExecutor::async(Task task) {
    bool notify;
    {
        std::unique_lock<std::mutex> lck(mutex_);
        notify = !hasPendingTasks();
        tasks_.emplace_back(std::move(task));
    }

    if (notify)
        cv_.notify_one();
}

void QueueExecutor::stop(Task task) {
    stopAsync(std::move(task));
    stopWait();
}

bool QueueExecutor::stopAsync(Task task)
{
    std::lock_guard lck(initMutex_);
    if (!running_)
        return false;

    async([this, aTask(std::move(task))]() {
        running_ = false;
        if (aTask) aTask();
        });

    return true;
}

void QueueExecutor::stopWait()
{
    executor_.join();
}

void QueueExecutor::loop() {
    std::unique_lock<std::mutex> lck(mutex_);
    while (running_) {
        if (!hasPendingTasksForExecutor())
            cv_.wait(lck, [&] { return hasPendingTasksForExecutor(); });

        auto task = std::move(tasks_.front());
        lck.unlock();

        task();

        lck.lock();
        tasks_.pop_front();
    }
}
