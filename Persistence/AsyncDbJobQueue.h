#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// 게임 스레드를 막지 않는 비동기 DB 처리의 핵심.
//
// 설계 원칙 (미르4 라이브 서비스에서 겪은 문제에서 출발):
//   - DB 쿼리는 워커 스레드에서만 실행한다 (Execute)
//   - CUser / UserEventContainer 등 게임 상태를 만지는 코드는
//     반드시 메인(게임) 스레드에서만 실행한다 (OnMainThreadDone)
//   - 두 콜백을 한 곳(Enqueue)에서 등록받아, 워커 스레드가 끝낸 작업의
//     "메인 스레드 몫"을 completion_queue_ 에 쌓아 두고
//     메인 스레드가 매 틱 ProcessCompletions() 로 직접 꺼내 실행한다.
//
// 이렇게 분리하지 않으면 워커 스레드가 콜백에서 곧바로 CUser를 건드리게 되고,
// 게임 로직과 DB I/O 스레드가 동시에 같은 객체를 수정하는 레이스 컨디션이
// 생긴다. 이 큐는 그 경계를 코드로 강제하기 위한 장치다.
// ─────────────────────────────────────────────────────────────────────────────
#include <functional>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace GameDb
{
    class AsyncDbJobQueue
    {
    public:
        static AsyncDbJobQueue& GetInstance()
        {
            static AsyncDbJobQueue instance;
            return instance;
        }

        // 서버 시작 시 한 번 호출
        void Start(int worker_count);

        // 서버 종료 시 한 번 호출 (대기 중인 워커를 깨워 정리)
        void Stop();

        // work        : 워커 스레드에서 실행 (블로킹 DB I/O 허용)
        // on_main_done: work 완료 후, 메인 스레드가 ProcessCompletions()를
        //               호출하는 시점에 메인 스레드에서 실행 (게임 상태 반영)
        void Enqueue(std::function<void()> work, std::function<void()> on_main_done = nullptr);

        // 메인(게임) 스레드에서 매 틱 호출.
        // 그 시점까지 완료된 작업들의 on_main_done 콜백을 순서대로 실행한다.
        void ProcessCompletions();

        // 테스트/모니터링용
        size_t PendingJobCount() const;

    private:
        AsyncDbJobQueue() = default;
        void WorkerLoop();

        std::vector<std::thread> workers_;
        std::atomic<bool> running_{ false };

        std::queue<std::function<void()>> job_queue_;
        mutable std::mutex job_mutex_;
        std::condition_variable job_cv_;

        std::queue<std::function<void()>> completion_queue_;
        std::mutex completion_mutex_;
    };

} // namespace GameDb
