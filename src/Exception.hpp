#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <stdexcept>
#include <string>

namespace konex {

class KOnexException : public std::runtime_error
{
public:
  KOnexException(const char* msg) : std::runtime_error(msg) {}
  KOnexException(std::string msg) : std::runtime_error(msg.c_str()) {}
};

} // namespace konex

#endif // EXCEPTION_H
