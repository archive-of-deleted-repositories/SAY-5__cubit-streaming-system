#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include "config.h"

// Simple config file loader
// Supports basic KEY=VALUE format (INI-style)
// TODO: Could use a proper library like libconfig or JSON, but this works for now
class ConfigLoader {
public:
    static bool loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "[Config] Could not open config file: " << filename << std::endl;
            std::cerr << "[Config] Using default configuration" << std::endl;
            return false;
        }

        std::cout << "[Config] Loading configuration from: " << filename << std::endl;

        std::string line;
        int lineNum = 0;
        int settingsLoaded = 0;

        while (std::getline(file, line)) {
            lineNum++;

            // Remove whitespace
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);

            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') {
                continue;
            }

            // Parse KEY=VALUE
            size_t pos = line.find('=');
            if (pos == std::string::npos) {
                std::cerr << "[Config] Warning: Invalid line " << lineNum << ": " << line << std::endl;
                continue;
            }

            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // Trim whitespace from key and value
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            // Remove quotes from value if present
            if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }

            // Apply setting
            if (applySetting(key, value)) {
                settingsLoaded++;
            } else {
                std::cerr << "[Config] Warning: Unknown setting '" << key << "' on line " << lineNum << std::endl;
            }
        }

        file.close();
        std::cout << "[Config] Loaded " << settingsLoaded << " settings" << std::endl;
        return true;
    }

private:
    static bool applySetting(const std::string& key, const std::string& value) {
        // Camera settings
        if (key == "camera_width") {
            Config::CAMERA_WIDTH = std::stoi(value);
            return true;
        }
        if (key == "camera_height") {
            Config::CAMERA_HEIGHT = std::stoi(value);
            return true;
        }
        if (key == "camera_fps") {
            Config::CAMERA_FPS = std::stoi(value);
            return true;
        }
        if (key == "camera_device") {
            Config::CAMERA_DEVICE = value;
            return true;
        }

        // Encoding settings
        if (key == "initial_bitrate") {
            Config::INITIAL_BITRATE = std::stoi(value);
            return true;
        }
        if (key == "min_bitrate") {
            Config::MIN_BITRATE = std::stoi(value);
            return true;
        }
        if (key == "max_bitrate") {
            Config::MAX_BITRATE = std::stoi(value);
            return true;
        }
        if (key == "encoder_preset") {
            Config::ENCODER_PRESET = value;
            return true;
        }
        if (key == "use_nvenc") {
            Config::USE_NVENC = (value == "true" || value == "1" || value == "yes");
            return true;
        }

        // Network settings
        if (key == "udp_port") {
            Config::UDP_PORT = std::stoi(value);
            return true;
        }

        // Performance settings
        if (key == "capture_queue_size") {
            Config::CAPTURE_QUEUE_SIZE = std::stoi(value);
            return true;
        }
        if (key == "encode_queue_size") {
            Config::ENCODE_QUEUE_SIZE = std::stoi(value);
            return true;
        }

        return false;  // Unknown setting
    }
};

#endif // CONFIG_LOADER_H
