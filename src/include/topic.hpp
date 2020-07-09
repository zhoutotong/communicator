#pragma once

#include <iostream>
#include <ros/serialization.h>
#include <ros/time.h>
#include <memory>
#include <functional>

#include <sys/time.h>

#include "communicator.hpp"
#include "direct_trans.hpp"

namespace boostros
{
typedef struct {
    double sec;
}Timestamp;
typedef struct {
    uint8_t type;
    size_t w;
    size_t h;
    size_t size;
    Timestamp ts;
} ImageHeader;


// 订阅者，用来订阅共享内存中的话题
template<class T>
class Subscriber
{
private:
    using Ptr      = boost::shared_ptr< T >;
    using ConstPtr = boost::shared_ptr< T const>;

    using CallbackFunc = std::function<void (const ConstPtr &)>;
    using CallbackFunc01 = std::function<void (const uint8_t *, const size_t &)>;

private:
    const CallbackFunc mCb;
    communicator::Interface *mIf;

public:
    Subscriber(const std::string &msg, const CallbackFunc &cb, const size_t &buffer_size, const size_t &buffer_num = 10) : mCb(cb)
    {

        std::string _msg = std::string(getenv("HOME")) + "/.boostros/" + msg;

        size_t cut_pos = _msg.find_last_of("/");
        mDir = _msg.substr(0, cut_pos);

        if(system(("mkdir -m 777 -p " + mDir).c_str()) < 0)
        {}

        mIf = new communicator::DirectTrans(_msg, 0, communicator::Interface::TransRecvSimplex);
        communicator::DirectTrans::DirectTransParams params;
        params.size = buffer_size;
        params.num_of_buf = buffer_num;

        mIf->setup(&params, sizeof(params));

        mIf->installRecvCallback(std::bind(&Subscriber::__recvCallback, this, std::placeholders::_1, std::placeholders::_2));
    }
    
    Subscriber(const std::string &msg, const CallbackFunc01 &cb, const size_t &buffer_size, const size_t &buffer_num = 10)
    {

        std::string _msg = std::string(getenv("HOME")) + "/.boostros/" + msg;

        size_t cut_pos = _msg.find_last_of("/");
        mDir = _msg.substr(0, cut_pos);

        if(system(("mkdir -m 777 -p " + mDir).c_str()) < 0)
        {}

        mIf = new communicator::DirectTrans(_msg, 0, communicator::Interface::TransRecvSimplex);
        communicator::DirectTrans::DirectTransParams params;
        params.size = buffer_size;
        params.num_of_buf = buffer_num;

        mIf->setup(&params, sizeof(params));

        mIf->installRecvCallback(cb);
    }

    ~Subscriber()
    {
        if(mIf)
        {
            mIf->release();
            delete mIf;
        }
    }

    void spinOnce()
    {
        if(mIf) mIf->spinOnce();
    }

private:

    std::string mDir;

    void __recvCallback(uint8_t *data, size_t len)
    {
        Ptr value(new T);
        
        ros::serialization::IStream istream(data, len);
        ros::serialization::deserialize(istream, *value.get());

        mCb(value);
    }
};


// 发布者，通过共享内存向订阅者发布话题
template<class T>
class Publisher
{

private:
    using Ptr      = boost::shared_ptr< T >;
    using ConstPtr = boost::shared_ptr< T const>;

public:

    Publisher(const std::string &msg, const size_t &buffer_size, const size_t &buffer_num = 10)
    {
        std::string _msg = std::string(getenv("HOME")) + "/.boostros/" + msg;

        size_t cut_pos = _msg.find_last_of("/");
        mDir = _msg.substr(0, cut_pos);

        if(system(("mkdir -m 777 -p " + mDir).c_str()) < 0)
        {}

        // 初始化传输接口
        mIf = new communicator::DirectTrans(_msg, 0, communicator::Interface::TransSendSimplex);
        communicator::DirectTrans::DirectTransParams params;
        
        params.size = buffer_size;
        params.num_of_buf = buffer_num;

        mIf->setup(&params, sizeof(params));
    }
    ~Publisher()
    {
        if(mIf)
        {
            mIf->release();
            delete mIf;

            std::cout << "rm dir: " << mDir << std::endl;

            system(("rm -rf " + mDir).c_str());
        }
    }

public:
    void publish(const T &data)
    {
        uint32_t size = ros::serialization::serializationLength(data);

        // 从共享内存中申请一个buffer用来传输
        uint8_t* buffer = static_cast<communicator::DirectTrans*>(mIf)->getBuffer(size);

        ros::serialization::OStream stream(buffer, size);
        ros::serialization::serialize(stream, data);

        // 将buffer还给共享内存，进行传输
        static_cast<communicator::DirectTrans*>(mIf)->releaseBuffer(buffer);
    }
    
    void publish(const uint8_t* data, const size_t &len)
    {
        // 从共享内存中申请一个buffer用来传输
        uint8_t* buffer = static_cast<communicator::DirectTrans*>(mIf)->getBuffer(len);

        memcpy(buffer, data, len);

        // 将buffer还给共享内存，进行传输
        static_cast<communicator::DirectTrans*>(mIf)->releaseBuffer(buffer);
    }

private:
    communicator::Interface *mIf;
    std::string mDir;

};




} // namespace boostros
