#include <iostream>
#include "communicator.hpp"
#include "direct_trans.hpp"
#include <unistd.h>

communicator::Interface *interface;

void recvCallback(const uint8_t *buf, uint32_t len)
{
    // std::cout << "recv data: " << (char*)buf << " " << interface->getRecvSeq() << std::endl;
    static size_t cnt = 0;
    cnt++;
    std::cout << "recv cnt: " << cnt << " - " << (char*)buf << std::endl;
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