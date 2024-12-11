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
        // ��ʼ�����������
        pluginManager_ = std::make_unique<PluginManager>(pluginDir);
        // ��ʼ�������������ʹ��ϵͳ���õ��߳�����
        taskManager_ = std::make_unique<TaskManager>();

        std::cout << "Platform initialized for user: " << userId << std::endl;
    }

    // ��ʾ���ݷ���������
    void runAnalysisWorkflow() {
        try {
            std::cout << "\n=== Starting Analysis Workflow ===\n";

            // 1. �������ݼ�
            std::cout << "\n1. Loading dataset..." << std::endl;
            auto numericDataset = std::dynamic_pointer_cast<NumericDataset>(
                DatasetFactory::createDataset("NUMERIC"));
            
            // ģ������
            std::vector<double> sampleData = {1.2, 3.4, 2.1, 5.6, 4.3, 7.8, 6.5};
            std::ofstream dataFile("sample_data.txt");
            for (const auto& value : sampleData) {
                dataFile << value << "\n";
            }
            dataFile.close();

            numericDataset->load("sample_data.txt");
            std::cout << "Dataset loaded with " << numericDataset->getSize() << " entries" << std::endl;

            // 2. ����Ԥ����
            std::cout << "\n2. Preprocessing data..." << std::endl;
            numericDataset->preprocess();
            std::cout << "Preprocessing completed" << std::endl;

            // 3. ����ͳ�Ʒ�������
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

            // 4. ���������������
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

            // 5. �ȴ�����ȡ���
            std::cout << "\n5. Waiting for tasks to complete..." << std::endl;
            
            // �ȴ�ͳ�Ʒ����������
            while (taskManager_->getTaskStatus(statsTaskId) != TaskStatus::COMPLETED) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            Result statsResult = taskManager_->getTaskResult(statsTaskId);
            
            // �ȴ������������
            while (taskManager_->getTaskStatus(clusterTaskId) != TaskStatus::COMPLETED) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            Result clusterResult = taskManager_->getTaskResult(clusterTaskId);

            // 6. չʾ���
            std::cout << "\n6. Analysis Results:" << std::endl;
            std::cout << "\nStatistical Analysis Results:" << std::endl;
            std::cout << statsResult.getData() << std::endl;
            
            std::cout << "\nClustering Results:" << std::endl;
            std::cout << clusterResult.getData() << std::endl;

            // 7. ���ز��ʾ��
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
        // ������Դ
        taskManager_->shutdown();
        pluginManager_->unloadAllPlugins();
        std::cout << "\nPlatform shutdown completed" << std::endl;
    }
};

// ƽ̨ʹ��ʾ��
int main() {
    std::cout << "=== Data Processing Platform Demo ===\n" << std::endl;

    // ����ƽ̨ʵ��
    PlatformDemo platform("wkaizzen", "/usr/local/plugins");

    // ����ʾ��������
    platform.runAnalysisWorkflow();

    return 0;
}