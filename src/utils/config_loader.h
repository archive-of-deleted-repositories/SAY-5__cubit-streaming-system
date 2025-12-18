#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include "config.h"

// Configuration file loader with validation
// Supports KEY=VALUE format with comprehensive bounds checking
class ConfigLoader {
public:
    // Validation constraints to prevent crashes
    struct Limits {
        // Camera constraints
        static constexpr int MIN_WIDTH = 320;
        static constexpr int MAX_WIDTH = 3840;     // 4K
        static constexpr int MIN_HEIGHT = 240;
        static constexpr int MAX_HEIGHT = 2160;    // 4K
        static constexpr int MIN_FPS = 1;
        static constexpr int MAX_FPS = 240;

        // Bitrate constraints (bits per second)
        static constexpr int MIN_BITRATE = 100000;      // 100 kbps
        static constexpr int MAX_BITRATE = 50000000;    // 50 Mbps

        // Queue constraints
        static constexpr int MIN_QUEUE_SIZE = 2;
        static constexpr int MAX_QUEUE_SIZE = 100;

        // Port constraints
        static constexpr int MIN_PORT = 1024;
        static constexpr int MAX_PORT = 65535;
    };

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
        int validationErrors = 0;

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
                std::cerr << "[Config] ERROR line " << lineNum
                          << ": Invalid format (missing '='): " << line << std::endl;
                validationErrors++;
                continue;
            }

            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            // Remove quotes
            if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }

            // Apply setting with validation
            std::string errorMsg;
            if (applySetting(key, value, errorMsg)) {
                settingsLoaded++;
            } else {
                std::cerr << "[Config] ERROR line " << lineNum << ": " << errorMsg << std::endl;
                validationErrors++;
            }
        }

        file.close();

        std::cout << "[Config] Loaded " << settingsLoaded << " settings successfully" << std::endl;

        if (validationErrors > 0) {
            std::cerr << "[Config] WARNING: " << validationErrors
                      << " validation errors found" << std::endl;
            std::cerr << "[Config] Invalid settings ignored, using defaults" << std::endl;
        }

        // Print final validated configuration
        printConfig();

        return validationErrors == 0;
    }

