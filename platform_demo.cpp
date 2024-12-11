// platform_demo.cpp
#include "core_framework.h"
#include "data_management.h"
#include "algorithm_module.h"
#include "task_management.h"
#include "extension_interface.h"
#include <iostream>

using namespace DataPlatform;

class PlatformDemo {
private:
    std::unique_ptr<PluginManager> pluginManager_;
    std::unique_ptr<TaskManager> taskManager_;
    std::string userId_;

public:
    PlatformDemo(const std::string& userId, const std::string& pluginDir) 
        : userId_(userId) {
        // 初始化插件管理器
        pluginManager_ = std::make_unique<PluginManager>(pluginDir);
        // 初始化任务管理器（使用系统可用的线程数）
        taskManager_ = std::make_unique<TaskManager>();

        std::cout << "Platform initialized for user: " << userId << std::endl;
    }

    // 演示数据分析工作流
    void runAnalysisWorkflow() {
        try {
            std::cout << "\n=== Starting Analysis Workflow ===\n";

            // 1. 加载数据集
            std::cout << "\n1. Loading dataset..." << std::endl;
            auto numericDataset = std::dynamic_pointer_cast<NumericDataset>(
                DatasetFactory::createDataset("NUMERIC"));
            
            // 模拟数据
            std::vector<double> sampleData = {1.2, 3.4, 2.1, 5.6, 4.3, 7.8, 6.5};
            std::ofstream dataFile("sample_data.txt");
            for (const auto& value : sampleData) {
                dataFile << value << "\n";
            }
            dataFile.close();

            numericDataset->load("sample_data.txt");
            std::cout << "Dataset loaded with " << numericDataset->getSize() << " entries" << std::endl;

            // 2. 数据预处理
            std::cout << "\n2. Preprocessing data..." << std::endl;
            numericDataset->preprocess();
            std::cout << "Preprocessing completed" << std::endl;

            // 3. 创建统计分析任务
            std::cout << "\n3. Creating statistical analysis task..." << std::endl;
            auto statsAlgorithm = AlgorithmFactory::createAlgorithm("StatisticalAnalysis");
            
            TaskConfig statsConfig;
            statsConfig.taskName = "Statistical Analysis";
            statsConfig.priority = TaskPriority::HIGH;
            statsConfig.isAsync = false;

            std::string statsTaskId = taskManager_->submitTask(
                userId_,
                statsConfig,
                numericDataset,
                statsAlgorithm
            );

            // 4. 创建聚类分析任务
            std::cout << "\n4. Creating clustering analysis task..." << std::endl;
            auto clusteringAlgorithm = AlgorithmFactory::createAlgorithm("KMeansClustering");
            
            TaskConfig clusterConfig;
            clusterConfig.taskName = "K-means Clustering";
            clusterConfig.priority = TaskPriority::MEDIUM;
            clusterConfig.isAsync = true;
            clusterConfig.parameters["k"] = "2";

            std::string clusterTaskId = taskManager_->submitTask(
                userId_,
                clusterConfig,
                numericDataset,
                clusteringAlgorithm
            );

            // 5. 等待并获取结果
            std::cout << "\n5. Waiting for tasks to complete..." << std::endl;
            
            // 等待统计分析任务完成
            while (taskManager_->getTaskStatus(statsTaskId) != TaskStatus::COMPLETED) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            Result statsResult = taskManager_->getTaskResult(statsTaskId);
            
            // 等待聚类任务完成
            while (taskManager_->getTaskStatus(clusterTaskId) != TaskStatus::COMPLETED) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            Result clusterResult = taskManager_->getTaskResult(clusterTaskId);

            // 6. 展示结果
            std::cout << "\n6. Analysis Results:" << std::endl;
            std::cout << "\nStatistical Analysis Results:" << std::endl;
            std::cout << statsResult.getData() << std::endl;
            
            std::cout << "\nClustering Results:" << std::endl;
            std::cout << clusterResult.getData() << std::endl;

            // 7. 加载插件示例
            std::cout << "\n7. Loading custom plugin..." << std::endl;
            try {
                pluginManager_->loadPlugin("libcustom_dataset.so");
                auto plugins = pluginManager_->getLoadedPlugins();
                std::cout << "Loaded plugins: " << std::endl;
                for (const auto& plugin : plugins) {
                    std::cout << "- " << plugin << std::endl;
                }
            } catch (const PlatformException& e) {
                std::cout << "Plugin loading skipped: " << e.what() << std::endl;
            }

        } catch (const PlatformException& e) {
            std::cerr << "Platform error: " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    ~PlatformDemo() {
        // 清理资源
        taskManager_->shutdown();
        pluginManager_->unloadAllPlugins();
        std::cout << "\nPlatform shutdown completed" << std::endl;
    }
};

// 平台使用示例
int main() {
    std::cout << "=== Data Processing Platform Demo ===\n" << std::endl;

    // 创建平台实例
    PlatformDemo platform("wkaizzen", "/usr/local/plugins");

    // 运行示例工作流
    platform.runAnalysisWorkflow();

    return 0;
}