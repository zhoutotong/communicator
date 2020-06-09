#include "utilities/aexception.hpp"


namespace awe
{
AException::AException(const AString &reason) : mWhat(reason)
{
    perror("");
}
AException::~AException()
{

}

} // namespace awe
