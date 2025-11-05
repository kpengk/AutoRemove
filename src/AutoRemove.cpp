#include "AutoRemove.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <thread>

#include <nlohmann/json.hpp>


AutoRemove::AutoRemove(const std::string& config_path) : config_path_{config_path}, scan_interval_minutes_{60} {}

AutoRemove::~AutoRemove() {
    stop();
}

bool AutoRemove::load_config() {
    try {
        std::ifstream config_file(config_path_);
        if (!config_file.is_open()) {
            log(LogLevel::ERROR, "Cannot open config file: " + config_path_);
            return false;
        }

        nlohmann::json config_json;
        config_file >> config_json;

        // Read the scanning interval
        if (config_json.contains("scan_interval_minutes")) {
            scan_interval_minutes_ = config_json["scan_interval_minutes"];
        }

        // Read the monitoring configuration
        if (config_json.contains("monitor_paths") && config_json["monitor_paths"].is_array()) {
            monitor_configs_.clear();
            for (const auto& path_config : config_json["monitor_paths"]) {
                const std::string path = path_config["path"];
                const size_t max_files_count = path_config["max_files_count"];
                const size_t min_file_age_hours = path_config["min_file_age_hours"];

                // Read wildcard mode (optional)
                std::string wildcard_pattern;
                if (path_config.contains("wildcard_pattern")) {
                    wildcard_pattern = to_regex_pattern(path_config["wildcard_pattern"]);
                }

                monitor_configs_.emplace_back(path, max_files_count, min_file_age_hours, std::move(wildcard_pattern));
            }
        }

        log(LogLevel::INFO, "Config loaded successfully. Monitor paths: " + std::to_string(monitor_configs_.size())
                                + ", Scan interval: " + std::to_string(scan_interval_minutes_) + " minutes.");
        return true;
    } catch (const std::exception& ex) {
        log(LogLevel::ERROR, "Error loading config: " + std::string(ex.what()));
        return false;
    }
}

void AutoRemove::run() {
    if (monitor_configs_.empty()) {
        log(LogLevel::ERROR, "No monitor configurations loaded");
        return;
    }

    running_.store(true);
    log(LogLevel::INFO, "AutoRemove started");

    while (running_) {
        log(LogLevel::INFO, "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
        log(LogLevel::INFO, "Starting scan cycle...");
        timer_.restart();

        for (const auto& config : monitor_configs_) {
            try {
                scan_path(config);
            } catch (const std::exception& ex) {
                log(LogLevel::ERROR, "Error scanning path " + config.path + ": " + ex.what());
            }
        }

        log(LogLevel::INFO, "Scan cycle completed.");

        // Wait for the next scan, but it is allowed to be interrupted by stop.
        wait_for_next_scan();
    }
    log(LogLevel::INFO, "AutoRemove stopped");
}

void AutoRemove::stop() {
    running_.store(false);
}


std::string AutoRemove::to_regex_pattern(const std::string& pattern) {
    std::string regex_pattern;
    for (char c : pattern) {
        if (c == '*') {
            regex_pattern += ".*";
        } else {
            regex_pattern += c;
        }
    }
    return regex_pattern;
}

void AutoRemove::wait_for_next_scan() {
    log(LogLevel::INFO, "Wait for the next scan...");
    while (running_.load()) {
        if (timer_.elapsed() >= scan_interval_minutes_ * 60) {
            break;
        }

        const auto remaining = scan_interval_minutes_ * 60 - timer_.elapsed();
        const auto wait_time = std::min(remaining, 2ULL);
        std::this_thread::sleep_for(std::chrono::seconds(wait_time));
    }
}

void AutoRemove::scan_path(const MonitorConfig& config) {
    const fs::path monitor_path(config.path);

    if (!fs::exists(monitor_path)) {
        log(LogLevel::WARNING, "Path does not exist: " + config.path);
        return;
    }

    if (!fs::is_directory(monitor_path)) {
        log(LogLevel::WARNING, "Path is not a directory: " + config.path);
        return;
    }

    log(LogLevel::INFO, "Scan path: " + config.path);
    auto files = get_files(monitor_path, config.wildcard_pattern);

    if (files.size() > config.max_files_count) {
        log(LogLevel::INFO, "Path " + config.path + " has " + std::to_string(files.size()) + " items (max: "
                                + std::to_string(config.max_files_count) + "), checking for removal...");
        remove_old_files(config, std::move(files));
    }
}

std::vector<fs::directory_entry> AutoRemove::get_files(const fs::path& path, const std::string& pattern) {
    std::vector<fs::directory_entry> files;

    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            // If there is no wildcard mode, include all files
            if (pattern.empty()) {
                files.push_back(entry);
                continue;
            }

            try {
                // Check whether the file name matches the wildcard pattern
                const std::string filename = entry.path().filename().string();
                const std::regex re(pattern, std::regex_constants::icase);
                if (std::regex_match(filename, re)) {
                    files.push_back(entry);
                }
            } catch (const std::regex_error& ex) {
                log(LogLevel::ERROR, std::string("Invalid wildcard pattern: ") + ex.what());
            }
        }
    } catch (const fs::filesystem_error& ex) {
        log(LogLevel::ERROR, "Error accessing path " + path.string() + ": " + ex.what());
    }

    return files;
}

