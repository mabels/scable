
#include <iostream>
#include <array>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <list>
#include <queue>
#include <random>
#include <algorithm>
#include <chrono>
#include <ctime>

#include <openssl/evp.h>
#include <unistd.h>

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

class BenchMark {
  private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    std::chrono::time_point<std::chrono::high_resolution_clock> end;

  public:
    BenchMark() : start(std::chrono::high_resolution_clock::now()) {
    }

    void done() {
      end = std::chrono::high_resolution_clock::now();
    }
    double takes() const {
      std::chrono::duration<double> elapsed_seconds = end - start;
      return elapsed_seconds.count();
    }

};

class QueueItem {
  private:
    unsigned char data[2048];
    int size;
    unsigned char *refPtr;
    int refSize;
  public:
    typedef QueueItem *Ptr;
    unsigned char *getData() {
      return data;
    }
    int getDataSizeOf() {
      return sizeof(data);
    }
    int getSize() const {
      return size;
    }
    void setSize(int _size) {
      size = _size;
    }
    void setRefPtr(unsigned char *_refPtr, int _refSize) {
      refPtr = _refPtr;
      refSize = _refSize;
    }
    unsigned char *getRefPtr() {
      return refPtr;
    }
    int getRefSize() const {
      return refSize;
    }
};

class Result {
  public:
    class Data {
      private:
        BenchMark bench;
      public:
        long long totalSize;
        void done() {
          bench.done();
        }
        float takes() const {
          return bench.takes();
        }
    };
  private:
    CountDownLatch cdl;
    mutable std::mutex results_mutex;
    std::list<Data> results;
  public:
    BlockingQueue<QueueItem::Ptr> toProducer;
    BlockingQueue<QueueItem::Ptr> toConsumer;
    Result(int countDown) : cdl(countDown) {
    }
    void countDown(Data &result) {
      std::lock_guard<std::mutex> lock(results_mutex);
      results.push_back(result);
      cdl.countDown();
    }
    const std::list<Data>& wait() {
      cdl.wait();
      return results;
    }
};

class ActionPrepare {
  private:
    Result& result;
    unsigned char *data;
    const long long size;

    std::thread th;

  public:

    ActionPrepare(Result& _result, unsigned char *_data, long long _size) : result(_result), data(_data), size(_size) {
    }

    ActionPrepare(ActionPrepare &other) : result(other.result), data(other.data), size(other.size) {
    }
    ~ActionPrepare() {
      th.join();
    }

    ActionPrepare *start() {
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
      unsigned char *last = this->data;
      Result::Data bench;
      for(unsigned char *ptr = this->data; ptr < (this->data+this->size); ptr += md_len) {
        EVP_DigestInit(&md_ctx, md);
        EVP_DigestUpdate(&md_ctx, ptr, md_len); // fakey
        EVP_DigestFinal(&md_ctx, hash, &md_len);
        memcpy(ptr, hash, md_len);
      }
      bench.done();
      bench.totalSize = size;
      LOG(INFO) << "ActionPrepare::run:DONE=" << this << " size=" << size << " time=" << bench.takes() << "=> " << size/1024/1024/bench.takes() << "mb/sec";
      EVP_MD_CTX_cleanup(&md_ctx);
      result.countDown(bench);
    }
};

class Patterer {
  public:
    typedef struct {
      unsigned char *packet;
      int  size;
    } Pattern;

    static Pattern *create_pattern(int pattern, unsigned char *data, long long size) {
      Pattern *ret = new Pattern[pattern];
      std::random_device rd;
      std::default_random_engine e1(rd());
      std::uniform_real_distribution<> dis(0, 1);
      for(int i = 0; i < pattern; ++i) {
        float precent = dis(e1);
        ret[i].size = 13 + precent*1400;
        ret[i].packet = data+(long)(((precent*(size-ret[i].size-sizeof(long)))/sizeof(long))*sizeof(long));
      }
      return ret;
    }
};

class OpenSSL {
  private:
    std::string cipher_suite;
    const EVP_CIPHER *cipher;
    unsigned char *password;
    unsigned char *iv;
  public:
    OpenSSL() {
      OpenSSL_add_all_algorithms();
    }
    std::string& getCipherSuite() {
      return cipher_suite;
    }

