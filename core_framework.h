// core_framework.h
#ifndef CORE_FRAMEWORK_H
#define CORE_FRAMEWORK_H

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <stdexcept>
#include <functional>

namespace DataPlatform {

// Forward declarations
class IDataset;
class IAlgorithm;
class Result;

// Exception classes
class PlatformException : public std::runtime_error {
public:
    explicit PlatformException(const std::string& message) 
        : std::runtime_error(message) {}
};

// Result class to store processing results
class Result {
public:
    enum class Status {
        SUCCESS,
        FAILURE,
        PENDING,
        PROCESSING
    };

private:
    Status status_;
    std::string message_;
    std::string data_;
    std::string timestamp_;

public:
    Result() : status_(Status::PENDING) {}

    void setStatus(Status status) { status_ = status; }
    void setMessage(const std::string& message) { message_ = message; }
    void setData(const std::string& data) { data_ = data; }
    void setTimestamp(const std::string& timestamp) { timestamp_ = timestamp; }

    Status getStatus() const { return status_; }
    std::string getMessage() const { return message_; }
    std::string getData() const { return data_; }
    std::string getTimestamp() const { return timestamp_; }
};

// Interface for Dataset
class IDataset {
public:
    virtual ~IDataset() = default;

    // Core methods
    virtual bool load(const std::string& source) = 0;
    virtual bool validate() const = 0;
    virtual bool preprocess() = 0;
    
    // Metadata methods
    virtual std::string getType() const = 0;
    virtual size_t getSize() const = 0;
    virtual std::string getDescription() const = 0;

    // Data access methods
    virtual bool isEmpty() const = 0;
    virtual void clear() = 0;
};

// Interface for Algorithm
class IAlgorithm {
public:
    virtual ~IAlgorithm() = default;

    // Core methods
    virtual bool initialize() = 0;
    virtual Result execute(const std::shared_ptr<IDataset>& dataset) = 0;
    virtual void terminate() = 0;

    // Metadata methods
    virtual std::string getType() const = 0;
    virtual std::string getDescription() const = 0;
    virtual std::vector<std::string> getSupportedDataTypes() const = 0;

    // Parameter management
    virtual bool setParameter(const std::string& key, const std::string& value) = 0;
    virtual std::string getParameter(const std::string& key) const = 0;
};

// Platform Manager class
class PlatformManager {
private:
    std::map<std::string, std::function<std::shared_ptr<IDataset>()>> datasetFactories_;
    std::map<std::string, std::function<std::shared_ptr<IAlgorithm>()>> algorithmFactories_;
    
    // Singleton instance
    static std::unique_ptr<PlatformManager> instance_;
    PlatformManager() = default;

public:
    static PlatformManager& getInstance() {
        if (!instance_) {
            instance_.reset(new PlatformManager());
        }
        return *instance_;
    }

    // Registration methods
    void registerDatasetType(
        const std::string& type,
        std::function<std::shared_ptr<IDataset>()> factory) {
        if (datasetFactories_.find(type) != datasetFactories_.end()) {
            throw PlatformException("Dataset type already registered: " + type);
        }
        datasetFactories_[type] = factory;
    }

    void registerAlgorithmType(
        const std::string& type,
        std::function<std::shared_ptr<IAlgorithm>()> factory) {
        if (algorithmFactories_.find(type) != algorithmFactories_.end()) {
            throw PlatformException("Algorithm type already registered: " + type);
        }
        algorithmFactories_[type] = factory;
    }

    // Factory methods
    std::shared_ptr<IDataset> createDataset(const std::string& type) {
        auto it = datasetFactories_.find(type);
        if (it == datasetFactories_.end()) {
            throw PlatformException("Unknown dataset type: " + type);
        }
        return it->second();
    }

    std::shared_ptr<IAlgorithm> createAlgorithm(const std::string& type) {
        auto it = algorithmFactories_.find(type);
        if (it == algorithmFactories_.end()) {
            throw PlatformException("Unknown algorithm type: " + type);
        }
        return it->second();
    }

    // Utility methods
    std::vector<std::string> getRegisteredDatasetTypes() const {
        std::vector<std::string> types;
        for (const auto& pair : datasetFactories_) {
            types.push_back(pair.first);
        }
        return types;
    }

    std::vector<std::string> getRegisteredAlgorithmTypes() const {
        std::vector<std::string> types;
        for (const auto& pair : algorithmFactories_) {
            types.push_back(pair.first);
        }
        return types;
    }
};

} // namespace DataPlatform

#endif // CORE_FRAMEWORK_H