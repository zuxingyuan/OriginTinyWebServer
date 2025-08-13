#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <unordered_map>
#include <pthread.h>
#include <string>
#include <mutex>
#include <vector>
#include <ctime>

// 前向声明
namespace Json {
    class Value;
}

namespace storage
{
    // 前向声明
    class Config;
    class FileUtil;
    class JsonUtil;

    typedef struct StorageInfo
    {
        time_t mtime_;
        time_t atime_;
        size_t fsize_;
        std::string storage_path_;
        std::string url_;

        // 声明方法，实现移到 .cpp
        bool NewStorageInfo(const std::string &storage_path);
    } StorageInfo;

    class DataManager
    {
    private:
        std::string storage_file_;
        pthread_rwlock_t rwlock_;
        std::unordered_map<std::string, StorageInfo> table_;
        bool need_persist_;

        static DataManager *instance_;
        static std::mutex instance_mutex_;

        // 私有构造函数
        DataManager();
        DataManager(const DataManager &) = delete;
        DataManager &operator=(const DataManager &) = delete;

    public:
        ~DataManager();

        // 单例方法
        static DataManager *GetInstance();
        static void DestroyInstance();

        // 数据操作方法
        bool InitLoad();
        bool Storage();
        bool Insert(const StorageInfo &info);
        bool Update(const StorageInfo &info);
        bool GetOneByURL(const std::string &key, StorageInfo *info);
        bool GetOneByStoragePath(const std::string &storage_path, StorageInfo *info);
        bool GetAll(std::vector<StorageInfo> *array);
    };
}

#endif
