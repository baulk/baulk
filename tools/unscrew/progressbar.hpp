//
#ifndef BAULK_UNSCREW_HPP
#define BAULK_UNSCREW_HPP
#include <bela/base.hpp>
#include <bela/comutils.hpp>
#include <ShObjIdl.h>
#include <ShlObj_core.h>

namespace baulk::unscrew {
class ProgressBar {
public:
  ProgressBar() = default;
  ProgressBar(const ProgressBar &) = delete;
  ProgressBar &operator=(const ProgressBar &) = delete;
  bool InitializeFile(std::wstring_view path);
  bool SetProgress(int64_t completed);

private:
  bela::comptr<IProgressDialog> b;
};
} // namespace baulk::unscrew

#endif