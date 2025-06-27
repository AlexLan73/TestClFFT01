#pragma once
#include <mutex>

class manual_reset_event
{
public:
    explicit manual_reset_event(bool signaled = false)
        : signaled_(signaled) {
    }

    void set()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        signaled_ = true;
        cv_.notify_all();
    }

    void reset()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        signaled_ = false;
    }

    // Аналог WaitOne(timeout)
    bool wait_one(const int milliseconds)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!signaled_) {
            return cv_.wait_for(lock, std::chrono::milliseconds(milliseconds), [this] { return signaled_; });
        }
        return true;
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    bool signaled_;
};
