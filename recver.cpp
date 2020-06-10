#include <iostream>
#include "communicator.hpp"
#include "direct_trans.hpp"
#include <unistd.h>

communicator::Interface *interface;

void recvCallback(const uint8_t *buf, uint32_t len)
{
    // std::cout << "recv data: " << (char*)buf << " " << interface->getRecvSeq() << std::endl;
    timeval sendTime;
    memcpy(&sendTime, &buf[1024], sizeof(timeval));

    timeval tv;
    gettimeofday(&tv, nullptr);

    uint64_t dt = tv.tv_sec * 1000000 + tv.tv_usec - sendTime.tv_sec * 1000000 - sendTime.tv_usec;

    static uint64_t sum = 0;
    static uint64_t max = dt, min = dt;

    max = max > dt ? max : dt;
    min = min < dt ? min : dt;
    sum += dt;


    static size_t cnt = 0;
    cnt++;

    double aver = (double)(sum - max - min) / (double)(cnt - 2);

    static size_t large_cnt = 0;
    if(dt > 3.0 * aver)
    {
        large_cnt++;
    }

    std::cout << "recv cnt: " << cnt << " max: " << max << " min: " << min << " aver: " << aver <<\
     " large cnt(%): " << (double)large_cnt / (double)cnt << " - " << (char*)buf << " dt: " << dt << std::endl;
}

int main(int argc, char *argv[])
{
    std::cout << "this is communicator program for recver..." << std::endl;
    interface = new communicator::DirectTrans("Direct_trans");
    communicator::DirectTrans::DirectTransParams params;
    const size_t memSize = 1024 * 1024 * 10;
    params.size = memSize;
    params.num_of_buf = 10;

    uint8_t *pbuf = new uint8_t[params.size];
    interface->setup(&params, sizeof(params));

    interface->installRecvCallback(recvCallback);

    while(true)
    {
        sleep(1);
    }

    interface->release();
    delete []pbuf;

    delete interface;
    return 0;
}