class CountDownLatch {
  private:
    mutable std::mutex count_mutex;
    int count;
    mutable std::mutex wait_all;

  public:
    CountDownLatch(int _count) : count(_count) {
      wait_all.lock();
    }

    void wait() {
      std::lock_guard<std::mutex> lock(wait_all);
    }

    void countDown() {
      std::lock_guard<std::mutex> lock(count_mutex);
      --count;
      if (count == 0) {
        wait_all.unlock();
      }
    }

    int getCount() const {
      std::lock_guard<std::mutex> lock(count_mutex);
      return count;
    }

};
