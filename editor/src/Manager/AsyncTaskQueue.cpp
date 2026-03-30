/******************************************************************************/
/*!
\file   AsyncTaskQueue.cpp
\author Team PASSTA
        Chew Bangxin Steven (bangxinsteven.chew\@digipen.edu)

\par    Course : CSD3401 / UXG3400
\date   2025/10/04
\brief  Implementation of the asynchronous task queue system.

Copyright (C) 2025 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
******************************************************************************/
#include "Manager/AsyncTaskQueue.hpp"
#include <sstream>
#include <iomanip>
#include <spdlog/spdlog.h>
#include <algorithm>

namespace editor {

AsyncTaskQueue::AsyncTaskQueue(size_t workerCount)
    : m_workerCount(workerCount)
{
    for (size_t i = 0; i < workerCount; ++i) {
        m_workers.emplace_back(&AsyncTaskQueue::WorkerThread, this);
    }
    spdlog::info("AsyncTaskQueue: Initialized with {} worker threads", workerCount);
}

AsyncTaskQueue::~AsyncTaskQueue()
{
    m_shutdown = true;
    m_taskCV.notify_all();
    
    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    spdlog::info("AsyncTaskQueue: Shutdown complete");
}

TaskID AsyncTaskQueue::GenerateTaskID()
{
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    std::ostringstream oss;
    oss << "task_" << ms << "_" << (++m_taskIdCounter);
    return oss.str();
}

TaskID AsyncTaskQueue::SubmitTask(
    std::string const& name,
    TaskFunction task,
    TaskCompleteCallback onComplete,
    TaskPriority priority,
    std::string const& description,
    bool canCancel)
{
    std::lock_guard<std::mutex> lock(m_taskMutex);
    
    TaskID taskId = GenerateTaskID();
    
    InternalTask internalTask;
    internalTask.m_info.m_id = taskId;
    internalTask.m_info.m_name = name;
    internalTask.m_info.m_description = description;
    internalTask.m_info.m_priority = priority;
    internalTask.m_info.m_canCancel = canCancel;
    internalTask.m_info.m_state = TaskState::Pending;
    internalTask.m_info.m_startTime = std::chrono::steady_clock::now();
    internalTask.m_function = std::move(task);
    internalTask.m_onComplete = std::move(onComplete);
    
    m_pendingTasks.emplace(taskId, std::move(internalTask));
    
    int priorityValue = static_cast<int>(priority);
    m_taskQueue.emplace(priorityValue, taskId);
    
    m_taskCV.notify_one();
    
    spdlog::debug("AsyncTaskQueue: Submitted task '{}' with ID {}", name, taskId);
    return taskId;
}

void AsyncTaskQueue::WorkerThread()
{
    while (!m_shutdown) {
        TaskID taskId;
        InternalTask task;
        
        {
            std::unique_lock<std::mutex> lock(m_taskMutex);
            m_taskCV.wait(lock, [this] {
                return m_shutdown || !m_taskQueue.empty();
            });
            
            if (m_shutdown) break;
            
            if (m_taskQueue.empty()) continue;
            
            while (!m_taskQueue.empty() && m_paused) {
                lock.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                lock.lock();
                if (m_shutdown) return;
            }
            
            if (m_taskQueue.empty()) continue;
            
            taskId = m_taskQueue.top().second;
            m_taskQueue.pop();
            
            auto it = m_pendingTasks.find(taskId);
            if (it == m_pendingTasks.end()) continue;
            
            task = std::move(it->second);
            m_pendingTasks.erase(it);
        }
        
        task.m_info.m_state = TaskState::Running;
        task.m_info.m_startTime = std::chrono::steady_clock::now();
        
        {
            std::lock_guard<std::mutex> lock(m_taskMutex);
            m_runningTasks.emplace(taskId, std::move(task));
        }
        
        // Get pointers to the running task components
        TaskInfo* runningInfo = nullptr;
        std::atomic<bool>* cancelFlag = nullptr;
        TaskFunction* funcPtr = nullptr;
        TaskCompleteCallback* onCompletePtr = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_taskMutex);
            auto it = m_runningTasks.find(taskId);
            if (it != m_runningTasks.end()) {
                runningInfo = &(it->second.m_info);
                cancelFlag = &(it->second.m_cancelRequested);
                funcPtr = &(it->second.m_function);
                onCompletePtr = &(it->second.m_onComplete);
            }
        }
        
        if (!runningInfo || !cancelFlag || !funcPtr) continue;
        