    void setPassword(unsigned char *password) {
      this->password = password;
    }
    void setIv(unsigned char *iv) {
      this->iv = iv;
    }

    unsigned char* getPassword() {
      return password;
    }

    unsigned char* getIv() {
      return iv;
    }

    void setCipherSuite(std::string &name) {
      cipher = EVP_get_cipherbyname(name.c_str());
      if (!cipher) {
        LOG(ERROR) << "EVP_get_cipherbyname failed. Check your cipher suite string.";
      }
    }

    ~OpenSSL() {
      EVP_cleanup();
    }

    size_t chunked_size(size_t size) {
      return ((size/EVP_MAX_MD_SIZE)+1)*EVP_MAX_MD_SIZE;
    }
    const EVP_CIPHER *getCipher() const {
      return cipher;
    }
    const char *getCipherName() {
      return EVP_CIPHER_name(this->cipher);
    }
    class Context {
      protected: 
        EVP_CIPHER_CTX *ctx;
        OpenSSL &openssl;
      public:
        Context(OpenSSL &_openssl) : openssl(_openssl) {
          ctx = EVP_CIPHER_CTX_new();
        }
        ~Context() {
          EVP_CIPHER_CTX_free(ctx);
        }
        const char *getCipherName() {
          return openssl.getCipherName();
        }
    };
    class Encryptor : public Context {
      public: 
        Encryptor(OpenSSL &openssl) : Context(openssl) {
          EVP_EncryptInit(ctx, openssl.getCipher(), openssl.getPassword(), openssl.getIv());
        }
        size_t encrypt(unsigned char *in, int inSize, unsigned char *out, size_t outSize) {
          int len;
          EVP_EncryptUpdate(ctx, out, &len, in, inSize);
          int result_len = len;
          EVP_EncryptFinal(ctx, out + len, &len);
          return result_len + len;
        }
    };
    class Decryptor : public Context {
      public:
        Decryptor(OpenSSL &openssl) : Context(openssl) {
          EVP_DecryptInit(ctx, openssl.getCipher(), openssl.getPassword(), openssl.getIv());
        }
        size_t decrypt(unsigned char *in, int inSize, unsigned char *out, size_t outSize) {
          int len;
          EVP_DecryptUpdate(ctx, out, &len, in, inSize);
          int result_len = len;
          EVP_DecryptFinal(ctx, out + len, &len);
          return result_len + len;
        }
    };
      
};

class ActionEncrypt {
  private:
    Result& result;
    unsigned char *data;
    const long long size;
    const int pattern;
    OpenSSL &openssl;

    std::thread th;

  public:

    ActionEncrypt(Result& _result, int _pattern, unsigned char *_data, long long _size, OpenSSL &_openssl):
      result(_result), pattern(_pattern), data(_data), size(_size), openssl(_openssl) { }

    ActionEncrypt(ActionEncrypt &other) :
      result(other.result), pattern(other.pattern), data(other.data), size(other.size), openssl(other.openssl) { }
    ~ActionEncrypt() {
      th.join();
    }

    ActionEncrypt *start() {
      th = std::thread(&ActionEncrypt::run, this);
      return this;
    }

    void run() {
      std::unique_ptr<Patterer::Pattern> patterns(Patterer::create_pattern(pattern, data, size));
      OpenSSL::Encryptor ctx(openssl);
      unsigned char ciphertext[8192];

      LOG(INFO) << "ActionEncrypt::run:cipher=" << openssl.getCipherName();
      Result::Data bench;
      long long totalSize = 0;
      for(int i = 0; i < pattern; ++i) {
        const Patterer::Pattern *pat = patterns.get()+i;
        if (((1+i)%(size/10)) == 0) {
          LOG(INFO) << "ActionEncrypt::run:=" << this << " " << i << " of " << pattern;
        }
        ctx.encrypt(pat->packet, pat->size, ciphertext, sizeof(ciphertext));
        totalSize += pat->size;
      }
      bench.done();
      bench.totalSize = totalSize;
      LOG(INFO) << "ActionEncrypt::run:DONE=" << this << " size=" << totalSize << 
        " time=" << bench.takes() << "=> " << totalSize/1024/1024/bench.takes() << "mb/sec";
      result.countDown(bench);
    }
};

