#pragma once

#include <functional>
#include <string>
#include <fstream>

class DisplayThreadTasks
{
public:
        virtual void add_task(std::function<bool()>&& task) = 0;
};

class FileSystem
{
public:
        virtual std::ifstream open_file(std::string relpath) const = 0;
};

class FileLoader;
using FileLoaderResource =
        std::unique_ptr<FileLoader, std::function<void(FileLoader*)>>;

FileLoaderResource makeFileLoader(FileSystem& fs,
                                  DisplayThreadTasks& display_tasks);

void loadFilePair(
        FileLoader& loader,
        std::string path1,
        std::string path2,
        std::function<void(std::string const&, std::string const&)> continuation);
