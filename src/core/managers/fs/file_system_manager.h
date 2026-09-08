#pragma once

#include "core/managers/manager.h"

#include "FileWatcher/FileWatcher.h"

#include <string>
#include <unordered_map>

class FileSystemManager : public Manager {
    MANAGER_DECLARE(FileSystemManager)

public:
    std::string get_resource_path() const { return resource_path; }

    void add_folder_watcher(const std::string& folder, FW::FileWatchListener callback, bool recursive = false);

private:
    FileSystemManager() = default;
    ~FileSystemManager() = default;

    Error initialize() override;
    Error finalize() override;

    void process();

    static void resource_watcher_callback(FW::WatchID, const FW::String&, const FW::String&, FW::Action);

    std::string resource_path = "";
    //std::string user_path = "";
    FW::FileWatcher resource_watcher;
    FW::FileWatcher custom_folder_watchers;
};
