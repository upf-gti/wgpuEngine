#pragma once

#include <filesystem>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>
#include <functional>

// Based on: https://solarianprogrammer.com/2019/01/13/cpp-17-filesystem-write-file-watcher-monitor/

// Define available file changes
enum class eFileStatus { Created, Modified, Erased};

class FileWatcher {
public:
    std::string path_to_watch;

    // Time interval at which we check the base folder for changes
    float delay = 0.0f;

    // Time until next check
    float counter = 0.0f;

    const std::function<void(std::string, eFileStatus)> callback;

    // Keep a record of files from the base directory and their last modification time
    FileWatcher(std::string path_to_watch, float delay, const std::function<void(std::string, eFileStatus)>& callback) : path_to_watch{ path_to_watch }, delay{ delay }, counter{ delay }, callback(callback) {
        for(auto &file : std::filesystem::recursive_directory_iterator(path_to_watch)) {
            paths[file.path().string()] = std::filesystem::last_write_time(file);
        }
    }

    // Monitor "path_to_watch" for changes and in case of a change execute the user supplied "action" function
    void update(float delta_time) {

        if (paths.empty()) return;

        // Wait for "delay"
        counter -= delta_time;

        if (counter > 0.0f) return;

        auto it = paths.begin();
        while (it != paths.end()) {
            if (!std::filesystem::exists(it->first)) {
                callback(it->first, eFileStatus::Erased);
                it = paths.erase(it);
            }
            else {
                it++;
            }                    
        }

        // Check if a file was Created or Modified
        for(auto &file : std::filesystem::recursive_directory_iterator(path_to_watch)) {
            auto current_file_last_write_time = std::filesystem::last_write_time(file);

            // File creation
            if(!contains(file.path().string())) {
                paths[file.path().string()] = current_file_last_write_time;
                callback(file.path().string(), eFileStatus::Created);
            // File modification
            } else {
                if(paths[file.path().string()] != current_file_last_write_time) {
                    paths[file.path().string()] = current_file_last_write_time;
                    callback(file.path().string(), eFileStatus::Modified);
                }
            }
        }

        counter = delay;
        
    }
private:
    std::unordered_map<std::string, std::filesystem::file_time_type> paths;

    // Check if "paths" contains a given key
    // If your compiler supports C++20 use paths.contains(key) instead of this function
    bool contains(const std::string &key) {
        auto el = paths.find(key);
        return el != paths.end();
    }
};
