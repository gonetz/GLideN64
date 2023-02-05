#pragma once

// This is an elaborate implementation of macOS dispatch queue

#include <condition_variable>
#include <deque>
#include <functional>
#include <variant>
#include <optional>

#include <windows.h>

class SynchImports
{
public:
    using WOA_ADDR = BOOL(WINAPI*)(volatile VOID*, PVOID, SIZE_T, DWORD);
    using WBAS_ADDR = void (WINAPI*)(PVOID);

    SynchImports()
    {
        HMODULE hmodSynch = LoadLibraryA("API-MS-Win-Core-Synch-l1-2-0.dll");
        waitOnAddress_ = (WOA_ADDR)GetProcAddress(hmodSynch, "WaitOnAddress");
        wakeByAddressSingle_ = (WBAS_ADDR)GetProcAddress(hmodSynch, "WakeByAddressSingle");
    }
    ~SynchImports()
    {
        if (hmodSynch_)
            FreeLibrary(hmodSynch_);
    }

    WOA_ADDR waitOnAddress() const 
    { return waitOnAddress_; }
    WBAS_ADDR wakeByAddressSingle() const 
    { return wakeByAddressSingle_; }

private:
    HMODULE hmodSynch_;
    WOA_ADDR waitOnAddress_;
    WBAS_ADDR wakeByAddressSingle_;
};

class QueueExecutor
{
public:
    class LegacyEvent
    {
    public:
        LegacyEvent() = default;

        void notify(uint32_t& notified)
        {
            {
                std::lock_guard<std::mutex> lck(mutex_);
                notified = true;
            }
            cv_.notify_one();
        }
        void wait(uint32_t& notified)
        {
            std::unique_lock<std::mutex> lck(mutex_);
            while (!notified)
            {
                cv_.wait(lck, [&]() { return notified; });
            }
        }

    private:
        std::condition_variable cv_;
        std::mutex mutex_;
    };

    class Event
    {
    public:
        explicit Event(SynchImports& synch)
        : waitOnAddress_(synch.waitOnAddress())
        , wakeByAddressSingle_(synch.wakeByAddressSingle())
        {
            if (!waitOnAddress_ || !wakeByAddressSingle_)
            {
                legacyEvent_.emplace();
            }
        }

        Event& operator=(const Event&) = delete;
        Event(const Event&) = delete;

        void notify()
        { 
            if (notified_)
                return;

            if (!legacyEvent_)
            {
                notified_ = true;
                wakeByAddressSingle_(&notified_);
            }
            else
            {
                legacyEvent_->notify(notified_);
            }
        }
        void wait()
        {
            if (!legacyEvent_)
            {
                uint32_t undesired = 0;
                uint32_t captured = notified_;
                while (captured == undesired)
                {
                    waitOnAddress_(&notified_, &undesired, sizeof(notified_), INFINITE);
                    captured = notified_;
                }
            }
            else
            {
                legacyEvent_->wait(notified_);
            }
        }

    private:
        uint32_t notified_ = false;
        std::optional<LegacyEvent> legacyEvent_;
        SynchImports::WOA_ADDR waitOnAddress_;
        SynchImports::WBAS_ADDR wakeByAddressSingle_;
    };

    using Fn = std::function<void()>;

    QueueExecutor();
    ~QueueExecutor();

    void sync(Fn);
    void async(Fn);
    void asyncOnce(Fn);

    void acceptTasks() { acceptsTasks_ = true; }
    void blockTasks() { acceptsTasks_ = false; }

private:
    class SyncTask
    {
    public:
        explicit SyncTask(SynchImports& imports, Fn fn)
        : event_(imports)
        , task_(fn)
        { }

        void run()
        { 
            task_();
            event_.notify();
        }

        Event& event()
        { return event_; }

    private:
        Event event_;
        Fn task_;
    };

    class AsyncTask
    {
    public:
        AsyncTask(Fn fn) : task_(fn)
        { }

        void run()
        {
            task_();
        }

    private:
        Fn task_;
    };

    using SyncTaskPtr = SyncTask*;
    using AsyncTaskPtr = std::unique_ptr<AsyncTask>;
    using Task = std::variant<SyncTaskPtr, AsyncTaskPtr>;

    SynchImports synch_;

    // Lots of shimes needed for the tasks queue, built around condvar + deque
    std::condition_variable cv_;
    std::mutex mutex_;
    std::deque<Task> tasks_;
    bool running_ = false;
    std::atomic_bool acceptsTasks_ = false;

    // The Executor as it goes
    std::thread executor_;

    void loop();

    void syncInternal(Fn fn);
    void asyncInternal(Fn fn);
};
