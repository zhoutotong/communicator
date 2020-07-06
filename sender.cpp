#include <iostream>
#include "communicator.hpp"
#include "direct_trans.hpp"
#include <unistd.h>

#include "utilities/systools.hpp"

int main(int argc, char *argv[])
{
    std::cout << "this is communicator program for sender..." << getpid() << std::endl;

    std::string name;
    awe::systools::getNameByPid(getpid(), name);
    std::cout << name << std::endl;

    communicator::Interface *interface = new communicator::DirectTrans("Direct_trans", 0, communicator::Interface::TransSendSimplex);
    communicator::DirectTrans::DirectTransParams params;
    
    const size_t memSize = 1024 * 1024 * 10;
    params.size = memSize;
    params.num_of_buf = 10;
    uint8_t *pbuf = new uint8_t[params.size];
    interface->setup(&params, sizeof(params));

    timeval tv;


    int cnt = 0;
    while(true)
    {
        std::stringstream ss;
        ss << "send data to rec: " << cnt;
        memcpy(pbuf, ss.str().c_str(), ss.str().size());


        // memset(pbuf, 'a', memSize);
        gettimeofday(&tv, nullptr);
        memcpy(&pbuf[1024], &tv, sizeof(timeval));
        interface->send(pbuf, params.size);
        // std::cout << "send data: " << interface->getSendSeq() << std::endl;
        // usleep(1);
        cnt++;
        std::cout << "send cnt: " << cnt << std::endl;
    }

    interface->release();
    delete []pbuf;

    delete interface;
    return 0;
}