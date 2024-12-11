// algorithm_module.h
#ifndef ALGORITHM_MODULE_H
#define ALGORITHM_MODULE_H

#include "core_framework.h"
#include "data_management.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <unordered_map>

namespace DataPlatform {

// 算法基类
class BaseAlgorithm : public IAlgorithm {
protected:
    std::string name_;
    std::string description_;
    std::map<std::string, std::string> parameters_;
    std::vector<std::string> supportedDataTypes_;

public:
    BaseAlgorithm(const std::string& name, const std::string& description)
        : name_(name), description_(description) {}

    virtual ~BaseAlgorithm() = default;

    // IAlgorithm interface implementation
    bool initialize() override {
        return true;
    }

    void terminate() override {}

    std::string getType() const override {
        return name_;
    }

    std::string getDescription() const override {
        return description_;
    }

    std::vector<std::string> getSupportedDataTypes() const override {
        return supportedDataTypes_;
    }

    bool setParameter(const std::string& key, const std::string& value) override {
        parameters_[key] = value;
        return true;
    }

    std::string getParameter(const std::string& key) const override {
        auto it = parameters_.find(key);
        return (it != parameters_.end()) ? it->second : "";
    }
};

// 数值数据统计分析算法
class StatisticalAnalysis : public BaseAlgorithm {
public:
    StatisticalAnalysis() 
        : BaseAlgorithm("StatisticalAnalysis", "Statistical analysis of numeric data") {
        supportedDataTypes_ = {"NUMERIC"};
    }

    Result execute(const std::shared_ptr<IDataset>& dataset) override {
        Result result;
        
        auto numericDataset = std::dynamic_pointer_cast<NumericDataset>(dataset);
        if (!numericDataset) {
            result.setStatus(Result::Status::FAILURE);
            result.setMessage("Dataset type mismatch");
            return result;
        }

        const auto& data = numericDataset->getData();
        if (data.empty()) {
            result.setStatus(Result::Status::FAILURE);
            result.setMessage("Empty dataset");
            return result;
        }

        // 计算统计指标
        std::ostringstream oss;
        oss << "Statistical Analysis Results:\n";
        oss << "Mean: " << numericDataset->getMean() << "\n";
        oss << "Standard Deviation: " << numericDataset->getStdDev() << "\n";
        oss << "Min: " << numericDataset->getMinValue() << "\n";
        oss << "Max: " << numericDataset->getMaxValue() << "\n";

        // 计算中位数
        std::vector<double> sorted_data = data;
        std::sort(sorted_data.begin(), sorted_data.end());
        double median = (sorted_data.size() % 2 == 0) 
            ? (sorted_data[sorted_data.size()/2 - 1] + sorted_data[sorted_data.size()/2]) / 2
            : sorted_data[sorted_data.size()/2];
        oss << "Median: " << median << "\n";

        result.setStatus(Result::Status::SUCCESS);
        result.setData(oss.str());
        return result;
    }
};

// K-means聚类算法
class KMeansClusteringAlgorithm : public BaseAlgorithm {
private:
    int k_ = 3; // 默认聚类数
    int maxIterations_ = 100;

public:
    KMeansClusteringAlgorithm()
        : BaseAlgorithm("KMeansClustering", "K-means clustering algorithm") {
        supportedDataTypes_ = {"NUMERIC"};
        setParameter("k", "3");
        setParameter("maxIterations", "100");
    }

