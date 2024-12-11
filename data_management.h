// data_management.h
#ifndef DATA_MANAGEMENT_H
#define DATA_MANAGEMENT_H

#include "core_framework.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace DataPlatform {

// 数据类型枚举
enum class DataType {
    NUMERIC,
    TEXT,
    CATEGORICAL,
    DATETIME,
    UNDEFINED
};

// 基础数据集抽象类
class BaseDataset : public IDataset {
protected:
    std::string name_;
    std::string description_;
    DataType type_;
    bool isPreprocessed_;
    std::map<std::string, std::string> metadata_;

public:
    BaseDataset(const std::string& name, DataType type)
        : name_(name), type_(type), isPreprocessed_(false) {}

    virtual ~BaseDataset() = default;

    // IDataset interface implementation
    std::string getType() const override {
        return toString(type_);
    }

    std::string getDescription() const override {
        return description_;
    }

    bool isEmpty() const override = 0;
    void clear() override = 0;

    // Metadata management
    void setMetadata(const std::string& key, const std::string& value) {
        metadata_[key] = value;
    }

    std::string getMetadata(const std::string& key) const {
        auto it = metadata_.find(key);
        return (it != metadata_.end()) ? it->second : "";
    }

protected:
    static std::string toString(DataType type) {
        switch (type) {
            case DataType::NUMERIC: return "NUMERIC";
            case DataType::TEXT: return "TEXT";
            case DataType::CATEGORICAL: return "CATEGORICAL";
            case DataType::DATETIME: return "DATETIME";
            default: return "UNDEFINED";
        }
    }
};

// 数值数据集实现
class NumericDataset : public BaseDataset {
private:
    std::vector<double> data_;
    double min_value_;
    double max_value_;
    double mean_;
    double std_dev_;

public:
    NumericDataset() : BaseDataset("NumericDataset", DataType::NUMERIC) {}

    bool load(const std::string& source) override {
        std::ifstream file(source);
        if (!file.is_open()) {
            throw PlatformException("Failed to open file: " + source);
        }

        data_.clear();
        std::string line;
        while (std::getline(file, line)) {
            try {
                double value = std::stod(line);
                data_.push_back(value);
            } catch (const std::exception& e) {
                // Skip invalid entries
                continue;
            }
        }

        calculateStatistics();
        return !data_.empty();
    }

    bool validate() const override {
        return !data_.empty();
    }

    bool preprocess() override {
        if (data_.empty()) return false;

        // Remove outliers using IQR method
        std::vector<double> sorted_data = data_;
        std::sort(sorted_data.begin(), sorted_data.end());

        size_t n = sorted_data.size();
        double q1 = sorted_data[n / 4];
        double q3 = sorted_data[3 * n / 4];
        double iqr = q3 - q1;
        double lower_bound = q1 - 1.5 * iqr;
        double upper_bound = q3 + 1.5 * iqr;

        data_.erase(
            std::remove_if(data_.begin(), data_.end(),
                [lower_bound, upper_bound](double x) {
                    return x < lower_bound || x > upper_bound;
                }),
            data_.end()
        );

        calculateStatistics();
        isPreprocessed_ = true;
        return true;
    }

    size_t getSize() const override {
        return data_.size();
    }

    bool isEmpty() const override {
        return data_.empty();
    }

    void clear() override {
        data_.clear();
        calculateStatistics();
    }

    // Numeric-specific methods
    const std::vector<double>& getData() const { return data_; }
    double getMinValue() const { return min_value_; }
    double getMaxValue() const { return max_value_; }
    double getMean() const { return mean_; }
    double getStdDev() const { return std_dev_; }

private:
    void calculateStatistics() {
        if (data_.empty()) {
            min_value_ = max_value_ = mean_ = std_dev_ = 0.0;
            return;
        }

        min_value_ = *std::min_element(data_.begin(), data_.end());
        max_value_ = *std::max_element(data_.begin(), data_.end());

        // Calculate mean
        mean_ = std::accumulate(data_.begin(), data_.end(), 0.0) / data_.size();

        // Calculate standard deviation
        double sum_sq = std::accumulate(data_.begin(), data_.end(), 0.0,
            [this](double acc, double val) {
                double diff = val - mean_;
                return acc + diff * diff;
            });
        std_dev_ = std::sqrt(sum_sq / data_.size());

        // Update metadata
        setMetadata("min", std::to_string(min_value_));
        setMetadata("max", std::to_string(max_value_));
        setMetadata("mean", std::to_string(mean_));
        setMetadata("std_dev", std::to_string(std_dev_));
    }
};

// 文本数据集实现
class TextDataset : public BaseDataset {
private:
    std::vector<std::string> data_;
    std::map<std::string, size_t> word_frequency_;

public:
    TextDataset() : BaseDataset("TextDataset", DataType::TEXT) {}

    bool load(const std::string& source) override {
        std::ifstream file(source);
        if (!file.is_open()) {
            throw PlatformException("Failed to open file: " + source);
        }

        data_.clear();
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                data_.push_back(line);
            }
        }

        calculateWordFrequency();
        return !data_.empty();
    }

    bool validate() const override {
        return !data_.empty();
    }

    bool preprocess() override {
        if (data_.empty()) return false;

        // Basic text preprocessing
        for (auto& text : data_) {
            // Convert to lowercase
            std::transform(text.begin(), text.end(), text.begin(), ::tolower);

            // Remove extra spaces
            auto new_end = std::unique(text.begin(), text.end(),
                [](char a, char b) {
                    return std::isspace(a) && std::isspace(b);
                });
            text.erase(new_end, text.end());

            // Trim leading and trailing spaces
            text = text.substr(text.find_first_not_of(" \t\n\r\f\v"));
            text = text.substr(0, text.find_last_not_of(" \t\n\r\f\v") + 1);
        }

        calculateWordFrequency();
        isPreprocessed_ = true;
        return true;
    }

    size_t getSize() const override {
        return data_.size();
    }

    bool isEmpty() const override {
        return data_.empty();
    }

    void clear() override {
        data_.clear();
        word_frequency_.clear();
    }

    // Text-specific methods
    const std::vector<std::string>& getData() const { return data_; }
    const std::map<std::string, size_t>& getWordFrequency() const { 
        return word_frequency_; 
    }

private:
    void calculateWordFrequency() {
        word_frequency_.clear();
        for (const auto& text : data_) {
            std::istringstream iss(text);
            std::string word;
            while (iss >> word) {
                ++word_frequency_[word];
            }
        }

        // Update metadata
        setMetadata("unique_words", std::to_string(word_frequency_.size()));
        setMetadata("total_words", std::to_string(
            std::accumulate(word_frequency_.begin(), word_frequency_.end(), 0,
                [](size_t sum, const auto& pair) {
                    return sum + pair.second;
                })
        ));
    }
};

// 数据集工厂类
class DatasetFactory {
public:
    static std::shared_ptr<IDataset> createDataset(const std::string& type) {
        if (type == "NUMERIC") {
            return std::make_shared<NumericDataset>();
        }
        else if (type == "TEXT") {
            return std::make_shared<TextDataset>();
        }
        throw PlatformException("Unknown dataset type: " + type);
    }
};

} // namespace DataPlatform

#endif // DATA_MANAGEMENT_H