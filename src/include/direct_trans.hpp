#pragma once

#include "communicator.hpp"


namespace communicator
{
class DirectTrans : public Interface
{
public:
    DirectTrans(const awe::AString &name);
    ~DirectTrans();
    // 初始化参数配置
    virtual void setup(const char* cfg, uint32_t len) override;
    // 释放通信资源
    virtual void release() override;

    // 设置发送目标
    virtual void setDestination(const AString &port) override;
    virtual void setDestination(const uint32_t &id) override;

    // 发送数据
    virtual void send(const uint8_t *buf, uint32_t len) override;
    virtual void send(const AString &buf) override;

    // 非堵塞接收数据
    virtual void recv(const uint8_t *buf, uint32_t len) override;

};
} // namespace communicator