class ActionDecrypt {
  private:
    Result& result;
    unsigned char *data;
    const long long size;
    const int pattern;
    OpenSSL &openssl;

    std::thread th;

  public:

    ActionDecrypt(Result& _result, int _pattern, unsigned char *_data, long long _size, OpenSSL &_openssl) :
      result(_result), pattern(_pattern), data(_data), size(_size), openssl(_openssl) {
      }

    ActionDecrypt(ActionDecrypt &other) :
      result(other.result), pattern(other.pattern), data(other.data), size(other.size), openssl(other.openssl) {
      }
    ~ActionDecrypt() {
      th.join();
    }

    ActionDecrypt *start() {
      th = std::thread(&ActionDecrypt::run, this);
      return this;
    }

    void run() {
      std::unique_ptr<Patterer::Pattern> patterns(Patterer::create_pattern(pattern, data, size));
      OpenSSL::Decryptor ctx(openssl);
      unsigned char ciphertext[8192];

      LOG(INFO) << "ActionDecrypt::run:cipher=" << ctx.getCipherName();
      Result::Data bench;
      long long totalSize = 0;
      for(int i = 0; i < pattern; ++i) {
        const Patterer::Pattern *pat = patterns.get()+i;
        if (((1+i)%(size/10)) == 0) {
          LOG(INFO) << "ActionDecrypt::run:=" << this << " " << i+1 << " of " << pattern;
        }
        ctx.decrypt(pat->packet, pat->size, ciphertext, sizeof(ciphertext));
        totalSize += pat->size;
      }
      bench.done();
      bench.totalSize = totalSize;
      LOG(INFO) << "ActionDecrypt::run:DONE=" << this << " size=" << totalSize << 
        " time=" << bench.takes() << "=> " << totalSize/1024/1024/bench.takes() << "mb/sec";
      result.countDown(bench);
    }
};


class ActionDecryptConsumer {
  private:
    Result& result;
    unsigned char *data;
    const long long size;
    const int pattern;
    std::thread th;
    mutable bool request_stop;
    CountDownLatch &cdl;
    OpenSSL &openssl;

  public:

    ActionDecryptConsumer(CountDownLatch &_cdl, Result& _result, int _pattern, unsigned char *_data, long long _size, OpenSSL &_openssl) :
      cdl(_cdl), result(_result), pattern(_pattern), data(_data), size(_size), request_stop(false), openssl(_openssl) {
      }

    ActionDecryptConsumer(ActionDecryptConsumer &other) :
      cdl(other.cdl), result(other.result), pattern(other.pattern), data(other.data), size(other.size),
      request_stop(other.request_stop), openssl(other.openssl) {
      }

    ~ActionDecryptConsumer() {
      th.join();
    }

    ActionDecryptConsumer *start() {
      th = std::thread(&ActionDecryptConsumer::run, this);
      return this;
    }

    void run() {
      OpenSSL::Decryptor ctx(openssl);
      LOG(INFO) << "ActionDecryptConsumer::run:cipher=" << openssl.getCipherName() << ":" << pattern;
      Result::Data bench;
      long long totalSize = 0;
      unsigned char ciphertext[8192];
      bench.totalSize = 0;
      std::chrono::milliseconds duration(500);
      for(int cnt = 0; true; ++cnt) {
        QueueItem::Ptr qi;
        bool ret = result.toConsumer.pop_wait_for(duration, &qi);
        if (request_stop) {
          break;
        }
        if (!ret) {
          continue;
        }
        if ((1+cnt)%(pattern/10) == 0) {
          LOG(INFO) << "ActionDecryptConsumer::run:=" << this << " " << cnt+1 << " of " << pattern;
        }
        int result_len = ctx.decrypt(qi->getData(), qi->getSize(), ciphertext, sizeof(ciphertext));
        if (qi->getRefSize() > result_len) {
          LOG(ERROR) << "ActionDecryptConsumer::error:size:" << cnt << ":" << qi->getRefSize() << "==" << result_len;
        }/* else if (memcmp(qi->getRefPtr(), ciphertext, qi->getRefSize())) {
          LOG(ERROR) << "ActionDecryptConsumer::error:cmp:" << cnt << ":" << (void*)qi->getRefPtr() << "==" << (void*)ciphertext;
        }*/
        bench.totalSize += result_len;
        result.toProducer.push(qi);
      }
      bench.done();
      LOG(INFO) << "ActionDecryptConsumer::run:DONE=" << this << " size=" << bench.totalSize
        << " time=" << bench.takes() << "=> " << bench.totalSize/1024/1024/bench.takes() << "mb/sec";
      cdl.countDown();
      result.countDown(bench);
    }

