#include "file_system_manager.h"

#include "spdlog/spdlog.h"

Error FileSystemManager::initialize()
{
    singleton_instance = this;

    resource_path = WGPUENGINE_ASSET_FOLDER;
    if (resource_path.empty()) {
        resource_path = WGPUENGINE_FOLDER;
    }

    resource_watcher.addWatch(resource_path, resource_watcher_callback, true);

    return Error::OK;
}

Error FileSystemManager::finalize()
{
    singleton_instance = nullptr;
    return Error::OK;
}

void FileSystemManager::process()
{
    resource_watcher.update();
    custom_folder_watchers.update();
}

void FileSystemManager::add_folder_watcher(const std::string& folder, FW::FileWatchListener callback, bool recursive)
{
    custom_folder_watchers.addWatch(folder, callback, recursive);
}

void FileSystemManager::resource_watcher_callback(FW::WatchID id, const FW::String& dir, const FW::String& filename, FW::Action action)
{
    spdlog::info("Resource changed: {}/{}", dir, filename);
}