    bool initialize() override {
        try {
            k_ = std::stoi(getParameter("k"));
            maxIterations_ = std::stoi(getParameter("maxIterations"));
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    Result execute(const std::shared_ptr<IDataset>& dataset) override {
        Result result;
        
        auto numericDataset = std::dynamic_pointer_cast<NumericDataset>(dataset);
        if (!numericDataset) {
            result.setStatus(Result::Status::FAILURE);
            result.setMessage("Dataset type mismatch");
            return result;
        }

        const auto& data = numericDataset->getData();
        if (data.size() < k_) {
            result.setStatus(Result::Status::FAILURE);
            result.setMessage("Not enough data points for k clusters");
            return result;
        }

        // 初始化中心点
        std::vector<double> centroids(k_);
        for (int i = 0; i < k_; ++i) {
            centroids[i] = data[i * data.size() / k_];
        }

        std::vector<int> clusters(data.size());
        bool changed = true;
        int iteration = 0;

        // K-means迭代
        while (changed && iteration < maxIterations_) {
            changed = false;
            
            // 分配点到最近的中心
            for (size_t i = 0; i < data.size(); ++i) {
                int nearest_cluster = 0;
                double min_distance = std::abs(data[i] - centroids[0]);
                
                for (int j = 1; j < k_; ++j) {
                    double distance = std::abs(data[i] - centroids[j]);
                    if (distance < min_distance) {
                        min_distance = distance;
                        nearest_cluster = j;
                    }
                }
                
                if (clusters[i] != nearest_cluster) {
                    clusters[i] = nearest_cluster;
                    changed = true;
                }
            }

            // 更新中心点
            std::vector<double> new_centroids(k_, 0.0);
            std::vector<int> cluster_sizes(k_, 0);
            
            for (size_t i = 0; i < data.size(); ++i) {
                new_centroids[clusters[i]] += data[i];
                cluster_sizes[clusters[i]]++;
            }

            for (int i = 0; i < k_; ++i) {
                if (cluster_sizes[i] > 0) {
                    new_centroids[i] /= cluster_sizes[i];
                }
            }

            centroids = new_centroids;
            iteration++;
        }

        // 生成结果报告
        std::ostringstream oss;
        oss << "K-means Clustering Results:\n";
        oss << "Number of clusters: " << k_ << "\n";
        oss << "Number of iterations: " << iteration << "\n";
        oss << "Final centroids:\n";
        for (int i = 0; i < k_; ++i) {
            oss << "Cluster " << i << ": " << centroids[i] << "\n";
        }

        result.setStatus(Result::Status::SUCCESS);
        result.setData(oss.str());
        return result;
    }
};

// 文本分析算法
class TextAnalysisAlgorithm : public BaseAlgorithm {
public:
    TextAnalysisAlgorithm()
        : BaseAlgorithm("TextAnalysis", "Text analysis algorithm") {
        supportedDataTypes_ = {"TEXT"};
    }

    Result execute(const std::shared_ptr<IDataset>& dataset) override {
        Result result;
        
        auto textDataset = std::dynamic_pointer_cast<TextDataset>(dataset);
        if (!textDataset) {
            result.setStatus(Result::Status::FAILURE);
            result.setMessage("Dataset type mismatch");
            return result;
        }

        const auto& word_frequency = textDataset->getWordFrequency();
        if (word_frequency.empty()) {
            result.setStatus(Result::Status::FAILURE);
            result.setMessage("Empty dataset");
            return result;
        }

        // 计算词频统计
        std::vector<std::pair<std::string, size_t>> sorted_freq(
            word_frequency.begin(), word_frequency.end());
        
        std::sort(sorted_freq.begin(), sorted_freq.end(),
            [](const auto& a, const auto& b) {
                return a.second > b.second;
            });

        std::ostringstream oss;
        oss << "Text Analysis Results:\n";
        oss << "Total unique words: " << word_frequency.size() << "\n";
        oss << "Top 10 most frequent words:\n";
        
        for (size_t i = 0; i < std::min(size_t(10), sorted_freq.size()); ++i) {
            oss << sorted_freq[i].first << ": " 
                << sorted_freq[i].second << " occurrences\n";
        }

        result.setStatus(Result::Status::SUCCESS);
        result.setData(oss.str());
        return result;
    }
};

// 算法工厂类
class AlgorithmFactory {
public:
    static std::shared_ptr<IAlgorithm> createAlgorithm(const std::string& type) {
        if (type == "StatisticalAnalysis") {
            return std::make_shared<StatisticalAnalysis>();
        }
        else if (type == "KMeansClustering") {
            return std::make_shared<KMeansClusteringAlgorithm>();
        }
        else if (type == "TextAnalysis") {
            return std::make_shared<TextAnalysisAlgorithm>();
        }
        throw PlatformException("Unknown algorithm type: " + type);
    }
};

} // namespace DataPlatform

#endif // ALGORITHM_MODULE_H