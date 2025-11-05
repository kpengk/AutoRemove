<p align="center">
	<h1 align="center">AutoRemove</h1>
</p>

AutoRemove 是一个基于 `C++17` 开发的自动文件清理工具，用于监控指定目录并在文件数量超过限制时自动删除最旧的文件。

# 功能特性

- 🔄 **定时扫描**：可配置的定时扫描间隔；
- 📁 **多路径监控**：支持同时监控多个目录；
- ⏰ **智能清理**：基于文件数量和创建时间自动清理；
- 🎯 **通配符匹配**：支持文件名通配符过滤；
- 📊 **详细日志**：完整的操作日志和统计信息；
- 🛡️ **安全可靠**：优雅的信号处理和异常捕获。

# 快速开始

## 系统要求

- C++17 兼容编译器 (GCC 11+, Clang 12+, MSVC 19.30+)
- CMake 3.16+
- [`nlohmann/json`](https://github.com/nlohmann/json) 库

## 构建项目

```bash
# 克隆项目
git clone https://github.com/kpengk/AutoRemove.git
cd AutoRemove

# 创建构建目录
mkdir build && cd build

# 配置和构建
cmake ..
cmake --build . --config Release
```

## 运行工具

```bash
# 使用默认配置文件
./AutoRemove

# 使用自定义配置文件
./AutoRemove /path/to/config.json
```

# 配置文件说明

## 基本配置结构

```json
{
  "scan_interval_minutes": 1,
  "monitor_paths": [
    {
      "path": "D:/temp",
      "max_files_count": 10,
      "min_file_age_hours": 24,
      "wildcard_pattern": "*.log"
    }
  ]
}
```

## 配置参数详解

### 全局配置

- `scan_interval_minutes`: 扫描间隔时间（分钟）

### 路径监控配置

- `path`: 要监控的目录路径
- `max_files_count`: 允许保留的最大文件数量（0表示无限制）
- `min_file_age_hours`: 文件最小年龄（小时），只有超过此年龄的文件才会被删除
- `wildcard_pattern`: 文件名通配符模式（可选，如 "*.log"）

## 通配符模式示例

- `"*.log"`- 匹配所有 .log 文件
- `"app_*.txt"`- 匹配 app_ 开头 .txt 结尾的文件
- `"backup_*.zip"`- 匹配 backup_ 开头 .zip 结尾的文件
- 不设置该参数 - 匹配所有文件

# 使用示例

## 示例1：清理日志文件

```json
{
  "scan_interval_minutes": 60,
  "monitor_paths": [
    {
      "path": "D:/temp",
      "max_files_count": 0,
      "min_file_age_hours": 0,
      "wildcard_pattern": "*.log"
    }
  ]
}
```

**效果**：每小时扫描一次 D:/temp 目录，删除所有 .log 文件（无数量和年龄限制）

## 示例2：备份文件管理

```json
{
  "scan_interval_minutes": 1440,
  "monitor_paths": [
    {
      "path": "D:/backups",
      "max_files_count": 50,
      "min_file_age_hours": 168
    }
  ]
}
```

**效果**：每天扫描一次 D:/backups 目录，当文件数量超过50个时，删除超过7天(168小时)的最旧文件

## 示例3：临时文件清理

```json
{
  "scan_interval_minutes": 120,
  "monitor_paths": [
    {
      "path": "/var/tmp",
      "max_files_count": 200,
      "min_file_age_hours": 72,
      "wildcard_pattern": "*.temp"
    }
  ]
}
```

**效果**：每2小时扫描一次 /var/tmp 目录，当 .temp 文件超过200个时，删除超过3天(72小时)的最旧文件

# 信号处理

工具支持优雅的关闭方式：

- **Ctrl+C**(SIGINT): 安全停止工具
- **kill命令**(SIGTERM): 安全停止工具

# 故障排除

## 常见问题

1. **权限不足**：确保工具对监控目录有读写权限Linux系统可能需要使用 sudo 运行
2. **路径不存在**：检查配置中的路径是否正确确保目录存在且可访问
3. **文件删除失败**：检查文件是否被其他程序占用确认有足够的删除权限

## 日志级别

- **I (INFO)**: 常规操作信息
- **W (WARNING)**: 警告信息，不影响继续运行
- **E (ERROR)**: 错误信息，可能需要人工干预

# 许可证

MIT License

# 贡献

欢迎提交 Issue 和 Pull Request 来改进这个工具。
