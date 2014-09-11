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
                file_system(fs),
                is_quitting(false) {}

        ~FileLoader()
        {
                is_quitting = true;
        }

        void loadFiles(std::string path1,
                       std::string path2,
                       std::function<void(std::string const&, std::string const&)> continuation)
        {
                auto future_pair = std::async(std::launch::async, [=]() {
                        try {
                                auto vs = file_system.open_file(path1);
                                auto fs = file_system.open_file(path2);
                                std::string vs_content {
                                        std::istreambuf_iterator<char>(vs),
                                        std::istreambuf_iterator<char>()
                                };
                                std::string fs_content {
                                        std::istreambuf_iterator<char>(fs),
                                        std::istreambuf_iterator<char>()
                                };

                                display_tasks.add_task([=] () {
                                        continuation(vs_content, fs_content);
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
        std::atomic<bool> is_quitting;
        std::mutex futures_mtx;
        std::vector<std::future<void>> futures;
};

FileLoaderResource makeFileLoader(FileSystem& fs,
                                  DisplayThreadTasks& display_tasks)
{
        return estd::make_unique<FileLoader>(display_tasks, fs);
}

void loadFilePair(
        FileLoader& loader,
        std::string path1,
        std::string path2,
        std::function<void(std::string const&, std::string const&)> continuation)
{
        loader.loadFiles(path1, path2, continuation);
}
