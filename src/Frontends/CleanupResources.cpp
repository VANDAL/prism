#include <csignal>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <iostream>
#include <mutex>

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

auto cleanupHandler()
{
    if (cleanupDir.empty() == false)
    {
        DIR *d = opendir(cleanupDir.c_str());
        if (d != nullptr)
        {
            char fullPath[256];
            const char* rootdir_str = cleanupDir.c_str();
            size_t rootdir_len = strlen(rootdir_str);
            for(dirent *dir = readdir(d); dir != nullptr; dir = readdir(d))
            {
                const char* dir_str = dir->d_name;
                size_t dir_len = strlen(dir_str);

                if ( (strcmp(dir->d_name, ".") == 0) || (strcmp(dir->d_name, "..") == 0) || ((rootdir_len + dir_len + 2) > 256) )
                {
                    // Don't delete relative paths.
                    // Not much to do if our buffer is too small...
                    // ...but that should be an improbably rare event
                    continue;
                }

                memset(fullPath, 0, 256);
                strncpy(fullPath, rootdir_str, rootdir_len);
                fullPath[rootdir_len] = '/';
                strncat(fullPath, dir_str, dir_len);
                if (int ret = remove(fullPath); ret < 0)
                    std::cerr << "error removing " << rootdir_str << " -- " << strerror(errno) << std::endl;
            }
            if (int ret = remove(rootdir_str); ret < 0)
                std::cerr << "error removing " << rootdir_str << " -- " << strerror(errno) << std::endl;
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
