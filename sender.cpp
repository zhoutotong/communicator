#include <iostream>
#include "communicator.hpp"
#include "direct_trans.hpp"
#include <unistd.h>

int main(int argc, char *argv[])
{
    std::cout << "this is communicator program..." << std::endl;
    communicator::Interface *interface = new communicator::DirectTrans("Direct_trans");
    communicator::DirectTrans::DirectTransParams params;
    params.size = 1024;
    uint8_t *pbuf = new uint8_t[params.size];
    interface->setup(&params, sizeof(params));

    int cnt = 100000;
    while(cnt--)
    {
        interface->send((const uint8_t*)"hello world", sizeof("hello world"));
        // std::cout << "send data: " << interface->getSendSeq() << std::endl;
        // sleep(1);
    }

    interface->release();
    delete []pbuf;

    delete interface;
    return 0;
}