private:
    static bool applySetting(const std::string& key, const std::string& value,
                            std::string& errorMsg) {
        try {
            // Camera width
            if (key == "camera_width") {
                int width = std::stoi(value);
                if (width < Limits::MIN_WIDTH || width > Limits::MAX_WIDTH) {
                    errorMsg = "camera_width must be " + std::to_string(Limits::MIN_WIDTH) +
                              "-" + std::to_string(Limits::MAX_WIDTH) + " (got " + value + ")";
                    return false;
                }
                // H.264 requires even dimensions
                if (width % 2 != 0) {
                    errorMsg = "camera_width must be even for H.264 (got " + value + ")";
                    return false;
                }
                Config::CAMERA_WIDTH = width;
                return true;
            }

            // Camera height
            if (key == "camera_height") {
                int height = std::stoi(value);
                if (height < Limits::MIN_HEIGHT || height > Limits::MAX_HEIGHT) {
                    errorMsg = "camera_height must be " + std::to_string(Limits::MIN_HEIGHT) +
                              "-" + std::to_string(Limits::MAX_HEIGHT) + " (got " + value + ")";
                    return false;
                }
                if (height % 2 != 0) {
                    errorMsg = "camera_height must be even for H.264 (got " + value + ")";
                    return false;
                }
                Config::CAMERA_HEIGHT = height;
                return true;
            }

            // Camera FPS - critical for avoiding division by zero
            if (key == "camera_fps") {
                int fps = std::stoi(value);
                if (fps < Limits::MIN_FPS || fps > Limits::MAX_FPS) {
                    errorMsg = "camera_fps must be " + std::to_string(Limits::MIN_FPS) +
                              "-" + std::to_string(Limits::MAX_FPS) + " (got " + value + ")";
                    return false;
                }
                Config::CAMERA_FPS = fps;
                return true;
            }

            // Camera device
            if (key == "camera_device") {
                if (value.empty()) {
                    errorMsg = "camera_device cannot be empty";
                    return false;
                }
                if (value[0] != '/') {
                    errorMsg = "camera_device must be absolute path (got " + value + ")";
                    return false;
                }
                Config::CAMERA_DEVICE = value;
                return true;
            }

            // Initial bitrate
            if (key == "initial_bitrate") {
                int bitrate = std::stoi(value);
                if (bitrate < Limits::MIN_BITRATE || bitrate > Limits::MAX_BITRATE) {
                    errorMsg = "initial_bitrate must be " + std::to_string(Limits::MIN_BITRATE) +
                              "-" + std::to_string(Limits::MAX_BITRATE) + " (got " + value + ")";
                    return false;
                }
                Config::INITIAL_BITRATE = bitrate;
                return true;
            }

            // Min bitrate
            if (key == "min_bitrate") {
                int bitrate = std::stoi(value);
                if (bitrate < Limits::MIN_BITRATE || bitrate > Limits::MAX_BITRATE) {
                    errorMsg = "min_bitrate must be " + std::to_string(Limits::MIN_BITRATE) +
                              "-" + std::to_string(Limits::MAX_BITRATE) + " (got " + value + ")";
                    return false;
                }
                // Logical check: min should not exceed max
                if (bitrate > Config::MAX_BITRATE) {
                    errorMsg = "min_bitrate (" + value + ") > max_bitrate (" +
                              std::to_string(Config::MAX_BITRATE) + ")";
                    return false;
                }
                Config::MIN_BITRATE = bitrate;
                return true;
            }

            // Max bitrate
            if (key == "max_bitrate") {
                int bitrate = std::stoi(value);
                if (bitrate < Limits::MIN_BITRATE || bitrate > Limits::MAX_BITRATE) {
                    errorMsg = "max_bitrate must be " + std::to_string(Limits::MIN_BITRATE) +
                              "-" + std::to_string(Limits::MAX_BITRATE) + " (got " + value + ")";
                    return false;
                }
                // Logical check: max should not be less than min
                if (bitrate < Config::MIN_BITRATE) {
                    errorMsg = "max_bitrate (" + value + ") < min_bitrate (" +
                              std::to_string(Config::MIN_BITRATE) + ")";
                    return false;
                }
                Config::MAX_BITRATE = bitrate;
                return true;
            }

            // Encoder preset
            if (key == "encoder_preset") {
                // Valid x264 presets
                const std::string validPresets[] = {
                    "ultrafast", "superfast", "veryfast", "faster", "fast",
                    "medium", "slow", "slower", "veryslow"
                };

                bool valid = false;
                for (const auto& preset : validPresets) {
                    if (value == preset) {
                        valid = true;
                        break;
                    }
                }

                if (!valid) {
                    errorMsg = "encoder_preset must be: ultrafast, superfast, veryfast, faster, "
                              "fast, medium, slow, slower, or veryslow (got " + value + ")";
                    return false;
                }

                Config::ENCODER_PRESET = value;
                return true;
            }

            // Use NVENC
            if (key == "use_nvenc") {
                std::string lowerValue = value;
                std::transform(lowerValue.begin(), lowerValue.end(),
                             lowerValue.begin(), ::tolower);

                if (lowerValue == "true" || lowerValue == "1" || lowerValue == "yes") {
                    Config::USE_NVENC = true;
                } else if (lowerValue == "false" || lowerValue == "0" || lowerValue == "no") {
                    Config::USE_NVENC = false;
                } else {
                    errorMsg = "use_nvenc must be true/false, 1/0, or yes/no (got " + value + ")";
                    return false;
                }
                return true;
            }

            // UDP port
            if (key == "udp_port") {
                int port = std::stoi(value);
                if (port < Limits::MIN_PORT || port > Limits::MAX_PORT) {
                    errorMsg = "udp_port must be " + std::to_string(Limits::MIN_PORT) +
                              "-" + std::to_string(Limits::MAX_PORT) + " (got " + value + ")";
                    return false;
                }
                Config::UDP_PORT = port;
                return true;
            }

            // Capture queue size
            if (key == "capture_queue_size") {
                int size = std::stoi(value);
                if (size < Limits::MIN_QUEUE_SIZE || size > Limits::MAX_QUEUE_SIZE) {
                    errorMsg = "capture_queue_size must be " + std::to_string(Limits::MIN_QUEUE_SIZE) +
                              "-" + std::to_string(Limits::MAX_QUEUE_SIZE) + " (got " + value + ")";
                    return false;
                }
                Config::CAPTURE_QUEUE_SIZE = size;
                return true;
            }

            // Encode queue size
            if (key == "encode_queue_size") {
                int size = std::stoi(value);
                if (size < Limits::MIN_QUEUE_SIZE || size > Limits::MAX_QUEUE_SIZE) {
                    errorMsg = "encode_queue_size must be " + std::to_string(Limits::MIN_QUEUE_SIZE) +
                              "-" + std::to_string(Limits::MAX_QUEUE_SIZE) + " (got " + value + ")";
                    return false;
                }
                Config::ENCODE_QUEUE_SIZE = size;
                return true;
            }

            // Unknown setting
            errorMsg = "Unknown configuration key: " + key;
            return false;

        } catch (const std::invalid_argument& e) {
            errorMsg = "Invalid number format for " + key + ": " + value;
            return false;
        } catch (const std::out_of_range& e) {
            errorMsg = "Value out of range for " + key + ": " + value;
            return false;
        }
    }

    static void printConfig() {
        std::cout << "\n[Config] ====== Validated Configuration ======" << std::endl;
        std::cout << "[Config] Camera:" << std::endl;
        std::cout << "[Config]   Device: " << Config::CAMERA_DEVICE << std::endl;
        std::cout << "[Config]   Resolution: " << Config::CAMERA_WIDTH << "x"
                  << Config::CAMERA_HEIGHT << std::endl;
        std::cout << "[Config]   FPS: " << Config::CAMERA_FPS << std::endl;

        std::cout << "[Config] Encoding:" << std::endl;
        std::cout << "[Config]   Preset: " << Config::ENCODER_PRESET << std::endl;
        std::cout << "[Config]   NVENC: " << (Config::USE_NVENC ? "enabled" : "disabled") << std::endl;
        std::cout << "[Config]   Bitrate: " << (Config::MIN_BITRATE / 1000) << "-"
                  << (Config::MAX_BITRATE / 1000) << " kbps (initial: "
                  << (Config::INITIAL_BITRATE / 1000) << " kbps)" << std::endl;

        std::cout << "[Config] Network:" << std::endl;
        std::cout << "[Config]   UDP Port: " << Config::UDP_PORT << std::endl;

        std::cout << "[Config] Performance:" << std::endl;
        std::cout << "[Config]   Queue Sizes: " << Config::CAPTURE_QUEUE_SIZE
                  << " (capture), " << Config::ENCODE_QUEUE_SIZE << " (encode)" << std::endl;
        std::cout << "[Config] =========================================\n" << std::endl;
    }
};

#endif // CONFIG_LOADER_H
