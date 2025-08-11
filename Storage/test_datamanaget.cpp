#include <iostream>
#include "DataManager.hpp"
#include <fstream>
#include <unistd.h> // for sleep

using namespace storage;

int main()
{
    // // 假设 Config 已经正确配置了 StorageInfoFile 路径 和 DownloadPrefix
    // Config::GetInstance()->SetStorageInfoFile("storage_info.json");
    // Config::GetInstance()->SetDownloadPrefix("/download/");

    // 创建一个测试文件
    std::string test_file = "test_upload.txt";
    std::ofstream ofs(test_file);
    ofs << "This is a test file for DataManager." << std::endl;
    ofs.close();

    // 1. 构造 StorageInfo
    StorageInfo info;
    if (!info.NewStorageInfo(test_file))
    {
        std::cerr << "❌ Failed to get file info" << std::endl;
        return 1;
    }

    std::cout << "✅ File info loaded: "
              << "\n  Path: " << info.storage_path_
              << "\n  Size: " << info.fsize_
              << "\n  URL : " << info.url_ << std::endl;

    // 2. 创建 DataManager
    DataManager dm;

    // 3. 插入数据
    if (!dm.Insert(info))
    {
        std::cerr << "❌ Failed to insert file info" << std::endl;
        return 1;
    }
    std::cout << "✅ Insert success" << std::endl;

    // 4. 通过 URL 获取
    StorageInfo got_by_url;
    if (dm.GetOneByURL(info.url_, &got_by_url))
    {
        std::cout << "✅ GetOneByURL success, file size = " << got_by_url.fsize_ << std::endl;
    }
    else
    {
        std::cerr << "❌ GetOneByURL failed" << std::endl;
    }

    // 5. 通过存储路径获取
    StorageInfo got_by_path;
    if (dm.GetOneByStoragePath(info.storage_path_, &got_by_path))
    {
        std::cout << "✅ GetOneByStoragePath success, URL = " << got_by_path.url_ << std::endl;
    }
    else
    {
        std::cerr << "❌ GetOneByStoragePath failed" << std::endl;
    }

    // 6. 获取全部文件信息
    std::vector<StorageInfo> all_infos;
    dm.GetAll(&all_infos);
    std::cout << "📂 Total stored files: " << all_infos.size() << std::endl;
    for (auto &e : all_infos)
    {
        std::cout << "  - " << e.url_ << " (" << e.fsize_ << " bytes)" << std::endl;
    }

    // 7. 持久化测试（Storage 会在 Insert 时自动调用）
    std::cout << "💾 Persisted to " << Config::GetInstance()->GetStorageInfoFile() << std::endl;

    // 8. 测试 InitLoad：重新创建 DataManager，看是否能加载
    {
        DataManager dm2;
        std::vector<StorageInfo> loaded;
        dm2.GetAll(&loaded);
        std::cout << "♻ Reloaded files: " << loaded.size() << std::endl;
    }

    return 0;
}


// g++ test_datamanaget.cpp -o main -lpthread -ljsoncpp -L/root/Program/bundle -lbundle -lstdc++fs -std=c++11 && ./main