
#include <iostream>
#include <mutex>
#include <thread>
#include <list>
#include <algorithm>

#include <openssl/evp.h>

#define ELPP_THREAD_SAFE
#include "easylogging++.h"


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

class ActionPrepare {
  private:
    CountDownLatch& cdl;
    char *data;
    const long long size;

    std::thread th;

  public:

    ActionPrepare(CountDownLatch& _cdl, char *_data, long long _size) : cdl(_cdl), data(_data), size(_size) {
      LOG(INFO) << "ActionPrepare::ActionPrepare=" << this ;
    }

    ActionPrepare(ActionPrepare &other) : cdl(other.cdl), data(other.data), size(other.size) {
      LOG(INFO) << "ActionPrepare::ActionPrepare=" << this ;
    }
    ~ActionPrepare() {
      LOG(INFO) << "ActionPrepare::~ActionPrepare=" << this ;
      th.join();
    }

    ActionPrepare *start() {
      LOG(INFO) << "ActionPrepare::run-1=" << this ;
      th = std::thread(&ActionPrepare::run, this);
      return this;
    }

    void run() {
      LOG(INFO) << "ActionPrepare::run:START=" << this << " data=" << (long long)this->data;
      EVP_MD_CTX md_ctx;
      EVP_MD_CTX_init(&md_ctx);
      const EVP_MD *md = EVP_sha256();

      unsigned char hash[EVP_MAX_MD_SIZE];
      unsigned int md_len = sizeof(hash);
      char *last = this->data;
      for(char *ptr = this->data; ptr < (this->data+this->size); ptr += md_len) {
        if ((ptr - last) > 10000000) {
          LOG(INFO) << "ActionPrepare::run:ptr=" << this << " data=" << (long long)ptr ;
          last = ptr;
        }
        EVP_DigestInit(&md_ctx, md);
        EVP_DigestUpdate(&md_ctx, ptr, md_len); // fakey
        EVP_DigestFinal(&md_ctx, hash, &md_len);
        memcpy(ptr, hash, md_len);
      }
      EVP_MD_CTX_cleanup(&md_ctx);
      cdl.countDown();
      LOG(INFO) << "ActionPrepare::run:DONE=" << this ;
    }
};

/*
static void delegate(ActionPrepare *processor) {
      processor->start();
}

template<class T> class ProcessorThread {
  private:
    std::unique_ptr<T> processor;
    std::unique_ptr<std::thread> thread;
  public:
    ProcessorThread(T *_processor) : processor(_processor),
      thread(new std::thread(&ProcessorThread<T>::delegate, _processor)) {
    }
    ProcessorThread(ProcessorThread& other) : processor(other.processor) , thread(other.thread) {
    }
    std::thread& getThread() {
      return *thread;
    }
    void delegate(T *processor) {
      processor->start();
    }


};

*/


class TestEnDecryption {
  private:
    char *data;
    long long size;
    long long workers;
  public:
    ~TestEnDecryption() {
      if (this->data) {
        delete this->data;
      }
    }
    void setup(long long size, int _workers) {
      LOG(INFO) << "Size=" << size << " workers=" << _workers ;
      this->workers = _workers;
      this->size = ((size/EVP_MAX_MD_SIZE)+1)*EVP_MAX_MD_SIZE;
      this->data = new char[this->size];
      CountDownLatch cdl(this->workers);
      std::list<std::unique_ptr<ActionPrepare>> workers;
      for (long long i = 0; i < this->workers; ++i) {
        long long size = std::min((i+1)*(this->size/this->workers), size);
        ActionPrepare *ap = (new ActionPrepare(cdl, this->data+(i*(this->size/this->workers)), size))->start();
        LOG(INFO) << "ActionPrepare:1=" << ap ;
        workers.push_back(std::unique_ptr<ActionPrepare>(ap));
      }
      LOG(INFO) << "Waiting" ;
      cdl.wait();
    }
};

INITIALIZE_EASYLOGGINGPP
int main(int argc, char **argv) {
  START_EASYLOGGINGPP(argc, argv);

  TestEnDecryption ted;

  ted.setup(1024*1024*1024, 8);
}
