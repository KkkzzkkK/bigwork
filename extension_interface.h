// extension_interface.h
#ifndef EXTENSION_INTERFACE_H
#define EXTENSION_INTERFACE_H

#include "core_framework.h"
#include "data_management.h"
#include "algorithm_module.h"
#include <memory>
#include <map>
#include <functional>
#include <dlfcn.h>

namespace DataPlatform {

// ��չ����ӿ�
class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getDescription() const = 0;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
};

// ���ݼ�����ӿ�
class IDatasetPlugin : public IPlugin {
public:
    virtual std::shared_ptr<IDataset> createDataset() = 0;
    virtual std::vector<std::string> getSupportedFormats() const = 0;
};

// �㷨����ӿ�
class IAlgorithmPlugin : public IPlugin {
public:
    virtual std::shared_ptr<IAlgorithm> createAlgorithm() = 0;
    virtual std::vector<std::string> getSupportedDataTypes() const = 0;
};

// �����������
class PluginManager {
private:
    struct PluginInfo {
        void* handle;
        std::shared_ptr<IPlugin> plugin;
        std::string path;
        bool isLoaded;
    };

    std::map<std::string, PluginInfo> plugins_;
    std::string pluginDirectory_;
    std::mutex mutex_;

public:
    explicit PluginManager(const std::string& pluginDir)
        : pluginDirectory_(pluginDir) 
    {}

    ~PluginManager() {
        unloadAllPlugins();
    }

    // ���ز��
    bool loadPlugin(const std::string& pluginPath) {
        std::lock_guard<std::mutex> lock(mutex_);

        // �򿪶�̬��
        void* handle = dlopen(pluginPath.c_str(), RTLD_LAZY);
        if (!handle) {
            throw PlatformException("Failed to load plugin: " + std::string(dlerror()));
        }

        // ��ȡ��������
        using CreatePluginFunc = IPlugin* (*)();
        CreatePluginFunc createPlugin = (CreatePluginFunc)dlsym(handle, "createPlugin");
        if (!createPlugin) {
            dlclose(handle);
            throw PlatformException("Invalid plugin format: createPlugin function not found");
        }

        // �������ʵ��
        std::shared_ptr<IPlugin> plugin(createPlugin());
        if (!plugin) {
            dlclose(handle);
            throw PlatformException("Failed to create plugin instance");
        }

        // ��ʼ�����
        if (!plugin->initialize()) {
            dlclose(handle);
            throw PlatformException("Plugin initialization failed");
        }

        // �洢�����Ϣ
        PluginInfo info{handle, plugin, pluginPath, true};
        plugins_[plugin->getName()] = info;

        return true;
    }

    // ж�ز��
    bool unloadPlugin(const std::string& pluginName) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = plugins_.find(pluginName);
        if (it == plugins_.end()) {
            return false;
        }

        if (it->second.isLoaded) {
            it->second.plugin->shutdown();
            dlclose(it->second.handle);
            it->second.isLoaded = false;
        }

        plugins_.erase(it);
        return true;
    }

    // ж�����в��
    void unloadAllPlugins() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (auto& pair : plugins_) {
            if (pair.second.isLoaded) {
                pair.second.plugin->shutdown();
                dlclose(pair.second.handle);
            }
        }
        plugins_.clear();
    }

    // ��ȡ����б�
    std::vector<std::string> getLoadedPlugins() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<std::string> result;
        for (const auto& pair : plugins_) {
            if (pair.second.isLoaded) {
                result.push_back(pair.first);
            }
        }
        return result;
    }

    // ��ȡ���ʵ��
    template<typename T>
    std::shared_ptr<T> getPlugin(const std::string& pluginName) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = plugins_.find(pluginName);
        if (it == plugins_.end() || !it->second.isLoaded) {
            return nullptr;
        }

        return std::dynamic_pointer_cast<T>(it->second.plugin);
    }
};

// �Զ������ݼ����ʾ��
class CustomDatasetPlugin : public IDatasetPlugin {
private:
    std::string name_;
    std::string version_;
    std::string description_;
    std::vector<std::string> supportedFormats_;

public:
    CustomDatasetPlugin()
        : name_("CustomDataset"),
          version_("1.0"),
          description_("Custom dataset plugin example"),
          supportedFormats_{".custom", ".cdt"} {}

    std::string getName() const override { return name_; }
    std::string getVersion() const override { return version_; }
    std::string getDescription() const override { return description_; }

    bool initialize() override {
        // ִ�г�ʼ���߼�
        return true;
    }

    void shutdown() override {
        // ִ�������߼�
    }

    std::shared_ptr<IDataset> createDataset() override {
        // �����Զ������ݼ�ʵ��
        return nullptr; // ʵ��ʵ����Ҫ���ؾ�������ݼ�ʵ��
    }

    std::vector<std::string> getSupportedFormats() const override {
        return supportedFormats_;
    }
};

// ���ע��ӿ�
extern "C" {
    IPlugin* createPlugin() {
        return new CustomDatasetPlugin();
    }
}

} // namespace DataPlatform

#endif // EXTENSION_INTERFACE_H