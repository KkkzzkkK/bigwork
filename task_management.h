// task_management.h
#ifndef TASK_MANAGEMENT_H
#define TASK_MANAGEMENT_H

#include "core_framework.h"
#include "data_management.h"
#include "algorithm_module.h"
#include <chrono>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <sstream>

namespace DataPlatform {

// 任务状态枚举
enum class TaskStatus {
    CREATED,
    QUEUED,
    RUNNING,
    COMPLETED,
    FAILED,
    CANCELLED
};

// 任务优先级枚举
enum class TaskPriority {
    LOW,
    MEDIUM,
    HIGH,
    CRITICAL
};

// 任务配置类
struct TaskConfig {
    std::string taskName;
    TaskPriority priority;
    bool isAsync;
    std::chrono::seconds timeout;
    std::map<std::string, std::string> parameters;

    TaskConfig() 
        : priority(TaskPriority::MEDIUM)
        , isAsync(false)
        , timeout(std::chrono::seconds(300)) {}  // 默认5分钟超时
};

// 任务类
class Task {
private:
    std::string taskId_;
    std::string userId_;
    TaskConfig config_;
    TaskStatus status_;
    std::shared_ptr<IDataset> dataset_;
    std::shared_ptr<IAlgorithm> algorithm_;
    Result result_;
    std::chrono::system_clock::time_point creationTime_;
    std::chrono::system_clock::time_point startTime_;
    std::chrono::system_clock::time_point endTime_;
    std::string errorMessage_;

public:
    Task(const std::string& userId,
         const TaskConfig& config,
         std::shared_ptr<IDataset> dataset,
         std::shared_ptr<IAlgorithm> algorithm)
        : userId_(userId)
        , config_(config)
        , status_(TaskStatus::CREATED)
        , dataset_(dataset)
        , algorithm_(algorithm)
        , creationTime_(std::chrono::system_clock::now())
    {
        // 生成唯一任务ID
        taskId_ = generateTaskId();
    }

    // Getters
    std::string getTaskId() const { return taskId_; }
    std::string getUserId() const { return userId_; }
    TaskStatus getStatus() const { return status_; }
    const Result& getResult() const { return result_; }
    const TaskConfig& getConfig() const { return config_; }
    std::chrono::system_clock::time_point getCreationTime() const { return creationTime_; }
    std::chrono::system_clock::time_point getStartTime() const { return startTime_; }
    std::chrono::system_clock::time_point getEndTime() const { return endTime_; }
    std::string getErrorMessage() const { return errorMessage_; }

    // 执行任务
    bool execute() {
        try {
            status_ = TaskStatus::RUNNING;
            startTime_ = std::chrono::system_clock::now();

            // 设置算法参数
            for (const auto& param : config_.parameters) {
                algorithm_->setParameter(param.first, param.second);
            }

            // 初始化算法
            if (!algorithm_->initialize()) {
                throw PlatformException("Algorithm initialization failed");
            }

            // 执行算法
            result_ = algorithm_->execute(dataset_);

            // 检查结果
            if (result_.getStatus() == Result::Status::SUCCESS) {
                status_ = TaskStatus::COMPLETED;
            } else {
                status_ = TaskStatus::FAILED;
                errorMessage_ = result_.getMessage();
            }

            endTime_ = std::chrono::system_clock::now();
            return true;
        }
        catch (const std::exception& e) {
            status_ = TaskStatus::FAILED;
            errorMessage_ = e.what();
            endTime_ = std::chrono::system_clock::now();
            return false;
        }
    }

    // 取消任务
    bool cancel() {
        if (status_ == TaskStatus::QUEUED || status_ == TaskStatus::RUNNING) {
            status_ = TaskStatus::CANCELLED;
            endTime_ = std::chrono::system_clock::now();
            return true;
        }
        return false;
    }

private:
    static std::string generateTaskId() {
        static std::atomic<uint64_t> counter(0);
        std::stringstream ss;
        ss << "TASK_" << std::hex << std::chrono::system_clock::now().time_since_epoch().count()
           << "_" << counter++;
        return ss.str();
    }
};

// 任务队列比较器
struct TaskComparator {
    bool operator()(const std::shared_ptr<Task>& t1, const std::shared_ptr<Task>& t2) {
        // 优先级高的排在前面
        if (t1->getConfig().priority != t2->getConfig().priority) {
            return t1->getConfig().priority < t2->getConfig().priority;
        }
        // 创建时间早的排在前面
        return t1->getCreationTime() > t2->getCreationTime();
    }
};

// 任务管理器类
class TaskManager {
private:
    std::priority_queue<std::shared_ptr<Task>, 
                       std::vector<std::shared_ptr<Task>>,
                       TaskComparator> taskQueue_;
    std::map<std::string, std::shared_ptr<Task>> taskMap_;
    std::vector<std::thread> workerThreads_;
    std::mutex mutex_;
    std::condition_variable condition_;
    bool isRunning_;
    size_t maxThreads_;
    std::atomic<size_t> activeThreads_;

public:
    TaskManager(size_t maxThreads = std::thread::hardware_concurrency())
        : isRunning_(true)
        , maxThreads_(maxThreads)
        , activeThreads_(0)
    {
        // 启动工作线程
        for (size_t i = 0; i < maxThreads_; ++i) {
            workerThreads_.emplace_back(&TaskManager::workerFunction, this);
        }
    }

    ~TaskManager() {
        shutdown();
    }

    // 提交任务
    std::string submitTask(const std::string& userId,
                          const TaskConfig& config,
                          std::shared_ptr<IDataset> dataset,
                          std::shared_ptr<IAlgorithm> algorithm) {
        auto task = std::make_shared<Task>(userId, config, dataset, algorithm);
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            taskMap_[task->getTaskId()] = task;
            taskQueue_.push(task);
        }
        
        condition_.notify_one();
        return task->getTaskId();
    }

    // 获取任务状态
    TaskStatus getTaskStatus(const std::string& taskId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = taskMap_.find(taskId);
        if (it != taskMap_.end()) {
            return it->second->getStatus();
        }
        throw PlatformException("Task not found: " + taskId);
    }

    // 获取任务结果
    Result getTaskResult(const std::string& taskId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = taskMap_.find(taskId);
        if (it != taskMap_.end()) {
            return it->second->getResult();
        }
        throw PlatformException("Task not found: " + taskId);
    }

    // 取消任务
    bool cancelTask(const std::string& taskId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = taskMap_.find(taskId);
        if (it != taskMap_.end()) {
            return it->second->cancel();
        }
        return false;
    }

    // 关闭任务管理器
    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            isRunning_ = false;
        }
        condition_.notify_all();
        
        for (auto& thread : workerThreads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

private:
    // 工作线程函数
    void workerFunction() {
        while (true) {
            std::shared_ptr<Task> task;
            
            {
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait(lock, [this] {
                    return !isRunning_ || !taskQueue_.empty();
                });

                if (!isRunning_ && taskQueue_.empty()) {
                    return;
                }

                if (!taskQueue_.empty()) {
                    task = taskQueue_.top();
                    taskQueue_.pop();
                }
            }

            if (task) {
                ++activeThreads_;
                task->execute();
                --activeThreads_;
            }
        }
    }
};

} // namespace DataPlatform

#endif // TASK_MANAGEMENT_H