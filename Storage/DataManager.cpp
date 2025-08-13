#include "DataManager.h"
#include "../Util/StorageConfig.hpp"
#include <json/json.h>
#include <cstring>

namespace storage
{
    // 静态成员变量定义
    DataManager* DataManager::instance_ = nullptr;
    std::mutex DataManager::instance_mutex_;

    // StorageInfo 方法实现
    bool StorageInfo::NewStorageInfo(const std::string &storage_path)
    {
        FileUtil f(storage_path);
        if (!f.Exists())
        {
            return false;
        }
        mtime_ = f.LastAccessTime();
        atime_ = f.LastModifyTime();
        fsize_ = f.FileSize();
        storage_path_ = storage_path;

        storage::Config *config = storage::Config::GetInstance();
        url_ = config->GetDownloadPrefix() + f.FileName();
        return true;
    }

    // DataManager 构造函数
    DataManager::DataManager()
    {
        storage::Config *config = storage::Config::GetInstance();
        storage_file_ = config ? config->GetStorageInfoFile() : "./storage.data";
        
        // 检查 pthread_rwlock_init 的返回值
        int ret = pthread_rwlock_init(&rwlock_, NULL);
        if (ret != 0)
        {
            throw std::runtime_error("Failed to initialize pthread_rwlock");
        }

        need_persist_ = false;
        InitLoad();
        need_persist_ = true;
    }

    // 析构函数
    DataManager::~DataManager()
    {
        pthread_rwlock_destroy(&rwlock_);
    }

    // 单例方法实现
    DataManager *DataManager::GetInstance()
    {
        if (instance_ == nullptr)
        {
            std::lock_guard<std::mutex> lock(instance_mutex_);
            if (instance_ == nullptr)
            {
                try
                {
                    instance_ = new DataManager();
                }
                catch (...)
                {
                    return nullptr;
                }
            }
        }
        return instance_;
    }

    void DataManager::DestroyInstance()
    {
        std::lock_guard<std::mutex> lock(instance_mutex_);
        if (instance_ != nullptr)
        {
            delete instance_;
            instance_ = nullptr;
        }
    }

    // 初始化加载
    bool DataManager::InitLoad()
    {
        storage::FileUtil f(storage_file_);
        if (!f.Exists())
        {
            return true;
        }

        std::string body;
        if (!f.GetContent(&body))
        {
            return false;
        }

        if (body.empty())
        {
            return true;
        }

        Json::Value root;
        if (!storage::JsonUtil::UnSerialize(body, &root))
        {
            return false;
        }

        if (!root.isArray())
        {
            return true;
        }

        // 直接操作table_，不调用Insert()避免死锁
        for (Json::Value::ArrayIndex i = 0; i < root.size(); i++)
        {
            try
            {
                const Json::Value &item = root[i];
                StorageInfo info;
                info.fsize_ = item.get("fsize_", 0).asInt();
                info.atime_ = item.get("atime_", 0).asInt();
                info.mtime_ = item.get("mtime_", 0).asInt();
                info.storage_path_ = item.get("storage_path_", "").asString();
                info.url_ = item.get("url_", "").asString();

                if (!info.url_.empty() && !info.storage_path_.empty())
                {
                    table_[info.url_] = info; // 直接插入，不调用Insert()
                }
            }
            catch (...)
            {
                continue;
            }
        }
        return true;
    }

    // 存储到文件
    bool DataManager::Storage()
    {
        pthread_rwlock_rdlock(&rwlock_);

        Json::Value root;
        for (auto e : table_) // 直接访问table_
        {
            Json::Value item;
            item["mtime_"] = (Json::Int64)e.second.mtime_;
            item["atime_"] = (Json::Int64)e.second.atime_;
            item["fsize_"] = (Json::Int64)e.second.fsize_;
            item["url_"] = e.second.url_.c_str();
            item["storage_path_"] = e.second.storage_path_.c_str();
            root.append(item);
        }

        pthread_rwlock_unlock(&rwlock_);

        // 在锁外进行文件操作
        std::string body;
        JsonUtil::Serialize(root, &body);
        FileUtil f(storage_file_);
        return f.SetContent(body.c_str(), body.size());
    }

    // 插入数据
    bool DataManager::Insert(const StorageInfo &info)
    {
        // 尝试获取写锁，并检查返回值
        int ret = pthread_rwlock_wrlock(&rwlock_);
        if (ret != 0)
        {
            return false;
        }

        table_[info.url_] = info;
        pthread_rwlock_unlock(&rwlock_);

        if (need_persist_ && !Storage())
        {
            return false;
        }
        return true;
    }

    // 更新数据
    bool DataManager::Update(const StorageInfo &info)
    {
        pthread_rwlock_wrlock(&rwlock_);
        table_[info.url_] = info;
        pthread_rwlock_unlock(&rwlock_);

        if (!Storage())
        {
            return false;
        }
        return true;
    }

    // 根据URL获取数据
    bool DataManager::GetOneByURL(const std::string &key, StorageInfo *info)
    {
        pthread_rwlock_rdlock(&rwlock_);
        if (table_.find(key) == table_.end())
        {
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }
        *info = table_[key];
        pthread_rwlock_unlock(&rwlock_);
        return true;
    }

    // 根据存储路径获取数据
    bool DataManager::GetOneByStoragePath(const std::string &storage_path, StorageInfo *info)
    {
        pthread_rwlock_rdlock(&rwlock_);
        for (auto e : table_)
        {
            if (e.second.storage_path_ == storage_path)
            {
                *info = e.second;
                pthread_rwlock_unlock(&rwlock_);
                return true;
            }
        }
        pthread_rwlock_unlock(&rwlock_);
        return false;
    }

    // 获取所有数据
    bool DataManager::GetAll(std::vector<StorageInfo> *array)
    {
        pthread_rwlock_rdlock(&rwlock_);
        for (auto e : table_)
        {
            array->emplace_back(e.second);
        }
        pthread_rwlock_unlock(&rwlock_);
        return true;
    }
}
