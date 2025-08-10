#include "metrics.h"
#include <fstream>  // For reading /proc/stat and /proc/meminfo
#include <unistd.h> // For sysconf
#include <sstream>  // For std::stringstream
#include <iomanip>  // For std::fixed, std::setprecision
#include <ctime>    // For std::time_t, std::localtime, std::put_time if needed for datetime

// 实现单例模式
ServerMetrics &ServerMetrics::get_instance()
{
    static ServerMetrics instance;
    return instance;
}

ServerMetrics::ServerMetrics()
    : total_requests_(0),
      active_connections_(0),
      current_cpu_usage_(0.0),
      current_memory_usage_mb_(0),
      start_time_(std::chrono::system_clock::now()), // 记录服务器启动时间
      last_total_cpu_time_(0),                       // Initialize CPU tracking variables
      last_idle_cpu_time_(0)
{
    // 可以在这里初始化或启动一些监控任务，例如定时更新系统指标的线程
    // 为了简化，我们暂时通过 to_json() 内部模拟更新，或手动调用 update_cpu_memory
}

void ServerMetrics::increment_requests()
{
    total_requests_++;
}

long long ServerMetrics::get_total_requests() const
{
    return total_requests_.load();
}
// 周期性刷新系统指标
void ServerMetrics::refresh_system_metrics()
{
    metrics_mutex_.lock();
    update_cpu_usage_internal();
    update_memory_usage_internal();
    metrics_mutex_.unlock();
}

// 外部实现更新CPU使用率
void ServerMetrics::update_cpu_usage_internal()
{
    std::ifstream file("/proc/stat");
    std::string line;
    if (!file.is_open())
    {
        // Handle error: file not found or cannot be opened.
        // For simplicity, we'll just return and keep previous values.
        return;
    }

    if (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string cpu_label;
        long long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;

        // Parse the "cpu" line from /proc/stat
        // The values are typically: user nice system idle iowait irq softirq steal guest guest_nice
        // guest/guest_nice might not be present on older kernels, but istringstream handles extra fields gracefully.
        iss >> cpu_label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal >> guest >> guest_nice;

        // Calculate total CPU time (all jiffies)
        long long current_total_cpu_time = user + nice + system + idle + iowait + irq + softirq + steal + guest + guest_nice;
        // Calculate idle CPU time (idle + iowait)
        long long current_idle_cpu_time = idle + iowait;

        // Calculate differences since last read
        long long total_diff = current_total_cpu_time - last_total_cpu_time_;
        long long idle_diff = current_idle_cpu_time - last_idle_cpu_time_;

        if (total_diff > 0)
        {
            current_cpu_usage_ = 100.0 * (1.0 - (double)idle_diff / total_diff);
        }
        else
        {
            // If total_diff is 0, no CPU activity or first read. Keep previous value or set to 0.
            current_cpu_usage_ = 0.0;
        }

        // Store current values for next calculation
        last_total_cpu_time_ = current_total_cpu_time;
        last_idle_cpu_time_ = current_idle_cpu_time;
    }
}

void ServerMetrics::update_memory_usage_internal()
{
    std::ifstream file("/proc/meminfo");
    std::string line;
    long long mem_total = 0;
    long long mem_free = 0;
    long long mem_available = 0; // Preferred over MemFree for available memory

    if (!file.is_open())
    {
        printf("Error: Could not open /proc/meminfo\n");
        return;
    }

    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string key;
        long long value;
        std::string unit; // "kB"

        iss >> key >> value >> unit;

        if (key == "MemTotal:")
        {
            mem_total = value;
        }
        else if (key == "MemFree:")
        { // Fallback if MemAvailable is not present
            mem_free = value;
        }
        else if (key == "MemAvailable:")
        { // Most accurate for user-space applications
            mem_available = value;
        }

        // Optimization: if we found all needed keys, break early
        // We prioritize MemAvailable, so if it's found, MemFree is less important.
        if (mem_total > 0 && (mem_available > 0 || (mem_available == 0 && mem_free > 0)))
        {
            break;
        }
    }

    // Calculate used memory: MemTotal - MemAvailable (preferred) or MemTotal - MemFree
    if (mem_total > 0)
    {
        if (mem_available > 0)
        {
            current_memory_usage_mb_ = (mem_total - mem_available) / 1024; // Convert kB to MB
        }
        else if (mem_free > 0)
        {
            // Fallback to MemFree if MemAvailable is not present (older kernels)
            current_memory_usage_mb_ = (mem_total - mem_free) / 1024; // Convert kB to MB
        }
        else
        {
            current_memory_usage_mb_ = 0; // No valid free/available memory found
        }
    }
    else
    {
        current_memory_usage_mb_ = 0; // Could not read or total memory is 0
    }
}

double ServerMetrics::get_cpu_usage_percent() const
{
    return current_cpu_usage_;
}

long long ServerMetrics::get_memory_usage_mb() const
{
    return current_memory_usage_mb_;
}

long long ServerMetrics::get_uptime_seconds() const
{
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
    return duration.count();
}

void ServerMetrics::increment_active_connections()
{
    active_connections_++;
}

void ServerMetrics::decrement_active_connections()
{
    active_connections_--;
}

int ServerMetrics::get_active_connections() const
{
    return active_connections_.load();
}

std::string ServerMetrics::to_json() const
{
    // Note: It's important to understand that if to_json() is called without
    // a prior call to refresh_system_metrics(), the CPU and Memory values
    // will be stale. The lock here protects consistency of current values,
    // but doesn't trigger an update.
    metrics_mutex_.lock();
    std::ostringstream oss;
    oss << "{";
    oss << "\"cpu_usage_percent\":" << std::fixed << std::setprecision(2) << current_cpu_usage_ << ",";
    oss << "\"memory_usage_mb\":" << current_memory_usage_mb_ << ",";
    oss << "\"total_requests\":" << total_requests_.load() << ",";
    oss << "\"uptime_seconds\":" << get_uptime_seconds() << ",";
    oss << "\"active_connections\":" << active_connections_.load();
    oss << "}";
    std::string result = oss.str();
    metrics_mutex_.unlock();
    return result;
}
