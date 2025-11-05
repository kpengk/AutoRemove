#pragma once

#include "elapsed_timer.hpp"

#include <atomic>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;


class AutoRemove {
public:
    struct MonitorConfig {
        std::string path;
        size_t max_files_count;
        size_t min_file_age_hours;
        std::string wildcard_pattern;

        MonitorConfig(const std::string& p, size_t max, size_t hours, std::string wildcard)
            : path(p), max_files_count(max), min_file_age_hours(hours), wildcard_pattern{std::move(wildcard)} {}
    };

    AutoRemove(const std::string& config_path);
    ~AutoRemove();

    bool load_config();
    void run();
    void stop();

private:
    enum class LogLevel { INFO, WARNING, ERROR };

    std::string to_regex_pattern(const std::string& pattern);
    void wait_for_next_scan();
    void scan_path(const MonitorConfig& config);
    std::vector<fs::directory_entry> get_files(const fs::path& path, const std::string& wildcard_pattern);
    bool matches_wildcard(const std::string& filename, const std::string& pattern);
    void remove_old_files(const MonitorConfig& config, std::vector<fs::directory_entry> files);
    bool should_remove(const fs::directory_entry& entry, size_t hours_threshold);
    void log(LogLevel level, const std::string& message);

    const std::string config_path_;
    std::size_t scan_interval_minutes_;
    std::vector<MonitorConfig> monitor_configs_;
    std::atomic_bool running_{false};
    elapsed_timer timer_;
};