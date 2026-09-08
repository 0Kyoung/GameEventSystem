#include "AsyncDbJobQueue.h"

namespace GameDb
{
    void AsyncDbJobQueue::Start(int worker_count)
    {
        if (running_.exchange(true))
            return; // 이미 실행 중

        for (int i = 0; i < worker_count; ++i)
            workers_.emplace_back(&AsyncDbJobQueue::WorkerLoop, this);
    }

    void AsyncDbJobQueue::Stop()
    {
        if (!running_.exchange(false))
            return;

        job_cv_.notify_all();
        for (auto& t : workers_)
        {
            if (t.joinable())
                t.join();
        }
        workers_.clear();
    }

    void AsyncDbJobQueue::Enqueue(std::function<void()> work, std::function<void()> on_main_done)
    {
        if (!work)
            return;

        // work 실행과 completion_queue_ 적재를 하나의 워커 작업으로 묶는다.
        // → 상위 코드(EventDbBridge 등)는 "워커에서 할 일 / 메인에서 할 일"만
        //   생각하면 되고, 큐잉 디테일을 몰라도 된다.
        auto wrapped = [this, work = std::move(work), on_main_done = std::move(on_main_done)]() mutable
        {
            work();

            if (on_main_done)
            {
                std::lock_guard<std::mutex> lock(completion_mutex_);
                completion_queue_.push(std::move(on_main_done));
            }
        };

        {
            std::lock_guard<std::mutex> lock(job_mutex_);
            job_queue_.push(std::move(wrapped));
        }
        job_cv_.notify_one();
    }

    void AsyncDbJobQueue::ProcessCompletions()
    {
        // 메인 스레드 전용. 이 시점 이후에야 CUser 등 게임 상태를 안전하게 건드릴 수 있다.
        std::queue<std::function<void()>> ready;
        {
            std::lock_guard<std::mutex> lock(completion_mutex_);
            std::swap(ready, completion_queue_);
        }

        while (!ready.empty())
        {
            auto& fn = ready.front();
            if (fn)
                fn();
            ready.pop();
        }
    }

    size_t AsyncDbJobQueue::PendingJobCount() const
    {
        std::lock_guard<std::mutex> lock(job_mutex_);
        return job_queue_.size();
    }

    void AsyncDbJobQueue::WorkerLoop()
    {
        for (;;)
        {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lock(job_mutex_);
                job_cv_.wait(lock, [this] { return !running_.load() || !job_queue_.empty(); });

                if (!running_.load() && job_queue_.empty())
                    return;

                job = std::move(job_queue_.front());
                job_queue_.pop();
            }

            job(); // 실제 DB I/O는 여기, 워커 스레드에서만 일어난다.
        }
    }

} // namespace GameDb
