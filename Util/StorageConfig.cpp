#include "StorageConfig.hpp"

namespace storage
{
    // 在源文件中定义静态成员
    std::mutex Config::_mutex;
    Config *Config::_instance = nullptr;
    const char *Config_File = "Storage.conf";
}
