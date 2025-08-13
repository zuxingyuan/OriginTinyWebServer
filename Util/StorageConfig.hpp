#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "Util.hpp"
#include <memory>
#include <mutex>

namespace storage
{
    extern const char *Config_File;

    class Config
    {
    private:
        int server_port_;
        std::string server_ip;
        std::string download_prefix_;
        std::string deep_storage_dir_;
        std::string low_storage_dir_;
        std::string storage_info_;
        int bundle_format_;

    private:
        static std::mutex _mutex; // 只声明，不定义
        static Config *_instance; // 只声明，不定义

        Config()
        {
            if (ReadConfig() == false)
            {
                return;
            }
        }

    public:
        bool ReadConfig()
        {
            
            storage::FileUtil fu(Config_File);
            std::string content;
            if (!fu.GetContent(&content))
            {
                return false;
            }
            Json::Value root;
            storage::JsonUtil::UnSerialize(content, &root);

            server_port_ = root["server_port"].asInt();
            server_ip = root["server_ip"].asString();
            download_prefix_ = root["download_prefix"].asString();
            storage_info_ = root["storage_info"].asString();
            deep_storage_dir_ = root["deep_storage_dir"].asString();
            low_storage_dir_ = root["low_storage_dir"].asString();
            bundle_format_ = root["bundle_format"].asInt();

            return true;
        }

        int GetServerPort() { return server_port_; }
        std::string GetServerIp() { return server_ip; }
        std::string GetDownloadPrefix() { return download_prefix_; }
        int GetBundleFormat() { return bundle_format_; }
        std::string GetDeepStorageDir() { return deep_storage_dir_; }
        std::string GetLowStorageDir() { return low_storage_dir_; }
        std::string GetStorageInfoFile() { return storage_info_; }

    public:
        static Config *GetInstance()
        {
            if (_instance == nullptr)
            {
                _mutex.lock();
                if (_instance == nullptr)
                {
                    _instance = new Config();
                }
                _mutex.unlock();
            }
            return _instance;
        }
    };

} // namespace storage

#endif // CONFIG_HPP