        try {
            (*funcPtr)(*runningInfo, *cancelFlag);
            
            if (cancelFlag->load()) {
                runningInfo->m_state = TaskState::Cancelled;
            } else {
                runningInfo->m_state = TaskState::Completed;
                runningInfo->m_progress = 1.0f;
            }
        } catch (const std::exception& e) {
            runningInfo->m_state = TaskState::Failed;
            runningInfo->m_statusMessage = e.what();
            spdlog::error("AsyncTaskQueue: Task '{}' failed: {}", runningInfo->m_name, e.what());
        } catch (...) {
            runningInfo->m_state = TaskState::Failed;
            runningInfo->m_statusMessage = "Unknown error";
            spdlog::error("AsyncTaskQueue: Task '{}' failed with unknown error", runningInfo->m_name);
        }
        
        runningInfo->m_endTime = std::chrono::steady_clock::now();
        
        if (onCompletePtr && *onCompletePtr) {
            try {
                (*onCompletePtr)(*runningInfo);
            } catch (const std::exception& e) {
                spdlog::error("AsyncTaskQueue: Completion callback for '{}' failed: {}", 
                    runningInfo->m_name, e.what());
            }
        }
        
        // Save task info before erasing
        TaskInfo completedInfo = *runningInfo;
        
        {
            std::lock_guard<std::mutex> lock(m_taskMutex);
            m_runningTasks.erase(taskId);
        }
        
        {
            std::lock_guard<std::mutex> lock(m_completedMutex);
            m_completedTasks.push_back(completedInfo);
            while (m_completedTasks.size() > m_maxCompletedTasks) {
                m_completedTasks.erase(m_completedTasks.begin());
            }
        }
        
        m_completionCV.notify_all();
    }
}

void AsyncTaskQueue::CancelTask(TaskID const& taskId)
{
    std::lock_guard<std::mutex> lock(m_taskMutex);
    
    auto runningIt = m_runningTasks.find(taskId);
    if (runningIt != m_runningTasks.end()) {
        if (runningIt->second.m_info.m_canCancel) {
            runningIt->second.m_cancelRequested = true;
            spdlog::info("AsyncTaskQueue: Cancellation requested for task '{}'", 
                runningIt->second.m_info.m_name);
        }
        return;
    }
    
    auto pendingIt = m_pendingTasks.find(taskId);
    if (pendingIt != m_pendingTasks.end()) {
        pendingIt->second.m_info.m_state = TaskState::Cancelled;
        pendingIt->second.m_info.m_endTime = std::chrono::steady_clock::now();
        
        {
            std::lock_guard<std::mutex> completedLock(m_completedMutex);
            m_completedTasks.push_back(pendingIt->second.m_info);
        }
        m_pendingTasks.erase(pendingIt);
        spdlog::info("AsyncTaskQueue: Cancelled pending task '{}'", taskId);
    }
}

void AsyncTaskQueue::CancelAllTasks()
{
    std::lock_guard<std::mutex> lock(m_taskMutex);
    
    for (auto& [id, task] : m_runningTasks) {
        if (task.m_info.m_canCancel) {
            task.m_cancelRequested = true;
        }
    }
    
    while (!m_taskQueue.empty()) {
        m_taskQueue.pop();
    }
    
    {
        std::lock_guard<std::mutex> completedLock(m_completedMutex);
        for (auto& [id, task] : m_pendingTasks) {
            task.m_info.m_state = TaskState::Cancelled;
            task.m_info.m_endTime = std::chrono::steady_clock::now();
            m_completedTasks.push_back(task.m_info);
        }
    }
    m_pendingTasks.clear();
    
    spdlog::info("AsyncTaskQueue: Cancelled all tasks");
}

std::vector<TaskInfo> AsyncTaskQueue::GetActiveTasks() const
{
    std::vector<TaskInfo> tasks;
    
    std::lock_guard<std::mutex> lock(m_taskMutex);
    
    for (auto const& [id, task] : m_pendingTasks) {
        tasks.push_back(task.m_info);
    }
    for (auto const& [id, task] : m_runningTasks) {
        tasks.push_back(task.m_info);
    }
    
    return tasks;
}

std::vector<TaskInfo> AsyncTaskQueue::GetAllTasks() const
{
    std::vector<TaskInfo> tasks = GetActiveTasks();
    
    std::lock_guard<std::mutex> lock(m_completedMutex);
    for (auto const& task : m_completedTasks) {
        tasks.push_back(task);
    }
    
    return tasks;
}

TaskInfo AsyncTaskQueue::GetTaskInfo(TaskID const& taskId) const
{
    std::lock_guard<std::mutex> lock(m_taskMutex);
    
    auto runningIt = m_runningTasks.find(taskId);
    if (runningIt != m_runningTasks.end()) {
        return runningIt->second.m_info;
    }
    
    auto pendingIt = m_pendingTasks.find(taskId);
    if (pendingIt != m_pendingTasks.end()) {
        return pendingIt->second.m_info;
    }
    
    std::lock_guard<std::mutex> completedLock(m_completedMutex);
    for (auto const& task : m_completedTasks) {
        if (task.m_id == taskId) {
            return task;
        }
    }
    
    return TaskInfo{};
}

