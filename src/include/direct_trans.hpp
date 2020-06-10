#pragma once

#include "communicator.hpp"
#include <sstream>
#include <string.h>
#include <thread>
#include <sys/time.h>
#include <unistd.h>


namespace communicator
{
class DirectTrans : public Interface
{

private:
    enum _DirectTransStatus {
        DirectTransFree = 0,   ///< 此状态下可写不可读
        DirectTransWrite = 1,  ///< 此状态写正在进行写入操作
        DirectTransWait = 2,   ///< 此状态下可读不可写
        DirectTransRead = 3,   ///< 此状态下正在进行读出操作
    };

    struct _DirectTransParams {
        size_t num_of_buf;
        size_t size;
    };

    struct _ShareMemHead {
        pthread_mutex_t share_mutex;
        size_t num_of_buf;
        size_t cur_read_ptr;
        size_t cur_write_ptr;
        size_t max_busy_cnt;
        size_t seq;
        size_t size;
        timeval timestamp;
        pid_t sender_pid;
        char sender_name[256];
    };

    struct _ShareMemStatus {
        _DirectTransStatus status;
        size_t busy_read_cnt;    ///< 在尝试写入时，如该buffer处于读状态，则该计数加1
        size_t size;
        size_t num_of_read_time;
        timeval timestamp;
    };

protected:
    typedef _ShareMemStatus ShareMemStatus;
    typedef _ShareMemHead   ShareMemHead;

public:
    typedef _DirectTransParams DirectTransParams;
    typedef _DirectTransStatus DirectTransStatus;

public:
    DirectTrans(const awe::AString &name, const int id = 0, const TransMode &mode = TransFullDuplex);
    ~DirectTrans();
    // 初始化参数配置
    virtual void setup(const void* cfg, uint32_t len) override;
    // 释放通信资源
    virtual void release() override;

    // 设置发送目标
    virtual void setDestination(const AString &port) override;
    virtual void setDestination(const uint32_t &id) override;

    // 发送数据
    virtual void send(const uint8_t *buf, uint32_t len) override;

    // 非堵塞接收数据
    virtual void recv(uint8_t *buf, uint32_t *len) override;

    void installRecvCallback(const RecvCallback cb) override;

    inline size_t getSendSeq() override { return mSendSeq; }
    inline size_t getRecvSeq() override { return mRecvSeq; }

    uint8_t* getBuffer(size_t len);
    void releaseBuffer(uint8_t* buf);

private:
    const int mId;
    size_t mSize;
    void *mShareMemPtr;
    int mShareMemId;
    int mShareMemKey;
    size_t mSendSeq;
    size_t mRecvSeq;

    std::thread *mRecThread;
    bool mIsRecWorking;

    size_t mBufferSize;
    size_t mMaxBusyCnt;

    ShareMemHead *mHeaderPtr;
    uint8_t *mShareBufs;

    size_t mSelfReadCnt;

    enum {
        SEND_LOCK_TIME_OUT = 500000,
        RECV_LOCK_TIME_OUT = 500000,
    };

    int __getAttchedNum(int id);
    size_t __getAttchedSize(int id);

    void __initMutex(pthread_mutex_t *m);
    uint8_t* __getWritablePtr();
    void __releaseWritablePtr(uint8_t* p);
    uint8_t* __getReadablePtr();
    void __releaseReadablePtr(uint8_t* p);

    int __timedMutexLock(pthread_mutex_t *m, uint32_t usec);
    void __checkErrorCode(const int e);

    bool __checkOk();// 检查是否满足创建共享内存条件
    void __checkFreeWriteAndUnlock();
    void __checkReadWaitFreeAndUnlock();


};
} // namespace communicator
