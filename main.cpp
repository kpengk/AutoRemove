#include "AutoRemove.hpp"

#include <csignal>
#include <iostream>

std::unique_ptr<AutoRemove> auto_remove;

void signalHandler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down...\n";
    if (auto_remove) {
        auto_remove->stop();
    }
}

int main(int argc, char* argv[]) {
    // Set signal processing
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::string config_path = "config.json";
    if (argc > 1) {
        config_path = argv[1];
    }

    try {
        auto_remove = std::make_unique<AutoRemove>(config_path);

        if (!auto_remove->load_config()) {
            std::cerr << "Failed to load configuration. Exiting.\n";
            return 1;
        }

        auto_remove->run();
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}