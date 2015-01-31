
#include <iostream>
#include <mutex>
#include <thread>
#include <list>
#include <random>
#include <algorithm>

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

class BenchMark {
  private:
    struct timeval start;
    struct timeval end;

  public:
    BenchMark() {
      gettimeofday(&start, 0);
    }

    void done() {
      gettimeofday(&end, 0);
    }
    float takes() const {
      struct timeval result;
      timeval_subtract (&result, end, start);
      return result.tv_sec + (result.tv_usec/1000000.0);
    }

    static int timeval_subtract (struct timeval *result, const struct timeval &x, const struct timeval &_y) {
      /* Perform the carry for the later subtraction by updating y. */
      struct timeval y = _y;
      if (x.tv_usec < y.tv_usec) {
        int nsec = (y.tv_usec - x.tv_usec) / 1000000 + 1;
        y.tv_usec -= 1000000 * nsec;
        y.tv_sec += nsec;
      }
      if (x.tv_usec - y.tv_usec > 1000000) {
        int nsec = (x.tv_usec - y.tv_usec) / 1000000;
        y.tv_usec += 1000000 * nsec;
        y.tv_sec -= nsec;
      }

      /* Compute the time remaining to wait.
         tv_usec is certainly positive. */
      result->tv_sec = x.tv_sec - y.tv_sec;
      result->tv_usec = x.tv_usec - y.tv_usec;

      /* Return 1 if result is negative. */
      return x.tv_sec < y.tv_sec;
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

class ActionEncrypt {
  private:
    Result& result;
    unsigned char *data;
    const long long size;
    const int pattern;
    const EVP_CIPHER *cipher;

    std::thread th;

  public:

    ActionEncrypt(Result& _result, int _pattern, unsigned char *_data, long long _size, const EVP_CIPHER *_cipher):
      result(_result), pattern(_pattern), data(_data), size(_size), cipher(_cipher) { }

    ActionEncrypt(ActionEncrypt &other) :
      result(other.result), pattern(other.pattern), data(other.data), size(other.size) { }
    ~ActionEncrypt() {
      th.join();
    }

    ActionEncrypt *start() {
      th = std::thread(&ActionEncrypt::run, this);
      return this;
    }

    void run() {
      std::unique_ptr<Patterer::Pattern> patterns(Patterer::create_pattern(pattern, data, size));
      EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

      unsigned char ciphertext[8192];

      LOG(INFO) << "ActionEncrypt::run:cipher=" << this->cipher;
      Result::Data bench;
      long long totalSize = 0;
      EVP_EncryptInit(ctx, this->cipher, data, data);
      for(int i = 0; i < pattern; ++i) {
        const Patterer::Pattern *pat = patterns.get()+i;
        if (((1+i)%(size/10)) == 0) {
          LOG(INFO) << "ActionEncrypt::run:=" << this << " " << i << " of " << pattern;
        }
        int len = sizeof(ciphertext);
        int result_len = 0;
        //LOG(INFO) << "in-" << pat->size << ":" << (long long)data << ":" << (long long)data+size << ":" << (long long)pat->packet;
        EVP_EncryptUpdate(ctx, ciphertext, &len, pat->packet, pat->size);
        //LOG(INFO) << "out-" << pat->size << ":" << len;
        result_len = len;
        EVP_EncryptFinal(ctx, ciphertext + result_len, &len);
        result_len += len;
        totalSize += pat->size;
      }
      bench.done();
      bench.totalSize = totalSize;
      LOG(INFO) << "ActionEncrypt::run:DONE=" << this << " size=" << totalSize << " time=" << bench.takes() << "=> " << totalSize/1024/1024/bench.takes() << "mb/sec";
      EVP_CIPHER_CTX_free(ctx);
      result.countDown(bench);
    }
};

class ActionDecrypt {
  private:
    Result& result;
    unsigned char *data;
    const long long size;
    const int pattern;
    const EVP_CIPHER *cipher;

    std::thread th;

  public:

    ActionDecrypt(Result& _result, int _pattern, unsigned char *_data, long long _size, const EVP_CIPHER *_cipher) :
      result(_result), pattern(_pattern), data(_data), size(_size), cipher(_cipher) {
      }

    ActionDecrypt(ActionDecrypt &other) :
      result(other.result), pattern(other.pattern), data(other.data), size(other.size) {
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

      EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

      unsigned char ciphertext[8192];

      LOG(INFO) << "ActionDecrypt::run:cipher=" << this->cipher;
      Result::Data bench;
      long long totalSize = 0;
      EVP_DecryptInit(ctx, this->cipher, data, data);
      for(int i = 0; i < pattern; ++i) {
        const Patterer::Pattern *pat = patterns.get()+i;
        if (((1+i)%(size/10)) == 0) {
          LOG(INFO) << "ActionDecrypt::run:=" << this << " " << i << " of " << pattern;
        }
        int len = sizeof(ciphertext);
        int result_len = 0;
        //LOG(INFO) << "in-" << pat->size << ":" << (long long)data << ":" << (long long)data+size << ":" << (long long)pat->packet;
        EVP_DecryptUpdate(ctx, ciphertext, &len, pat->packet, pat->size);
        //LOG(INFO) << "out-" << pat->size << ":" << len;
        result_len = len;
        EVP_DecryptFinal(ctx, ciphertext + result_len, &len);
        result_len += len;
        totalSize += pat->size;
      }
      bench.done();
      bench.totalSize = totalSize;
      LOG(INFO) << "ActionDecrypt::run:DONE=" << this << " size=" << totalSize << " time=" << bench.takes() << "=> " << totalSize/1024/1024/bench.takes() << "mb/sec";
      EVP_CIPHER_CTX_free(ctx);
      result.countDown(bench);
    }
};


class TestEnDecryption {
  private:
    unsigned char *data;
    long long size;
    long long workers;
    int pattern;
    const EVP_CIPHER *cipher;

  public:
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
      LOG(INFO) << "total result time=" << time << "sec totalsize=" << size/1024/1024 << "mb " << (size/1024/1024/time)*list.size() << "mb/sec";
    }
    void setup(long long size, int _workers, const char* cipher_suite) {
      LOG(INFO) << "Size=" << size/1024/1024 << "mb workers=" << _workers;
      OpenSSL_add_all_algorithms();
      this->cipher = EVP_get_cipherbyname(cipher_suite);
      if (!this->cipher) {
        LOG(ERROR) << "EVP_get_cipherbyname failed. Check your cipher suite string.";
        exit(1);
      }
      this->workers = _workers;
      this->size = ((size/EVP_MAX_MD_SIZE)+1)*EVP_MAX_MD_SIZE;
      this->data = new unsigned char[this->size];
      Result result(this->workers);
      std::list<std::unique_ptr<ActionPrepare>> workers;
      for (long long i = 0; i < this->workers; ++i) {
        long long size = std::min((i+1)*(this->size/this->workers), size);
        ActionPrepare *ap = (new ActionPrepare(result, this->data+(i*(this->size/this->workers)), size))->start();
        workers.push_back(std::unique_ptr<ActionPrepare>(ap));
      }
      LOG(INFO) << "Waiting" ;
      TestEnDecryption::wait(result);
    }

    void encrypt(int pattern) {
      LOG(INFO) << "Pattern=" << pattern << " workers=" << this->workers;
      Result result(this->workers);
      std::list<std::unique_ptr<ActionEncrypt>> workers;
      for (long long i = 0; i < this->workers; ++i) {
        ActionEncrypt *ae = (new ActionEncrypt(result, pattern, data, size, this->cipher))->start();
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
        ActionDecrypt *ae = (new ActionDecrypt(result, pattern, data, size, this->cipher))->start();
        workers.push_back(std::unique_ptr<ActionDecrypt>(ae));
      }
      LOG(INFO) << "Waiting" ;
      TestEnDecryption::wait(result);
    }

};

INITIALIZE_EASYLOGGINGPP
int main(int argc, char **argv) {
  START_EASYLOGGINGPP(argc, argv);
  const char *cipher_suite = "aes-256-cbc";

  TestEnDecryption ted;
  long long memory = 1024 * 1024 * 128;
  int workers = 2;
  int pattern = 100000;
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
    cipher_suite = std::stringstream(argv[4]).str().c_str();
  }
  ted.setup(memory, workers, cipher_suite);
  ted.encrypt(pattern);
  ted.decrypt(pattern);
//  sleep(10);
}
