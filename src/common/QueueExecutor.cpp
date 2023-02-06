#include "QueueExecutor.h"

QueueExecutor::QueueExecutor()
: running_(false)
{
}

void QueueExecutor::syncInternal(Fn fn)
try
{
    SyncTask task{ synch_, std::move(fn) };
    {
        std::unique_lock<std::mutex> lck(mutex_);
        tasks_.emplace_back(&task);
    }

    cv_.notify_one();
    task.event().wait();
}
catch (...)
{
}

void QueueExecutor::sync(Fn fn)
try
{
    if (!acceptsTasks_)
        return;

    return syncInternal(std::move(fn));
}
catch (...)
{
}

void QueueExecutor::asyncInternal(Fn fn)
try
{
    auto task = std::make_unique<AsyncTask>(std::move(fn));
    {
        std::unique_lock<std::mutex> lck(mutex_);
        tasks_.emplace_back(std::move(task));
    }

    cv_.notify_one();
}
catch (...)
{
}

void QueueExecutor::async(Fn fn)
try
{
    if (!acceptsTasks_)
        return;

    return asyncInternal(std::move(fn));
}
catch (...)
{
}

void QueueExecutor::asyncOnce(Fn fn)
try
{
    if (!acceptsTasks_)
        return;

    auto task = std::make_unique<AsyncTask>(std::move(fn));
    {
        std::unique_lock<std::mutex> lck(mutex_);
        if (!tasks_.empty())
            return;

        tasks_.emplace_back(std::move(task));
    }

    cv_.notify_one();
}
catch (...)
{
}

QueueExecutor::~QueueExecutor()
{
    stop();
}

void QueueExecutor::loop()
{
    std::unique_lock<std::mutex> lck(mutex_);
    while (running_)
    {
        if (tasks_.empty())
            cv_.wait(lck, [&] { return !tasks_.empty(); });

        auto task = std::move(tasks_.front());
        lck.unlock();

        try
        {
            if (auto sync = std::get_if<SyncTaskPtr>(&task))
            {
                (*sync)->run();
            }
            if (auto async = std::get_if<AsyncTaskPtr>(&task))
            {
                (*async)->run();
            }
        }
        catch (...)
        {
        }

        lck.lock();
        tasks_.pop_front();
    }
}

void QueueExecutor::start()
{
    if (running_)
        return;

    running_ = true;
    executor_ = std::thread{ &QueueExecutor::loop, this };
    acceptsTasks_ = true;
}

void QueueExecutor::stop()
{
    if (!running_)
        return;

    acceptsTasks_ = false;
    asyncInternal([&]()
    {
        running_ = false;
    });
    executor_.join();
}

bool QueueExecutor::disableTasksAndAsync(Fn fn)
{
    if (!acceptsTasks_)
        return false;

    acceptsTasks_ = false;
    asyncInternal(std::move(fn));
    return true;
}
