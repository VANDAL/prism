#include "DbiFrontend.hpp"
#include <csignal>
#include <cstdlib>
#include <dirent.h>
#include <iostream>

namespace Cleanup
{

namespace
{

std::mutex cleanupMtx;
bool initialized;
std::string cleanupDir;
std::terminate_handler prev_terminate_handler;

using signal_handler = void(*)(int);
signal_handler prev_sigint_handler;
signal_handler prev_sigsegv_handler;

auto cleanupHandler() -> void
{
    if (cleanupDir.empty() == false)
    {
        DIR *d = opendir(cleanupDir.c_str());
        if (d != nullptr)
        {
            dirent *dir = readdir(d);
            char fullPath[256];
            while (dir != nullptr)
            {
                /* TODO check path length */
                sprintf(fullPath, "%s/%s", cleanupDir.c_str(), dir->d_name);
                remove(fullPath);
                dir = readdir(d);
            }
            int ret = remove(cleanupDir.c_str());
            if (ret < 0)
                std::cerr << "error removing " + cleanupDir + " -- " + strerror(errno) << std::endl;
        }
    }
}

};

auto setCleanupDir(std::string dir) -> void
{
    std::lock_guard<std::mutex> lock(cleanupMtx);

    if (initialized == false)
    {
        cleanupDir = dir;

        /* Cleanup at normal exit */
        std::atexit(cleanupHandler);

        /* or SIGINT/SIGSEGV */
        auto dummy = [](int){};
        prev_sigint_handler = std::signal(SIGINT, dummy);
        prev_sigsegv_handler = std::signal(SIGSEGV, dummy);
        std::signal(SIGINT, [](int signum){ cleanupHandler(); prev_sigint_handler(signum); });
        std::signal(SIGSEGV, [](int signum){ cleanupHandler(); prev_sigsegv_handler(signum); });

        /* or unhandled exception */
#if __GNUC__ < 5
        prev_terminate_handler = std::abort;
#else
        prev_terminate_handler = std::get_terminate();
#endif
        std::set_terminate([]{ cleanupHandler(); prev_terminate_handler(); });

        initialized = true;
    }
}

}; //end namespace Cleanup
