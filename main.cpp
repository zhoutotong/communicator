#include <iostream>
#include "communicator.hpp"
#include "direct_trans.hpp"

int main(int argc, char *argv[])
{
    std::cout << "this is communicator program..." << std::endl;
    communicator::Interface *interface = new communicator::DirectTrans("Direct_trans");
    interface->setDestination("");

    delete interface;
    return 0;
}