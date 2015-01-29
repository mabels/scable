
#include <iostream>
#include <mutex>
#include <thread>
#include <list>
#include <random>
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

class BenchMark {
  private:
    struct timeval start;

  public:
    BenchMark() {
      gettimeofday(&start, 0);
    }

    float done() {
      struct timeval done;
      gettimeofday(&done, 0);
      struct timeval result;
      timeval_subtract (&result, &done, &start);
      return result.tv_sec + (result.tv_usec/1000000.0);
    }

    int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y) {
      /* Perform the carry for the later subtraction by updating y. */
      if (x->tv_usec < y->tv_usec) {
        int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
      }
      if (x->tv_usec - y->tv_usec > 1000000) {
        int nsec = (x->tv_usec - y->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
      }

      /* Compute the time remaining to wait.
         tv_usec is certainly positive. */
      result->tv_sec = x->tv_sec - y->tv_sec;
      result->tv_usec = x->tv_usec - y->tv_usec;

      /* Return 1 if result is negative. */
      return x->tv_sec < y->tv_sec;
    }

};

class ActionPrepare {
  private:
    CountDownLatch& cdl;
    unsigned char *data;
    const long long size;

    std::thread th;

  public:

    ActionPrepare(CountDownLatch& _cdl, unsigned char *_data, long long _size) : cdl(_cdl), data(_data), size(_size) {
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
      unsigned char *last = this->data;
      BenchMark bench;
      for(unsigned char *ptr = this->data; ptr < (this->data+this->size); ptr += md_len) {
        if ((ptr - last) > 10000000) {
//          LOG(INFO) << "ActionPrepare::run:ptr=" << this << " data=" << (long long)ptr ;
          last = ptr;
        }
        EVP_DigestInit(&md_ctx, md);
        EVP_DigestUpdate(&md_ctx, ptr, md_len); // fakey
        EVP_DigestFinal(&md_ctx, hash, &md_len);
        memcpy(ptr, hash, md_len);
      }
      float time = bench.done();
      LOG(INFO) << "ActionPrepare::run:DONE=" << this << " size=" << size << " time=" << time << "=> " << size/1024/1024/time << "mb/sec";
      EVP_MD_CTX_cleanup(&md_ctx);
      cdl.countDown();
    }
};


class ActionEncrypt {
  private:
    CountDownLatch& cdl;
    unsigned char *data;
    const long long size;
    const int pattern;

    std::thread th;

  public:

    ActionEncrypt(CountDownLatch& _cdl, int _pattern, unsigned char *_data, long long _size) :
      cdl(_cdl), pattern(_pattern), data(_data), size(_size) {
        LOG(INFO) << "ActionEncrypt::ActionEncrypt=" << this ;
      }

    ActionEncrypt(ActionEncrypt &other) :
      cdl(other.cdl), pattern(other.pattern), data(other.data), size(other.size) {
        LOG(INFO) << "ActionEncrypt::ActionEncrypt=" << this ;
      }
    ~ActionEncrypt() {
      LOG(INFO) << "ActionEncrypt::~ActionEncrypt=" << this ;
      th.join();
    }

    ActionEncrypt *start() {
      LOG(INFO) << "ActionEncrypt::run-1=" << this ;
      th = std::thread(&ActionEncrypt::run, this);
      return this;
    }

    typedef struct {
      unsigned char *packet;
      int  size;
    } Pattern;

    Pattern *create_pattern() {
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


    void run() {
      LOG(INFO) << "ActionEncrypt::run:Prepare:Start:" << this << " data=" << (long long)this->data;
      std::unique_ptr<Pattern> patterns(create_pattern());
      LOG(INFO) << "ActionEncrypt::run:Prepare:Done:" << this << " data=" << (long long)this->data;

      EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

      unsigned char ciphertext[8192];

      const EVP_CIPHER *cipher = EVP_aes_256_cbc(); //EVP_get_cipherbyname(cipher_name);
      LOG(INFO) << "ActionEncrypt::run:cipher=" << cipher;
      BenchMark bench;
      long long totalSize = 0;
      for(int i = 0; i < pattern; ++i) {
        Pattern *pat = patterns.get()+i;
        if (((1+i)%10000) == 0) {
          LOG(INFO) << "ActionEncrypt::run:=" << this << " i=" << i;
        }
        EVP_EncryptInit(ctx, cipher, data, data);
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
      float time = bench.done();
      LOG(INFO) << "ActionEncrypt::run:DONE=" << this << " size=" << totalSize << " time=" << time << "=> " << totalSize/1024/1024/time << "mb/sec";
      EVP_CIPHER_CTX_free(ctx);
      cdl.countDown();
    }
};


class TestEnDecryption {
  private:
    unsigned char *data;
    long long size;
    long long workers;
    int pattern;
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
      this->data = new unsigned char[this->size];
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

    void encrypt(int pattern) {
      LOG(INFO) << "Pattern=" << pattern << " workers=" << this->workers;
      CountDownLatch cdl(this->workers);
      std::list<std::unique_ptr<ActionEncrypt>> workers;
      for (long long i = 0; i < this->workers; ++i) {
        ActionEncrypt *ae = (new ActionEncrypt(cdl, pattern, data, size))->start();
        LOG(INFO) << "ActionEncrypt:1=" << ae;
        workers.push_back(std::unique_ptr<ActionEncrypt>(ae));
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
  ted.encrypt(100000);
}