bool AutoRemove::matches_wildcard(const std::string& filename, const std::string& regex_pattern) {
    try {
        const std::regex re(regex_pattern, std::regex_constants::icase);
        return std::regex_match(filename, re);
    } catch (const std::regex_error& ex) {
        log(LogLevel::ERROR, std::string("Invalid wildcard pattern: ") + ex.what());
        return false;
    }
}

void AutoRemove::remove_old_files(const MonitorConfig& config, std::vector<fs::directory_entry> files) {
    size_t remove_count = 0;

    // Sort by the last modification time (the earliest is first).
    std::sort(files.begin(), files.end(), [](const fs::directory_entry& a, const fs::directory_entry& b) {
        return fs::last_write_time(a) < fs::last_write_time(b);
    });

    for (const auto& file : files) {
        if (files.size() - remove_count <= config.max_files_count) {
            break;
        }

        try {
            if (should_remove(file, config.min_file_age_hours)) {
                if (fs::is_directory(file)) {
                    fs::remove_all(file);
                    log(LogLevel::INFO, "Removed directory: " + file.path().string());
                } else {
                    fs::remove(file);
                    log(LogLevel::INFO, "Removed file: " + file.path().string());
                }
                ++remove_count;
            } else {
                break;
            }
        } catch (const fs::filesystem_error& ex) {
            log(LogLevel::ERROR, "Error removing " + file.path().string() + ": " + ex.what());
        }
    }

    if (remove_count > 0) {
        log(LogLevel::INFO, "Removed " + std::to_string(remove_count) + " items from " + config.path);
    }
}

bool AutoRemove::should_remove(const fs::directory_entry& entry, size_t hours_threshold) {
    const auto file_time = fs::last_write_time(entry);
    const auto current_time = fs::file_time_type::clock::now();
    const auto hours = std::chrono::duration_cast<std::chrono::hours>(current_time - file_time).count();
    return hours > hours_threshold;
}

void AutoRemove::log(LogLevel level, const std::string& message) {
    const auto now = std::chrono::system_clock::now();
    const auto time_t = std::chrono::system_clock::to_time_t(now);
    const auto tm = *std::localtime(&time_t);

    char level_char;
    switch (level) {
    case LogLevel::INFO:
        level_char = 'I';
        break;
    case LogLevel::WARNING:
        level_char = 'W';
        break;
    case LogLevel::ERROR:
        level_char = 'E';
        break;
    default:
        level_char = 'I';
        break;
    }

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " " << level_char << " - " << message << "\n";
    std::cout << oss.str();
}