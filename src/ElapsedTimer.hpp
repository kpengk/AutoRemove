#pragma once
#include <chrono>
#include <cstdint>

class elapsed_timer {
public:
    elapsed_timer() : start_{std::chrono::steady_clock::now()} {}

    std::size_t restart() {
        const auto end{std::chrono::steady_clock::now()};
        const auto value{std::chrono::duration_cast<std::chrono::seconds>(end - start_).count()};
        start_ = end;
        return value;
    }

    std::size_t elapsed() {
        const auto end{std::chrono::steady_clock::now()};
        const auto value{std::chrono::duration_cast<std::chrono::seconds>(end - start_).count()};
        return value;
    }

private:
    std::chrono::steady_clock::time_point start_;
};
