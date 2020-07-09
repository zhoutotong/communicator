#pragma once

#include <iostream>
#include <functional>

#include "utilities/typedef.hpp"
#include "utilities/aexception.hpp"


#define UNSUPORT_FUNC_EXCEPTION(where) throw AException(AString(__FUNCTION__) + "Unsuport Function in <" + where + ">.")

using namespace awe;

namespace communicator
{

class Interface
{

public:
    
    enum _TransMode {
        TransFullDuplex,
        TransHalfDuplex,
        TransSendSimplex,
        TransRecvSimplex,
    };

protected:

// 回调接口定义
using RecvCallback = std::function<void (uint8_t *, uint32_t)>;

public:
// 定义工作模式
using TransMode = _TransMode;

public:
    Interface(const AString &name, const TransMode &mode = TransFullDuplex)
     : mInterfaceName(name)
     , mRecvCb(nullptr)
     , mTransMode(mode){}
    virtual ~Interface(){};

    // 初始化参数配置
    virtual void setup(const void* cfg, uint32_t len){}
    // 释放通信资源
    virtual void release(){}

    // 设置发送目标
    virtual void setDestination(const AString &port) { UNSUPORT_FUNC_EXCEPTION(mInterfaceName); }
    virtual void setDestination(const uint32_t &id) { UNSUPORT_FUNC_EXCEPTION(mInterfaceName); }
    virtual void setDestination(const AString &ip, const uint32_t &port) { UNSUPORT_FUNC_EXCEPTION(mInterfaceName); }

    // 发送数据
    virtual void send(const uint8_t *buf, uint32_t len) { UNSUPORT_FUNC_EXCEPTION(mInterfaceName); };
    virtual void send(const AString &buf) { UNSUPORT_FUNC_EXCEPTION(mInterfaceName); };

    // 非堵塞接收数据
    virtual void recv(uint8_t *buf, uint32_t *len){}

    // 设置数据接收回调，设置后将屏蔽非堵塞接收数据方法
    virtual void installRecvCallback(const RecvCallback cb) { mRecvCb = cb; };

    virtual void spinOnce() = 0;


    // 获取传输包信息
    virtual size_t getSendSeq() { UNSUPORT_FUNC_EXCEPTION(mInterfaceName); };
    virtual size_t getRecvSeq() { UNSUPORT_FUNC_EXCEPTION(mInterfaceName); };

private:

protected:
    const AString mInterfaceName;
    RecvCallback mRecvCb;
    const TransMode mTransMode;


};


class Communicator
{
private:
    /* data */
public:
    explicit Communicator(Interface &interface);
    ~Communicator();

private:
    Interface &mIf;


};

} // namespace communicator
