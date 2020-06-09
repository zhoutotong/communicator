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
   , mShareBufs(nullptr)
   , mMaxBusyCnt(10)
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
    // size 组成
    mBufferSize = params->num_of_buf;
    mSize = sizeof(ShareMemHead) + \
            (params->size + sizeof(ShareMemStatus)) * mBufferSize;

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

    if(errno == EEXIST)
    {
        id = shmget(key, mSize, 0600);
    }

    if(-1 == id)
    {
        // 删除对应的文件
        r = remove(mInterfaceName.c_str());
        if (r < 0)
        {
            std::stringstream ss;
            ss << "Share Mem File Remove Failed(" << errno << ").";
            throw AException(ss.str());
        }
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
    mHeaderPtr = static_cast<ShareMemHead*>(mShareMemPtr);
    mShareBufs = static_cast<uint8_t*>(mShareMemPtr) + sizeof(ShareMemHead);


    // 检查当前连接的数量，如果为 1 则初始化状态参数
    int num = __getAttchedNum(mShareMemId);
    if(num == 1)
    {
        mHeaderPtr->num_of_buf = mBufferSize;
        mHeaderPtr->max_busy_cnt = mMaxBusyCnt;
        mHeaderPtr->size = params->size;
        mHeaderPtr->cur_read_ptr = 0;
        mHeaderPtr->cur_write_ptr = 0;
        mHeaderPtr->seq = 0;
        gettimeofday(&mHeaderPtr->timestamp, nullptr);

        __initMutex(&mHeaderPtr->share_mutex);

        // 获取缓冲区地址，并进行初始化
        for(int i = 0; i < mHeaderPtr->num_of_buf; i++)
        {
            ShareMemStatus *p = (ShareMemStatus*)(mShareBufs + \
                                    ((sizeof(ShareMemStatus) + mHeaderPtr->size) * i));
            
            p->status = DirectTransFree;
            p->busy_read_cnt = 0;
        }

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
        if(1 == __getAttchedNum(mShareMemId))
        {
            // 销毁互斥锁
            pthread_mutex_destroy(&static_cast<ShareMemHead*>(mShareMemPtr)->share_mutex);
        }
        
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

uint8_t* DirectTrans::getBuffer(size_t len)
{
    // 判断当前状态，当变为可写状态是写入
    ShareMemHead *pHead = static_cast<ShareMemHead*>(mShareMemPtr);
    uint8_t* pbuf = __getWritablePtr();
    ShareMemStatus* pStatus = (ShareMemStatus*)pbuf;
    pbuf += sizeof(ShareMemStatus);

    if(!pStatus)
    {
        throw AException("Can Not Get Avaiable Write Buffer.");
    }

    if(pHead->size < len)
    {
        std::stringstream ss;
        ss << "Get Size Of Buffer Is Too Large." << std::endl \
        << "Max Size: " << pHead->size << " Then Require: " << len << std::endl;
        throw AException(ss.str());
    }

    pStatus->size = len;

    // 设置时间戳
    gettimeofday(&pStatus->timestamp, nullptr);
    mSendSeq++;

    return pbuf;
}
void DirectTrans::releaseBuffer(uint8_t* buf)
{
    __releaseWritablePtr(buf - sizeof(ShareMemStatus));
}

// 发送数据
void DirectTrans::send(const uint8_t *buf, uint32_t len)
{
    // 判断当前状态，当变为可写状态是写入
    ShareMemHead *pHead = static_cast<ShareMemHead*>(mShareMemPtr);
    uint8_t* pbuf = __getWritablePtr();
    ShareMemStatus* pStatus = (ShareMemStatus*)pbuf;
    pbuf += sizeof(ShareMemStatus);

    if(!pStatus)
    {
        throw AException("Can Not Get Avaiable Write Buffer.");
    }
    // 取较小的size进行发送
    size_t s = pHead->size < len ? pHead->size : len;
    pStatus->size = s;
    // 设置时间戳
    gettimeofday(&pStatus->timestamp, nullptr);
    // 写入缓存
    memcpy(pbuf, buf, s);
    mSendSeq++;
    __releaseWritablePtr((uint8_t*)pStatus);
}

// 非堵塞接收数据
void DirectTrans::recv(uint8_t *buf, uint32_t *len)
{
    if(mRecvCb)
    {
        throw AException("Please Read Data In Receive Callback Function.");
    }

    uint8_t* pbuf = __getReadablePtr();
    ShareMemStatus* pStatus = (ShareMemStatus*)pbuf;
    pbuf += sizeof(ShareMemStatus);
    if(pbuf)
    {
        // 取较小的size进行接收
        size_t s = mHeaderPtr->size < *len ? mHeaderPtr->size : *len;
        // 取出数据区域
        memcpy(buf, pbuf, s);
        mRecvSeq++;
        *len = s;
        __releaseReadablePtr((uint8_t*)pStatus);
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
                uint8_t* pbuf = __getReadablePtr();
                ShareMemStatus* pStatus = (ShareMemStatus*)pbuf;
                pbuf += sizeof(ShareMemStatus);

                if(pStatus)
                {
                    // 计算延迟
                    timeval tv;
                    gettimeofday(&tv, nullptr);
                    int dt = -(pStatus->timestamp.tv_sec * 1e6 + pStatus->timestamp.tv_usec \
                                    - tv.tv_usec - (tv.tv_sec * 1e6));
                    // 取出数据区域
                    mRecvCb(pbuf, pStatus->size);
                    mRecvSeq++;
                    __releaseReadablePtr((uint8_t*)pStatus);
                }
                else
                {
                    usleep(10);
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
    // 尝试三次，失败就返回
    uint8_t *wbuffer = nullptr;
    while(true)
    {
        // 进行加锁
        int r = __timedMutexLock(&mHeaderPtr->share_mutex, SEND_LOCK_TIME_OUT);
        if(r == 0)
        {
            size_t &wcptr = mHeaderPtr->cur_write_ptr;
            ShareMemStatus* curBufStatus = nullptr;
            // 获取当前状态
            curBufStatus = (ShareMemStatus*)(mShareBufs + \
                                            (wcptr * (sizeof(ShareMemStatus) + mHeaderPtr->size)));

            // 检查当前写 buffer 是否空闲，等待空闲时写入
            // 如果当前只有一个接入的话就处于独占状态，这样的话就持续写入
            // 在这里一直等待，直到占用的节点读取完毕，或者退出
            if(curBufStatus->status != DirectTransFree && 1 < __getAttchedNum(mShareMemId))
            {
                curBufStatus->busy_read_cnt++;
                pthread_mutex_unlock(&mHeaderPtr->share_mutex);
                continue;
            }
            else
            {
                // 清空忙计数
                curBufStatus->busy_read_cnt = 0;
                // 获取可用指针
                curBufStatus->status = DirectTransWrite; // 将这个buffer设置为写入状态
                wbuffer = (uint8_t *)curBufStatus;
            }

            // 处理完毕进行解锁
            pthread_mutex_unlock(&mHeaderPtr->share_mutex);
            break;
        }

        __checkErrorCode(r);
        usleep(10);
    }

    return wbuffer;
}

// 获取可读取的地址
uint8_t* DirectTrans::__getReadablePtr()
{
    // 进行加锁
    int r = __timedMutexLock(&mHeaderPtr->share_mutex, 500000);
    if(r == 0)
    {
        size_t &rcptr = mHeaderPtr->cur_read_ptr;
        size_t cnt = 0;
        uint8_t *rbuffer = nullptr;
        ShareMemStatus* curBufStatus = nullptr;

        // 当前的读方案是遍历全部指针，找到可读的地址
        while(cnt < mHeaderPtr->num_of_buf)
        {
            cnt++;
            // 获取当前状态
            curBufStatus = (ShareMemStatus*)(mShareBufs + \
                                            (rcptr * (sizeof(ShareMemStatus) + mHeaderPtr->size)));

            // 检查当前状态是否处于 Wait，只有当状态变为 Wait 时才可以读取
            if(curBufStatus->status == DirectTransWait)
            {
                // 获取可用指针
                curBufStatus->status = DirectTransRead; // 将这个buffer设置为写入状态
                rbuffer = (uint8_t*)curBufStatus;

                timeval rect;
                gettimeofday(&rect, nullptr);

                uint64_t sendt = curBufStatus->timestamp.tv_sec * 1e6 + \
                                curBufStatus->timestamp.tv_usec;
                uint64_t dt = rect.tv_sec * 1e6 + rect.tv_usec - sendt;
                break;
            }
            else
            {
                rcptr = (rcptr + 1) % mHeaderPtr->num_of_buf;
            }
        }


        pthread_mutex_unlock(&mHeaderPtr->share_mutex);
        return rbuffer;
    }

    __checkErrorCode(r);
    return nullptr;
}

void DirectTrans::__releaseWritablePtr(uint8_t* p)
{
    // 进行加锁
    int r = __timedMutexLock(&mHeaderPtr->share_mutex, SEND_LOCK_TIME_OUT);
    if(r == 0)
    {
        ShareMemStatus *pStatus = (ShareMemStatus*)(p);

        pStatus->status = DirectTransWait;// 切换内存状态到等待读取状态
        size_t &wcptr = mHeaderPtr->cur_write_ptr;
        wcptr = (wcptr + 1) % mHeaderPtr->num_of_buf;// 移动写指针到下一个buffer
        pthread_mutex_unlock(&mHeaderPtr->share_mutex);
    }
    else
    {
        __checkErrorCode(r);
    }

    return;
}
void DirectTrans::__releaseReadablePtr(uint8_t* p)
{
    // 进行加锁
    int r = __timedMutexLock(&mHeaderPtr->share_mutex, RECV_LOCK_TIME_OUT);
    if(r == 0)
    {
        ShareMemStatus *pStatus = (ShareMemStatus*)(p);

        pStatus->status = DirectTransFree;// 修改内存状态
        size_t &rcptr = mHeaderPtr->cur_read_ptr;
        rcptr = (rcptr + 1) % mHeaderPtr->num_of_buf;// 移动指针到下一个buffer

        pthread_mutex_unlock(&mHeaderPtr->share_mutex);
    }
    return;
}

int DirectTrans::__timedMutexLock(pthread_mutex_t *m, uint32_t usec)
{
    int r = 0;
    timeval tv;
    gettimeofday(&tv, nullptr);
    uint32_t now = tv.tv_sec * 1000000 + tv.tv_usec;
    const uint32_t start = now;

    while(now - start < usec)
    {
        r = pthread_mutex_trylock(m);
        if(r == 0)
        {
            return r;
        }

        gettimeofday(&tv, nullptr);
        now = tv.tv_sec * 1000000 + tv.tv_usec;
        usleep(5);
    }

    return r;
}

void DirectTrans::__checkErrorCode(const int e)
{
    switch (e)
    {
        case 0:break;
        case ETIMEDOUT:
            std::cout << "Mutex Time Out." << std::endl;
            break;
        case EINVAL:
            std::cout << "Invalid Argument." << std::endl;
            break;
        case EBUSY:
            break;
        default:
            std::cout << "Unknown Error: " << e << std::endl;
            break;
    }
}
} // namespace communicator
