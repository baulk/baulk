//
#ifndef BAULK_ECHO_HPP
#define BAULK_ECHO_HPP
#include <bela/base.hpp>

namespace baulk::echo {
class Processor {
public:
  Processor() = default;
  Processor(const Processor &) = delete;
  Processor &operator=(const Processor &) = delete;
  int Execute();
};
} // namespace baulk::echo

#endif