#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <unordered_map>
#include <pthread.h>
#include <string>
#include "../Util/Config.hpp"
namespace storage
{
    // 用作初始化存储文件的属性信息
    typedef struct StorageInfo
    {
        time_t mtime_;
        time_t atime_;
        size_t fsize_;
        std::string storage_path_; // 文件存储路径
        std::string url_;          // 请求URL中的资源路径

        bool NewStorageInfo(const std::string &storage_path)
        {
            FileUtil f(storage_path);
            if (!f.Exists())
            {
                // 文件不存在
                return false;
            }
            mtime_ = f.LastAccessTime();
            atime_ = f.LastModifyTime();
            fsize_ = f.FileSize();
            storage_path_ = storage_path;
            // URL实际就是用户下载文件请求的路径
            // 下载路径前缀+文件名
            storage::Config *config = storage::Config::GetInstance();
            url_ = config->GetDownloadPrefix() + f.FileName();

            return true;
        }
    } StorageInfo; // namespace StorageInfo

    class DataManager
    {
    private:
        std::string storage_file_;                           // 存储文件的路径
        pthread_rwlock_t rwlock_;                            // 读写锁
        std::unordered_map<std::string, StorageInfo> table_; // 存储文件的属性信息
        bool need_persist_;                                  // 是否需要持久化存储文件信息
    public:
        DataManager()
        {
            storage_file_ = storage::Config::GetInstance()->GetStorageInfoFile();
            pthread_rwlock_init(&rwlock_, NULL);
            need_persist_ = false;
            InitLoad();
            need_persist_ = true; // 初始化后需要持久化存储文件信息
        }
        ~DataManager()
        {
            pthread_rwlock_destroy(&rwlock_);
        }
        bool InitLoad() // 初始化加载存储文件信息
        {
            storage::FileUtil f(storage_file_);
            if (!f.Exists())
            {
                // 文件不存在，直接返回
                return true;
            }
            std::string body;
            if (!f.GetContent(&body))
                return false;

            // 反序列化存储文件信息
            Json::Value root;
            storage::JsonUtil::UnSerialize(body, &root);

            // 将反序列化得到的Json::Value中的数据添加到table中
            for (int i = 0; i < root.size(); i++)
            {
                StorageInfo info;
                info.fsize_ = root[i]["fsize_"].asInt();
                info.atime_ = root[i]["atime_"].asInt();
                info.mtime_ = root[i]["mtime_"].asInt();
                info.storage_path_ = root[i]["storage_path_"].asString();
                info.url_ = root[i]["url_"].asString();
                Insert(info);
            }
            return true;
        }
        bool Storage() // 持久化存储文件信息
        {
            std::vector<StorageInfo> arr;
            if (!GetAll(&arr))
            {
                // 获取所有存储文件信息失败
                return false;
            }

            Json::Value root;
            for (auto e : arr)
            {
                Json::Value item;
                item["mtime_"] = (Json::Int64)e.mtime_;
                item["atime_"] = (Json::Int64)e.atime_;
                item["fsize_"] = (Json::Int64)e.fsize_;
                item["url_"] = e.url_.c_str();
                item["storage_path_"] = e.storage_path_.c_str();
                root.append(item); // 作为数组
            }

            // 序列化Json::Value到字符串
            std::string body;
            JsonUtil::Serialize(root, &body);
            // 写入文件
            FileUtil f(storage_file_);

            f.SetContent(body.c_str(), body.size());
            return true;
        }
        bool Insert(const StorageInfo &info) // 插入存储文件信息
        {
            pthread_rwlock_wrlock(&rwlock_);
            table_[info.url_] = info;
            pthread_rwlock_unlock(&rwlock_);
            if (need_persist_ && !Storage())
            {
                // 持久化存储文件信息
                return false;
            }
            return true;
        }

        bool Updata(const StorageInfo &info) // 更新存储文件信息
        {
            pthread_rwlock_wrlock(&rwlock_);
            table_[info.url_] = info;
            pthread_rwlock_unlock(&rwlock_);
            if (Storage() == false)
            {
                return false;
            }
            return true;
        }

        bool GetOneByURL(const std::string &key, StorageInfo *info)
        {
            pthread_rwlock_rdlock(&rwlock_);
            // URL是key，所以直接find()找
            if (table_.find(key) == table_.end())
            {
                pthread_rwlock_unlock(&rwlock_);
                return false;
            }
            *info = table_[key]; // 获取url对应的文件存储信息
            pthread_rwlock_unlock(&rwlock_);
            return true;
        }

        bool GetOneByStoragePath(const std::string &storage_path, StorageInfo *info)
        {
            pthread_rwlock_rdlock(&rwlock_);
            // 遍历 通过realpath字段找到对应存储信息
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
        bool GetAll(std::vector<StorageInfo> *arry)
        {
            pthread_rwlock_rdlock(&rwlock_);
            for (auto e : table_)
                arry->emplace_back(e.second);
            pthread_rwlock_unlock(&rwlock_);
            return true;
        }
    };

}

#endif