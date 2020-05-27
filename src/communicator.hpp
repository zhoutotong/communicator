#pragma once

#include <iostream>

#include "device/device.hpp"

#include "utilities/typedef.hpp"
#include "utilities/aexception.hpp"


#define UNSUPORT_FUNC_EXCEPTION(where) throw AException(AString(__FUNCTION__) + "Unsuport Function in <" + where + ">.")

using namespace awe;

namespace communicator
{

class Interface
{
private:
typedef void(*RecvCallback)(const uint8_t *buf, uint32_t len);
public:
    Interface(const AString &name) : mInterfaceName(name), mRecvCb(nullptr){}
    ~Interface(){};

    // 初始化参数配置
    virtual void setup(const char* cfg, uint32_t len){}
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
    virtual void recv(const uint8_t *buf, uint32_t len){}

    // 设置数据接收回调，设置后将屏蔽非堵塞接收数据方法
    void installRecvCallback(const RecvCallback cb) { mRecvCb = cb; };

private:

protected:
    const AString mInterfaceName;
    RecvCallback mRecvCb;


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