    void stop() {
      LOG(INFO) << "ActionDecryptConsumer::stop";
      request_stop = true;
    }

};

class ActionEncryptProducer {
  private:
    Result& result;
    unsigned char *data;
    const long long size;
    const int pattern;
    CountDownLatch &cdl;
    std::thread th;
    OpenSSL &openssl;

  public:

    ActionEncryptProducer(CountDownLatch &_cdl, Result& _result,
        int _pattern, unsigned char *_data, long long _size, OpenSSL &_openssl) :
      cdl(_cdl), result(_result), pattern(_pattern), data(_data), size(_size), openssl(_openssl) {
      }

    ActionEncryptProducer(ActionEncryptProducer &other) :
      cdl(other.cdl), result(other.result), pattern(other.pattern), data(other.data), 
      size(other.size), openssl(other.openssl) {
      }
    ~ActionEncryptProducer() {
      th.join();
    }

    ActionEncryptProducer *start() {
      th = std::thread(&ActionEncryptProducer::run, this);
      return this;
    }

    void run() {
      std::unique_ptr<Patterer::Pattern> patterns(Patterer::create_pattern(pattern, data, size));
      OpenSSL::Encryptor ctx(openssl);
      LOG(INFO) << "ActionEncryptProducer::run:cipher=" << openssl.getCipherName() << ":" << pattern;
      Result::Data bench;
      bench.totalSize = 0;
      for(int i = 0; i < pattern; ++i) {
        const Patterer::Pattern *pat = patterns.get()+i;
        QueueItem::Ptr item = result.toProducer.pop();
        if (((1+i)%(pattern/10)) == 0) {
          LOG(INFO) << "ActionEncryptProducer::run:=" << this << " " << i << " of " << pattern;
        }
        item->setSize(ctx.encrypt(pat->packet, pat->size, item->getData(), item->getDataSizeOf()));
        item->setRefPtr(pat->packet, pat->size);
        //LOG(INFO) << "send to consumer:" << item << ":" << item->getSize() << ":" << (void*)item;
        result.toConsumer.push(item);
        bench.totalSize += pat->size;
      }
      bench.done();
      LOG(INFO) << "ActionEncryptProducer::run:DONE=" << this << " size=" << bench.totalSize
        << " time=" << bench.takes() << "=> " << bench.totalSize/1024/1024/bench.takes() << "mb/sec";
      cdl.countDown();
      result.countDown(bench);
    }
};


class TestEnDecryption {
  private:
    unsigned char *data;
    long long size;
    long long workers;
    int pattern;
    OpenSSL &openssl;

  public:
    TestEnDecryption(OpenSSL &_openssl) : openssl(_openssl) {
    }

    ~TestEnDecryption() {
      if (this->data) {
        delete this->data;
        this->data = NULL;
      }
    }
    static void wait(Result &result) {
      const std::list<Result::Data> &list = result.wait();
      long long size = 0;
      float time = 0;
      for(std::list<Result::Data>::const_iterator i = list.begin(); i != list.end(); ++i) {
        size += i->totalSize;
        time += i->takes();
      }
      LOG(INFO) << "total result time=" << time << "sec totalsize=" << size/1024/1024
        << "mb " << (size/1024/1024/time)*list.size() << "mb/sec";
    }
    void setup(long long size, int _workers) {
      LOG(INFO) << "Size=" << size/1024/1024 << "mb workers=" << _workers;

      this->workers = _workers;
      this->size = openssl.chunked_size(size);
      this->data = new unsigned char[this->size];
      Result result(this->workers);
      std::list<std::unique_ptr<ActionPrepare>> workers;
      for (long long i = 0; i < this->workers; ++i) {
        long long size = std::min((i+1)*(this->size/this->workers), size);
        ActionPrepare *ap = (new ActionPrepare(result, this->data+(i*(this->size/this->workers)), size))->start();
        workers.push_back(std::unique_ptr<ActionPrepare>(ap));
      }
      LOG(INFO) << "Waiting" ;
      openssl.setPassword(data);
      openssl.setIv(data);
      TestEnDecryption::wait(result);
    }

