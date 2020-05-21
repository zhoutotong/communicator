#include <iostream>

namespace communicator
{

class Interface
{
private:
    /* data */
public:
    Interface();
    ~Interface();

    void setDestination();
};


class Communicator
{
private:
    /* data */
public:
    explicit Communicator(Interface &interface);
    ~Communicator();
};

} // namespace communicator
