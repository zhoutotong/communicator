#include <iostream>
#include "communicator.hpp"
#include "direct_trans.hpp"
#include <unistd.h>

int main(int argc, char *argv[])
{
    std::cout << "this is communicator program for sender..." << std::endl;
    communicator::Interface *interface = new communicator::DirectTrans("Direct_trans");
    communicator::DirectTrans::DirectTransParams params;
    
    const size_t memSize = 1024 * 1024 * 10;
    params.size = memSize;
    params.num_of_buf = 10;
    uint8_t *pbuf = new uint8_t[params.size];
    interface->setup(&params, sizeof(params));

    memcpy(pbuf, "hello world\0", sizeof("hello world\0"));
    int cnt = 0;
    while(true)
    {
        // memset(pbuf, 'a', memSize);
        interface->send(pbuf, params.size);
        // std::cout << "send data: " << interface->getSendSeq() << std::endl;
        usleep(1);
        cnt++;
        std::cout << "send cnt: " << cnt << std::endl;
    }

    interface->release();
    delete []pbuf;

    delete interface;
    return 0;
}