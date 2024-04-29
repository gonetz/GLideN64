#pragma once

// This is an elaborate implementation of macOS dispatch queue
// It always persists the 'thread' that is executing
// That means that 'sync' is always executing on either created thread or in current thread
// Similarly to macOS impl 'async' always ex

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>

class QueueExecutor {
public:
    using Task = std::function<void()>;
    QueueExecutor() = default;

    void start(bool allowSameThreadExec);
    template<typename Fn>
    void sync(Fn fn)
    {
        bool notify;
        bool executeNow = false;
        std::atomic_flag finished = ATOMIC_FLAG_INIT;

        {
            std::lock_guard lck(mutex_);
            if (allowSameThreadExec_ && !hasPendingTasks())
            {
                executeNow = true;
                busy_ = true;
            }
            else
            {
                // either busy or tasks are present, defer to thread
                executeNow = false;
                notify = !hasPendingTasks();
                tasks_.emplace_back([&finished, afn{ std::move(fn) }]()
                    {
                        afn();
                        finished.test_and_set();
                        finished.notify_one();
                    });
            }
        }

        if (executeNow)
        {
            // we are executing the task on the same thread
            fn();
            {
                // we are done executing stolen task, unbusy and maybe wakeup the thread
                std::lock_guard lck(mutex_);
                busy_ = false;
                notify = !tasks_.empty();
            }

            if (notify)
            {
                cv_.notify_one();
            }
        }
        else
        {
            // just notify the thread and wait for execution to be done similarly to 'async'
            if (notify)
            {
                cv_.notify_one();
            }

            finished.wait(false);
            int a = 0;
        }
    }

    void async(Task);
    void stop(Task = {});

    bool stopAsync(Task = {});
    void stopWait();

private:
    // currently there is a task that is being executed, either in sync thread or in executor thread
    bool hasPendingTasks() const
    {
        return busy_ || !tasks_.empty();
    }

    // called by executor to check if there is anything it needs to execute
    bool hasPendingTasksForExecutor() const
    {
        return !busy_ && !tasks_.empty();
    }

    // OpenGL will be unhappy if different thread will attempt to execute the code related to it
    // This flag will force execution on 'executor' even if current thread can be used instead
    bool allowSameThreadExec_ = false;

    // Lots of shimes needed for the tasks queue, built around condvar + deque
    std::condition_variable cv_;
    std::mutex mutex_;
    std::deque<Task> tasks_;
    bool running_ = false;
    bool busy_ = false;

    // The Executor as it goes
    std::thread executor_;

    // Sync for 'start' and 'stop' to avoid weird edge cases
    std::mutex initMutex_;

    void loop();
};
