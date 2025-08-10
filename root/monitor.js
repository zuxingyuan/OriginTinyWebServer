document.addEventListener('DOMContentLoaded', () => {
    // 获取页面上需要更新的元素
    const cpuValueSpan = document.getElementById('cpu-value');
    const memoryValueSpan = document.getElementById('memory-value');
    const requestValueSpan = document.getElementById('request-value');
    const uptimeValueSpan = document.getElementById('uptime-value');
    const connectionsValueSpan = document.getElementById('connections-value');
    const logMessagesDiv = document.getElementById('log-messages');

    // 将秒转换为更易读的格式 (D H M S)
    function formatUptime(totalSeconds) {
        const days = Math.floor(totalSeconds / (3600 * 24));
        totalSeconds %= (3600 * 24);
        const hours = Math.floor(totalSeconds / 3600);
        totalSeconds %= 3600;
        const minutes = Math.floor(totalSeconds / 60);
        const seconds = totalSeconds % 60;

        let result = [];
        if (days > 0) result.push(`${days}d`);
        if (hours > 0 || days > 0) result.push(`${hours}h`); // 只有当有天数或小时数时才显示小时
        if (minutes > 0 || hours > 0 || days > 0) result.push(`${minutes}m`); // 只有当有小时数或分钟数时才显示分钟
        result.push(`${seconds}s`); // 秒总是显示

        return result.join(' ');
    }

    async function fetchAndDisplayMetrics() {
        try {
            // 向你的C++ WebServer的API接口发送请求
            const response = await fetch('./monitor'); 
            
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            
            const data = await response.json(); // 解析JSON响应

            // 更新HTML元素
            cpuValueSpan.innerText = `${data.cpu_usage_percent}%`;
            memoryValueSpan.innerText = `${data.memory_usage_mb} MB`;
            requestValueSpan.innerText = data.total_requests;
            uptimeValueSpan.innerText = formatUptime(data.uptime_seconds);
            connectionsValueSpan.innerText = data.active_connections;

            // TODO: 如果你实现了线程池队列大小，在这里更新
            // const threadQueueValueSpan = document.getElementById('thread-queue-value');
            // if (data.thread_pool_queue_size !== undefined) {
            //     threadQueueValueSpan.innerText = data.thread_pool_queue_size;
            // }

            // 静态日志消息，因为我们还没有日志API
            logMessagesDiv.innerText = "实时日志功能尚未实现。"; 
            // 如果你未来实现了日志API（例如 /logs），这里可以这样：
            // const logResponse = await fetch('/logs');
            // const logData = await logResponse.text(); // 或者解析日志JSON
            // logMessagesDiv.innerText = logData;


        } catch (error) {
            console.error('Error fetching monitor data:', error);
            // 发生错误时显示错误信息
            cpuValueSpan.innerText = 'Error';
            memoryValueSpan.innerText = 'Error';
            requestValueSpan.innerText = 'Error';
            uptimeValueSpan.innerText = 'Error';
            connectionsValueSpan.innerText = 'Error';
            // threadQueueValueSpan.innerText = 'Error';
            logMessagesDiv.innerText = 'Error loading logs.';
        }
    }

    // 每隔一段时间刷新数据 (例如：每2秒)
    const refreshInterval = 2000; // 2000 毫秒 = 2 秒
    setInterval(fetchAndDisplayMetrics, refreshInterval);

    // 页面加载后立即获取一次数据
    fetchAndDisplayMetrics();
});
