#include "main_types.h"

#include "../src/estd.hpp"

#include <future>
#include <sstream>
#include <string>
#include <vector>

class FileLoader
{
public:
        FileLoader(DisplayThreadTasks& display_tasks, FileSystem& fs) :
                display_tasks(display_tasks),
                file_system(fs) {}

        void loadFile(std::string path,
                      std::function<void(std::string const&)> continuation)
        {
                auto future_pair = std::async(std::launch::async, [=]() {
                        try {
                                auto stream = file_system.open_file(path);
                                std::string content {
                                        std::istreambuf_iterator<char>(stream),
                                        std::istreambuf_iterator<char>()
                                };

                                display_tasks.add_task([=] () {
                                        continuation(content);
                                        return true;
                                });
                        } catch (std::exception& e) {
                                // pass any exception to display thread so it can be treated
                                display_tasks.add_task([=] () -> bool {
                                        throw e;
                                });
                        }

                });

                std::lock_guard<std::mutex> lock(futures_mtx);
                futures.push_back(std::move(future_pair));
        }

        void loadFiles(std::string path1,
                       std::string path2,
                       std::function<void(std::string const&, std::string const&)> continuation)
        {
                auto future_pair = std::async(std::launch::async, [=]() {
                        try {
                                auto stream1 = file_system.open_file(path1);
                                auto stream2 = file_system.open_file(path2);
                                std::string content1 {
                                        std::istreambuf_iterator<char>(stream1),
                                        std::istreambuf_iterator<char>()
                                };
                                std::string content2 {
                                        std::istreambuf_iterator<char>(stream2),
                                        std::istreambuf_iterator<char>()
                                };

                                display_tasks.add_task([=] () {
                                        continuation(content1, content2);
                                        return true;
                                });
                        } catch (std::exception& e) {
                                // pass any exception to display thread so it can be treated
                                display_tasks.add_task([=] () -> bool {
                                        throw e;
                                });
                        }

                });

                std::lock_guard<std::mutex> lock(futures_mtx);
                futures.push_back(std::move(future_pair));
        }

private:
        DisplayThreadTasks& display_tasks;
        FileSystem& file_system;
        std::mutex futures_mtx;
        std::vector<std::future<void>> futures;
};

FileLoaderResource makeFileLoader(FileSystem& fs,
                                  DisplayThreadTasks& display_tasks)
{
        return estd::make_unique<FileLoader>(display_tasks, fs);
}

void loadFile(
        FileLoader& loader,
        std::string path,
        std::function<void(std::string const&)> continuation)
{
        loader.loadFile(path, continuation);
}

void loadFilePair(
        FileLoader& loader,
        std::string path1,
        std::string path2,
        std::function<void(std::string const&, std::string const&)> continuation)
{
        loader.loadFiles(path1, path2, continuation);
}
