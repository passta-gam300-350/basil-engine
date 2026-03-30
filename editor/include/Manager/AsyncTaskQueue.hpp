/******************************************************************************/
/*!
\file   AsyncTaskQueue.hpp
\author Team PASSTA
        Chew Bangxin Steven (bangxinsteven.chew\@digipen.edu)

\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief  Thread-safe asynchronous task queue for non-blocking UI operations.
        Provides a worker thread pool for executing long-running tasks while
        keeping the main UI thread responsive.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
******************************************************************************/
#ifndef EDITOR_ASYNC_TASK_QUEUE_HPP
#define EDITOR_ASYNC_TASK_QUEUE_HPP

#include <string>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include <chrono>
#include <unordered_map>

namespace editor {

enum class TaskState {
    Pending,
    Running,
    Completed,
    Failed,
    Cancelled
};

enum class TaskPriority {
    Low = 0,
    Normal = 1,
    High = 2
};

struct TaskInfo {
    std::string m_id;
    std::string m_name;
    std::string m_description;
    TaskState m_state{TaskState::Pending};
    TaskPriority m_priority{TaskPriority::Normal};
    float m_progress{0.0f};
    std::string m_statusMessage;
    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::steady_clock::time_point m_endTime;
    bool m_isIndeterminate{false};
    bool m_canCancel{false};
    
    TaskInfo() = default;
    TaskInfo(std::string const& id, std::string const& name, std::string const& desc = "")
        : m_id(id), m_name(name), m_description(desc) {}
};

using TaskID = std::string;
using TaskFunction = std::function<void(TaskInfo&, std::atomic<bool>&)>;
using TaskCompleteCallback = std::function<void(TaskInfo const&)>;

class AsyncTaskQueue {
public:
    explicit AsyncTaskQueue(size_t workerCount = 2);
    ~AsyncTaskQueue();
    
    AsyncTaskQueue(AsyncTaskQueue const&) = delete;
    AsyncTaskQueue& operator=(AsyncTaskQueue const&) = delete;
    AsyncTaskQueue(AsyncTaskQueue&&) = delete;
    AsyncTaskQueue& operator=(AsyncTaskQueue&&) = delete;

    TaskID SubmitTask(
        std::string const& name,
        TaskFunction task,
        TaskCompleteCallback onComplete = nullptr,
        TaskPriority priority = TaskPriority::Normal,
        std::string const& description = "",
        bool canCancel = false
    );

    void CancelTask(TaskID const& taskId);
    void CancelAllTasks();
    
    std::vector<TaskInfo> GetActiveTasks() const;
    std::vector<TaskInfo> GetAllTasks() const;
    TaskInfo GetTaskInfo(TaskID const& taskId) const;
    
    bool HasActiveTasks() const;
    size_t GetActiveTaskCount() const;
    size_t GetPendingTaskCount() const;
    
    void ClearCompletedTasks();
    void SetMaxCompletedTasks(size_t maxCount);
    
    void Pause();
    void Resume();
    bool IsPaused() const;
    
    void WaitForAllTasks();

private:
    struct InternalTask {
        TaskInfo m_info;
        TaskFunction m_function;
        TaskCompleteCallback m_onComplete;
        std::atomic<bool> m_cancelRequested{false};
        
        InternalTask() = default;
        InternalTask(const InternalTask&) = delete;
        InternalTask& operator=(const InternalTask&) = delete;
        InternalTask(InternalTask&& other) noexcept
            : m_info(std::move(other.m_info))
            , m_function(std::move(other.m_function))
            , m_onComplete(std::move(other.m_onComplete))
            , m_cancelRequested(other.m_cancelRequested.load()) {}
        InternalTask& operator=(InternalTask&& other) noexcept {
            if (this != &other) {
                m_info = std::move(other.m_info);
                m_function = std::move(other.m_function);
                m_onComplete = std::move(other.m_onComplete);
                m_cancelRequested = other.m_cancelRequested.load();
            }
            return *this;
        }
    };
    
    void WorkerThread();
    TaskID GenerateTaskID();
    
    mutable std::mutex m_taskMutex;
    mutable std::mutex m_completedMutex;
    std::condition_variable m_taskCV;
    std::condition_variable m_completionCV;
    
    std::vector<std::thread> m_workers;
    std::priority_queue<std::pair<int, TaskID>> m_taskQueue;
    std::unordered_map<TaskID, InternalTask> m_pendingTasks;
    std::unordered_map<TaskID, InternalTask> m_runningTasks;
    std::vector<TaskInfo> m_completedTasks;
    
    std::atomic<bool> m_shutdown{false};
    std::atomic<bool> m_paused{false};
    std::atomic<size_t> m_taskIdCounter{0};
    size_t m_maxCompletedTasks{50};
    size_t m_workerCount;
};

class TaskProgress {
public:
    explicit TaskProgress(TaskInfo& info, std::atomic<bool>& cancelFlag);
    
    void SetProgress(float progress);
    void SetProgress(int current, int total);
    void SetStatus(std::string const& status);
    void SetIndeterminate(bool indeterminate);
    bool IsCancelled() const;
    void CheckCancelled() const;

private:
    TaskInfo& m_info;
    std::atomic<bool>& m_cancelFlag;
};

class NotificationManager {
public:
    static NotificationManager& Instance();
    
    void AddNotification(std::string const& id, std::string const& message);
    void UpdateNotification(std::string const& id, std::string const& message, float progress = -1.0f);
    void RemoveNotification(std::string const& id);
    void ClearAll();
    
    struct Notification {
        std::string m_id;
        std::string m_message;
        float m_progress{0.0f};
        bool m_hasProgress{false};
        std::chrono::steady_clock::time_point m_createdTime;
        std::chrono::steady_clock::time_point m_updatedTime;
    };
    
    std::vector<Notification> GetNotifications() const;
    bool HasNotifications() const;
    size_t GetNotificationCount() const;

private:
    NotificationManager() = default;
    
    mutable std::mutex m_mutex;
    std::vector<Notification> m_notifications;
};

}

#endif
