#ifndef SERVER_METRICS_H
#define SERVER_METRICS_H

#include <string>
#include <atomic>
#include <chrono>
#include "../lock/locker.h"
#include <vector>

class ServerMetrics
{
public:
    // 获取单例实例
    static ServerMetrics &get_instance();

    // 禁止拷贝构造和赋值
    ServerMetrics(const ServerMetrics &) = delete;
    ServerMetrics &operator=(const ServerMetrics &) = delete;

    void increment_requests();
    long long get_total_requests() const;

    // 周期性释放
    void refresh_system_metrics();
    //  获取CPU和内存使用率
    double get_cpu_usage_percent() const;  // 获取CPU使用率百分比
    long long get_memory_usage_mb() const; // 获取内存使用量 (MB)

    // 获取服务器运行时间
    long long get_uptime_seconds() const;

    // 获取活跃连接数
    void increment_active_connections();
    void decrement_active_connections();
    int get_active_connections() const;

    // 将所有指标生成JSON格式字符串
    std::string to_json() const;

public:
    // 以下需要锁保护，因为它们可能由多个线程更新或读取
    mutable locker metrics_mutex_;

private:
    ServerMetrics(); // 私有构造函数，实现单例模式

    std::atomic<long long> total_requests_;
    std::atomic<int> active_connections_; // 活跃连接数

    double current_cpu_usage_;                                      // CPU 使用率百分比
    long long current_memory_usage_mb_;                             // 内存使用量 (MB)
    std::chrono::time_point<std::chrono::system_clock> start_time_; // 服务器启动时间

    // 记录上次CPU使用率计算的状态
    long long last_total_cpu_time_;
    long long last_idle_cpu_time_;

    // 外部函数用于更新CPU和内存使用率
    void update_cpu_usage_internal();
    void update_memory_usage_internal();
    // TODO: 线程池队列大小、日志等可以后续添加
    std::atomic<int> thread_pool_queue_size_;
    std::vector<std::string> log_buffer_; // 存储最近的日志
};

#endif // SERVER_METRICS_H