    void encrypt(int pattern) {
      LOG(INFO) << "Pattern=" << pattern << " workers=" << this->workers;
      Result result(this->workers);
      std::list<std::unique_ptr<ActionEncrypt>> workers;
      for (long long i = 0; i < this->workers; ++i) {
        ActionEncrypt *ae = (new ActionEncrypt(result, pattern, data, size, openssl))->start();
        workers.push_back(std::unique_ptr<ActionEncrypt>(ae));
      }
      LOG(INFO) << "Waiting" ;
      TestEnDecryption::wait(result);
    }

    void decrypt(int pattern) {
      LOG(INFO) << "Pattern=" << pattern << " workers=" << this->workers;
      Result result(this->workers);
      std::list<std::unique_ptr<ActionDecrypt>> workers;
      for (long long i = 0; i < this->workers; ++i) {
        ActionDecrypt *ae = (new ActionDecrypt(result, pattern, data, size, openssl))->start();
        workers.push_back(std::unique_ptr<ActionDecrypt>(ae));
      }
      LOG(INFO) << "Waiting" ;
      TestEnDecryption::wait(result);
    }

    void endecrypt(int pattern) {
      const int my_workers = workers / 2;
      LOG(INFO) << "Pattern=" << pattern << " workers=" << my_workers;
      Result result(workers);
      std::array<QueueItem, 128> queueItems;
      for (int i = 0; i < queueItems.size(); ++i) {
        QueueItem::Ptr my = &queueItems[i];
        result.toProducer.push(my);
      }
      // Start Consumer
      typedef std::list<std::unique_ptr<ActionDecryptConsumer>> ADCWorkers;
      ADCWorkers lst_adcworkers;
      CountDownLatch adc_cdl(my_workers);
      for (int i = 0; i < my_workers; ++i) {
        ActionDecryptConsumer *adc = (new ActionDecryptConsumer(adc_cdl, result, pattern, data, size, openssl))->start();
        lst_adcworkers.push_back(std::unique_ptr<ActionDecryptConsumer>(adc));
      }
      typedef std::list<std::unique_ptr<ActionEncryptProducer>> AECWorkers;
      AECWorkers lst_aecworkers;
      CountDownLatch aec_cdl(my_workers);
      for (int i = 0; i < my_workers; ++i) {
        ActionEncryptProducer *aec = (new ActionEncryptProducer(aec_cdl, result, pattern, data, size, openssl))->start();
        lst_aecworkers.push_back(std::unique_ptr<ActionEncryptProducer>(aec));
      }
      LOG(INFO) << "AEC Waiting";
      aec_cdl.wait();
      LOG(INFO) << "AEC Waiting Completed:" << lst_adcworkers.size();
      for (ADCWorkers::iterator i = lst_adcworkers.begin(); i != lst_adcworkers.end(); ++i) {
        (*i)->stop();
      }
      LOG(INFO) << "ADC Waiting";
      adc_cdl.wait();
      LOG(INFO) << "ADC Waiting Completed";
      TestEnDecryption::wait(result);
    }

};

INITIALIZE_EASYLOGGINGPP
int main(int argc, char **argv) {
  START_EASYLOGGINGPP(argc, argv);

  OpenSSL openssl;
  TestEnDecryption ted(openssl);
  long long memory = 1024 * 1024 * 128;
  int workers = 2;
  int pattern = 100000;
  std::string cipher_suite("aes-128-cbc");
  if (argc >= 2) {
    std::stringstream(argv[1]) >> memory;
    memory *= 1024 * 1024;
    if (memory < 1024*1024) {
      memory = 1024 * 1024;
    }
  }
  if (argc >= 3) {
    std::stringstream(argv[2]) >> workers;
  }
  if (argc >= 4) {
    std::stringstream(argv[3]) >> pattern;
  }
  if (argc >= 5) {
    std::stringstream(argv[4]) >> cipher_suite;
  }
  openssl.setCipherSuite(cipher_suite);
  ted.setup(memory, (workers/2)*2);
  ted.encrypt(pattern);
  ted.decrypt(pattern);
  ted.endecrypt(pattern);
}
