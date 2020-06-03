#include <iostream>
#include "communicator.hpp"
#include "direct_trans.hpp"
#include <unistd.h>

communicator::Interface *interface;

void recvCallback(const uint8_t *buf, uint32_t len)
{
    std::cout << "recv data: " << (char*)buf << " " << interface->getRecvSeq() << std::endl;

}

int main(int argc, char *argv[])
{
    std::cout << "this is communicator program..." << std::endl;
    interface = new communicator::DirectTrans("Direct_trans");
    communicator::DirectTrans::DirectTransParams params;
    params.size = 1024;
    params.num_of_buf = 10;

    uint8_t *pbuf = new uint8_t[params.size];
    interface->setup(&params, sizeof(params));

    interface->installRecvCallback(recvCallback);

    while(interface->getRecvSeq() < 100000)
    {
        sleep(1);
    }

    interface->release();
    delete []pbuf;

    delete interface;
    return 0;
}