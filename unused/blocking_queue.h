
#include <mutex>
#include <condition_variable>
#include <queue>

template<typename T> class BlockingQueue {
  private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    mutable std::condition_variable cond_;
  public:

    T pop() {
      std::unique_lock<std::mutex> mlock(mutex_);
      while (queue_.empty()) {
        cond_.wait(mlock);
      }
      auto item = queue_.front();
      queue_.pop();
      return item;
    }

    template <class R, class P>
      bool pop_wait_for(const std::chrono::duration<R, P> &duration, T *item) {
        std::unique_lock<std::mutex> mlock(mutex_);
        while (queue_.empty()) {
          if (cond_.wait_for(mlock, duration) != std::cv_status::no_timeout) {
            return false;
          }
        }
        *item = queue_.front();
        queue_.pop();
        return true;
      }

    void pop(T& item) {
      std::unique_lock<std::mutex> mlock(mutex_);
      while (queue_.empty()) {
        cond_.wait(mlock);
      }
      item = queue_.front();
      queue_.pop();
    }

    void push(const T& item)
    {
      std::unique_lock<std::mutex> mlock(mutex_);
      queue_.push(item);
      mlock.unlock();
      cond_.notify_one();
    }
};