bool AsyncTaskQueue::HasActiveTasks() const
{
    std::lock_guard<std::mutex> lock(m_taskMutex);
    return !m_pendingTasks.empty() || !m_runningTasks.empty();
}

size_t AsyncTaskQueue::GetActiveTaskCount() const
{
    std::lock_guard<std::mutex> lock(m_taskMutex);
    return m_pendingTasks.size() + m_runningTasks.size();
}

size_t AsyncTaskQueue::GetPendingTaskCount() const
{
    std::lock_guard<std::mutex> lock(m_taskMutex);
    return m_pendingTasks.size();
}

void AsyncTaskQueue::ClearCompletedTasks()
{
    std::lock_guard<std::mutex> lock(m_completedMutex);
    m_completedTasks.clear();
}

void AsyncTaskQueue::SetMaxCompletedTasks(size_t maxCount)
{
    std::lock_guard<std::mutex> lock(m_completedMutex);
    m_maxCompletedTasks = maxCount;
    while (m_completedTasks.size() > m_maxCompletedTasks) {
        m_completedTasks.erase(m_completedTasks.begin());
    }
}

void AsyncTaskQueue::Pause()
{
    m_paused = true;
    spdlog::info("AsyncTaskQueue: Paused");
}

void AsyncTaskQueue::Resume()
{
    m_paused = false;
    spdlog::info("AsyncTaskQueue: Resumed");
}

bool AsyncTaskQueue::IsPaused() const
{
    return m_paused;
}

void AsyncTaskQueue::WaitForAllTasks()
{
    std::unique_lock<std::mutex> lock(m_taskMutex);
    m_completionCV.wait(lock, [this] {
        return m_pendingTasks.empty() && m_runningTasks.empty();
    });
}

TaskProgress::TaskProgress(TaskInfo& info, std::atomic<bool>& cancelFlag)
    : m_info(info), m_cancelFlag(cancelFlag)
{
}

void TaskProgress::SetProgress(float progress)
{
    m_info.m_progress = std::clamp(progress, 0.0f, 1.0f);
    m_info.m_isIndeterminate = false;
}

void TaskProgress::SetProgress(int current, int total)
{
    if (total > 0) {
        SetProgress(static_cast<float>(current) / static_cast<float>(total));
    }
}

void TaskProgress::SetStatus(std::string const& status)
{
    m_info.m_statusMessage = status;
}

void TaskProgress::SetIndeterminate(bool indeterminate)
{
    m_info.m_isIndeterminate = indeterminate;
}

bool TaskProgress::IsCancelled() const
{
    return m_cancelFlag.load();
}

void TaskProgress::CheckCancelled() const
{
    if (m_cancelFlag.load()) {
        throw std::runtime_error("Task cancelled by user");
    }
}

NotificationManager& NotificationManager::Instance()
{
    static NotificationManager instance;
    return instance;
}

void NotificationManager::AddNotification(std::string const& id, std::string const& message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (auto& notif : m_notifications) {
        if (notif.m_id == id) {
            notif.m_message = message;
            notif.m_updatedTime = std::chrono::steady_clock::now();
            return;
        }
    }
    
    Notification notif;
    notif.m_id = id;
    notif.m_message = message;
    notif.m_createdTime = std::chrono::steady_clock::now();
    notif.m_updatedTime = notif.m_createdTime;
    m_notifications.push_back(notif);
}

void NotificationManager::UpdateNotification(std::string const& id, std::string const& message, float progress)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (auto& notif : m_notifications) {
        if (notif.m_id == id) {
            notif.m_message = message;
            notif.m_hasProgress = (progress >= 0.0f);
            notif.m_progress = progress;
            notif.m_updatedTime = std::chrono::steady_clock::now();
            return;
        }
    }
    
    Notification notif;
    notif.m_id = id;
    notif.m_message = message;
    notif.m_hasProgress = (progress >= 0.0f);
    notif.m_progress = progress;
    notif.m_createdTime = std::chrono::steady_clock::now();
    notif.m_updatedTime = notif.m_createdTime;
    m_notifications.push_back(notif);
}

void NotificationManager::RemoveNotification(std::string const& id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_notifications.erase(
        std::remove_if(m_notifications.begin(), m_notifications.end(),
            [&id](Notification const& n) { return n.m_id == id; }),
        m_notifications.end()
    );
}

void NotificationManager::ClearAll()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_notifications.clear();
}

std::vector<NotificationManager::Notification> NotificationManager::GetNotifications() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_notifications;
}

bool NotificationManager::HasNotifications() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return !m_notifications.empty();
}

size_t NotificationManager::GetNotificationCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_notifications.size();
}

}
