#ifndef PRISM_FRONTEND_COMMON_H
#define PRISM_FRONTEND_COMMON_H

#include <mutex>
#include <condition_variable>

/* Common Utility classes */ 


class Sem
{
  public:
    Sem(int init) : val(init) {}

    auto P()
    {
        std::unique_lock<std::mutex> lock(mtx);
        cond.wait(lock, [&] { return val > 0; });
        --val;
    }

    auto V()
    {
        std::unique_lock<std::mutex> lock(mtx);
        ++val;
        cond.notify_one();
    }

    auto value()
    {
        std::unique_lock<std::mutex> lock(mtx);
        return val;
    }

  private:
    std::mutex mtx;
    std::condition_variable cond;
    int val;
};


template<typename T, unsigned N>
struct CircularQueue
{
    static_assert((N >= 2) && ((N & (N - 1)) == 0), "N must be a power of 2");
  public:
    auto enqueue(T val)
    {
        q[tail] = val;
        tail = (tail+1) & (N-1);
    }

    auto dequeue()
    {
        auto temp = head;
        head = (head+1) & (N-1);
        return q[temp];
    }

    T q[N];
    size_t head{0}, tail{0};
};

#endif
