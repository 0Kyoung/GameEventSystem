#include "EventDbBridge.h"
#include "DbTypes.h"
#include "AsyncDbJobQueue.h"

namespace GameDb
{
    void EventDbBridge::LoadUserEventsAsync(
        uint64_t uid,
        std::shared_ptr<CUser> user,
        std::function<void()> on_loaded)
    {
        if (user == nullptr)
            return;

        // 워커 스레드 결과를 메인 스레드 콜백까지 들고 가야 하므로
        // 두 람다가 함께 참조할 수 있게 shared_ptr로 감싼다.
        auto rows = std::make_shared<std::vector<DbEventRow>>();

        AsyncDbJobQueue::GetInstance().Enqueue(
            // ── 워커 스레드: DB I/O만 수행, CUser는 절대 건드리지 않는다 ──────
            [uid, rows]()
            {
                *rows = QueryUserEvents(uid);
            },
            // ── 메인 스레드: DB 결과를 게임 상태에 반영 ───────────────────────
            [user, rows, on_loaded]()
            {
                for (const auto& row : *rows)
                {
                    auto info = std::make_shared<UserEventInfo>();
                    info->event_id_         = row.event_id;
                    info->event_step_       = row.event_step;
                    info->sub_event_step_   = row.sub_event_step;
                    info->event_deleted_    = row.event_deleted;
                    info->last_update_time_ = row.last_update_time;
                    user->GetUserEvent().Insert(info);
                }

                if (on_loaded)
                    on_loaded();
            });
    }

} // namespace GameDb
