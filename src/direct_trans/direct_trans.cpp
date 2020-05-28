#include "direct_trans.hpp"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <sys/stat.h>

#include <sys/time.h>

namespace communicator
{
DirectTrans::DirectTrans(const awe::AString &name, const int id) : Interface(name)
   , mId(id)
   , mSize(0)
   , mShareMemPtr(nullptr)
   , mShareMemId(-1)
   , mShareMemKey(-1)
   , mSendSeq(0)
   , mRecvSeq(0)
   , mRecThread(nullptr)
   , mIsRecWorking(false)
   , mBufferSize(2)
   , mHeaderPtr(nullptr)
{

}

DirectTrans::~DirectTrans()
{
    release();
}


// 初始化参数配置
void DirectTrans::setup(const void* cfg, uint32_t len)
{
    // 检查传入参数大小是否正常，异常则不处理
    if(len != sizeof(DirectTransParams))
    {
        throw AException("Params Error.");
    }

    const DirectTransParams *params = static_cast<const DirectTransParams*>(cfg);
    // size 的组成为：要申请的大小 + head 的大小
    mSize = (params->size + sizeof(DirectTransHead)) * mBufferSize;

    // 检查文件是否存在，不存在则尝试创建
    int r = 0;
    r = access(mInterfaceName.c_str(), F_OK);
    if(r < 0)
    {
        // 文件不存在，此时新建文件
        FILE *fd = fopen(mInterfaceName.c_str(), "w+");
        if(!fd)
        {
            std::stringstream ss;
            ss << "Shared Mem <" << mInterfaceName << "> File Create Failed(" << errno << ").";
            throw AException(ss.str());
        }
        fclose(fd);
        chmod(mInterfaceName.c_str(), 0400);
    }

    // 获取全局唯一标识符
    key_t key = ftok(mInterfaceName.c_str(), mId);

    if(-1 == key)
    {
        std::stringstream ss;
        ss << "Can Not Get Key For Share Mem, Please Check Name Is Available(" << errno << ").";
        throw AException(ss.str());
    }

    // 创建共享内存
    int id = shmget(key, mSize, IPC_CREAT | 0600);

    if(-1 == id)
    {
        std::stringstream ss;
        ss << "Create Share Mem Failed(" << errno << ").";
        throw AException(ss.str());
    }

    // 挂接共享内存
    void* ptr = shmat(id, nullptr, 0);

    if ((void *)(-1) == ptr)
    {
        // 尝试删除创建的共享内存，如果当前连接的进程数量为 0 ，那么就删除这片共享内存，否则不处理
        if (0 == __getAttchedNum(id))
        {
            int r = shmctl(id, IPC_RMID, nullptr);
            if (-1 == r)
            {
                std::stringstream ss;
                ss << "Remove Shared Mem Id Failed(" << errno << ").";
                throw AException(ss.str());
            }
            // 删除对应的文件
            r = remove(mInterfaceName.c_str());
            if (r < 0)
            {
                std::stringstream ss;
                ss << "Share Mem File Remove Failed(" << errno << ").";
                throw AException(ss.str());
            }
        }
        std::stringstream ss;
        ss << "Map To Share Mem Failed(" << errno << ").";
        throw AException(ss.str());
    }

    // 创建成功，保存必要参数
    mShareMemId = id;
    mShareMemPtr = ptr;
    mShareMemKey = key;
    mHeaderPtr = static_cast<DirectTransHead*>(mShareMemPtr);


    // 检查当前连接的数量，如果为 1 则初始化状态参数
    int num = __getAttchedNum(mShareMemId);
    if(num == 1)
    {
        mHeaderPtr->status = DirectTransFree;///< 初始化为等待写入状态
        mHeaderPtr->seq = 0; ///< 发送计数清零
        mHeaderPtr->size = 0;
        __initMutex(&mHeaderPtr->mutex);
    }

}
// 释放通信资源
void DirectTrans::release()
{
    // 如果接收回调线程在工作，则首先退出
    if(mIsRecWorking)
    {
        mIsRecWorking = false;
        mRecThread->join();
        delete mRecThread;
        mRecThread = nullptr;
    }

    // 解除挂接状态
    if(mShareMemPtr != nullptr)
    {
        int r = shmdt(mShareMemPtr);
        if(-1 == r)
        {
            std::stringstream ss;
            ss << "Detach Shared Mem Failed(" << errno << ").";
            throw AException(ss.str());
        }
    }

    // 尝试删除创建的共享内存，如果当前连接的进程数量为 0 ，那么就删除这片共享内存，否则不处理
    if(0 == __getAttchedNum(mShareMemId))
    {
        // 销毁互斥锁
        pthread_mutex_destroy(&static_cast<DirectTransHead*>(mShareMemPtr)->mutex);
        // 删除共享内存
        int r = shmctl(mShareMemId, IPC_RMID, nullptr);
        if(-1 == r)
        {
            std::stringstream ss;
            ss << "Remove Shared Mem Id Failed(" << errno << ").";
            throw AException(ss.str());
        }
        // 删除对应的文件
        r = remove(mInterfaceName.c_str());
        if(r < 0)
        {
            std::stringstream ss;
            ss << "Share Mem File Remove Failed(" << errno << ").";
            throw AException(ss.str());
        }
    }
}

// 设置发送目标
void DirectTrans::setDestination(const AString &port)
{

}
void DirectTrans::setDestination(const uint32_t &id)
{

}

// 发送数据
void DirectTrans::send(const uint8_t *buf, uint32_t len)
{
    // 判断当前状态，当变为可写状态是写入
    DirectTransHead *pHead = static_cast<DirectTransHead*>(mShareMemPtr);

    if(pHead->status == DirectTransFree)
    {
        pHead->status = DirectTransWrite;
        // 取较小的size进行发送
        size_t s = mSize < len ? mSize : len;
        // 取出数据区域
        void *pData = static_cast<void*>((uint8_t*)mShareMemPtr + sizeof(DirectTransHead));
        memcpy(pData, buf, s);
        // 计数加 1
        pHead->seq++;
        pHead->size = s;
        mSendSeq = pHead->seq;
        // 修改状态为 Wait
        pHead->status = DirectTransWait;
    }
    else
    {
        // std::cout << "share mem status: " << pHead->status << std::endl;
    }
}

// 非堵塞接收数据
void DirectTrans::recv(uint8_t *buf, uint32_t *len)
{
    if(mRecvCb)
    {
        throw AException("Please Read Data In Receive Callback Function.");
    }
    DirectTransHead *pHead = static_cast<DirectTransHead*>(mShareMemPtr);
    if(pHead->status == DirectTransWait)
    {
        pHead->status = DirectTransRead;
        // 取较小的size进行接收
        size_t s = mSize < *len ? mSize : *len;
        // 取出数据区域
        void *pData = static_cast<void*>((uint8_t*)mShareMemPtr + sizeof(DirectTransHead));
        memcpy(buf, pData, s);
        mRecvSeq = pHead->seq;
        *len = s;
        // 修改状态为 Wait
        pHead->status = DirectTransFree;
    }
    else
    {
        *len = 0;
    }
}

void DirectTrans::installRecvCallback(const RecvCallback cb)
{
    if(mRecThread)
    {
        throw AException("There Are Already Has Rec Cb.");
    }
    mRecvCb = cb;
    mRecThread = new std::thread(
        [this](){
            mIsRecWorking = true;
            while(mIsRecWorking)
            {
                DirectTransHead *pHead = static_cast<DirectTransHead*>(mShareMemPtr);
                if(pHead->status == DirectTransWait)
                {
                    pHead->status = DirectTransRead;
                    // 取出数据区域
                    uint8_t *pData = static_cast<uint8_t*>((uint8_t*)mShareMemPtr + sizeof(DirectTransHead));
                    mRecvCb(pData, pHead->size);
                    mRecvSeq = pHead->seq;
                    // 修改状态为 Wait
                    pHead->status = DirectTransFree;
                }
                else
                {
                    sleep(0);
                }
            }
        }
    );
}

int DirectTrans::__getAttchedNum(int id)
{
    shmid_ds shmInfo;
    int r = shmctl(id, IPC_STAT, &shmInfo);
    if(-1 == r)
    {
        std::stringstream ss;
        ss << "Get Shared Mem Stat Failed(" << errno << ").";
        throw AException(ss.str());
    }
    return shmInfo.shm_nattch;
}

size_t DirectTrans::__getAttchedSize(int id)
{
    shmid_ds shmInfo;
    int r = shmctl(id, IPC_STAT, &shmInfo);
    if(-1 == r)
    {
        std::stringstream ss;
        ss << "Get Shared Mem Size Failed(" << errno << ").";
        throw AException(ss.str());
    }
    return shmInfo.shm_segsz;
}

void DirectTrans::__initMutex(pthread_mutex_t *m)
{
    pthread_mutexattr_t mutexattr;
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(m, &mutexattr);
}

// 获取可写入的地址
uint8_t* DirectTrans::__getWritablePtr()
{
    // 进行加锁
    timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000;
    int r = pthread_mutex_timedlock(&mHeaderPtr->mutex, &ts);
    if(r == 0)
    {

        pthread_mutex_unlock(&mHeaderPtr->mutex);
    }
    return nullptr;
}

// 获取可读取的地址
uint8_t* DirectTrans::__getReadablePtr()
{
    // 进行加锁
    timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000;
    int r = pthread_mutex_timedlock(&mHeaderPtr->mutex, &ts);
    if(r == 0)
    {

        pthread_mutex_unlock(&mHeaderPtr->mutex);
    }
    return nullptr;
}

} // namespace communicator
