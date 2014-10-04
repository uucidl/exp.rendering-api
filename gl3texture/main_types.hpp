#include "../ref/main_types.h"

#include <functional>
#include <future>
#include <vector>

static std::string dirname(std::string path)
{
        return path.substr(0, path.find_last_of("/\\"));
}

class Tasks : public DisplayThreadTasks
{
public:
        void add_task(std::function<bool()>&& task)
        {
                std::lock_guard<std::mutex> lock(tasks_mtx);
                tasks.emplace_back(task);
        }

        void run()
        {
                std::lock_guard<std::mutex> lock(tasks_mtx);
                for (auto& task : tasks) {
                        std::future<bool> future = task.get_future();
                        task();
                        future.get();
                }
                tasks.clear();
        }

private:
        std::mutex tasks_mtx;
        std::vector<std::packaged_task<bool()>> tasks;
};

class RootDirFileSystem : public FileSystem
{
public:
        RootDirFileSystem(std::string const& basePath) : base_path(basePath) {}

        std::ifstream open_file(std::string relpath) const
        {
                auto stream = std::ifstream(base_path + "/" + relpath);

                if (stream.fail()) {
                        throw std::runtime_error("could not load file at " + relpath);
                }

                return stream;
        }

private:
        std::string const base_path;
};
