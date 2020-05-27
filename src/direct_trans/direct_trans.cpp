#include "direct_trans.hpp"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>

namespace communicator
{
DirectTrans::DirectTrans(const awe::AString &name) : Interface(name)
{
    std::cout << "trans name is: " << name << std::endl;

    key_t key = ftok("./ipc_test", 1);
    int id = shmget(key, 4096, IPC_CREAT | IPC_EXCL);

    std::cout << "创建共享内存：" << key << " " << id << std::endl;

    if(id != -1)
    {

    }
}

DirectTrans::~DirectTrans()
{
}


// 初始化参数配置
void DirectTrans::setup(const char* cfg, uint32_t len)
{

}
// 释放通信资源
void DirectTrans::release()
{

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

}
void DirectTrans::send(const AString &buf)
{

}

// 非堵塞接收数据
void DirectTrans::recv(const uint8_t *buf, uint32_t len)
{

}

} // namespace communicator
