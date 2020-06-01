// comutils.hpp
#ifndef BELA_COMUTILS_HPP
#define BELA_COMUTILS_HPP
#include <combaseapi.h>
#include <type_traits>
#include <cassert>

namespace bela {

template <typename T> class comptr {
public:
  typedef typename std::add_lvalue_reference<T>::type element_type_reference;
  typedef T element_type;
  //! A pointer to the template type `T` being held by the com_ptr_t (what
  //! `get()` returns).
  typedef T *pointer;
  comptr() noexcept = default;
  comptr(pointer p) noexcept : ptr(p) {
    if (ptr != nullptr)
      ptr->AddRef();
  }
  comptr(const comptr<T> &other) noexcept : ptr(other.ptr) {
    if (ptr != nullptr)
      ptr->AddRef();
  }
  explicit operator bool() const noexcept { return (ptr != nullptr); }
  const pointer addressof() const noexcept { return &ptr; }
  pointer addressof() noexcept { return &ptr; }
  void reset() noexcept {
    auto p = ptr;
    ptr = nullptr;
    if (p != nullptr) {
      p->Release();
    }
  }
  void reset(std::nullptr_t) noexcept { reset(); }
  void attach(pointer other) noexcept {
    auto p = ptr;
    ptr = other;
    if (p != nullptr) {
      auto ref = p->Release();
      assert((other != p) || (ref > 0));
    }
  }
  [[nodiscard]] pointer detach() noexcept {
    auto temp = ptr;
    ptr = nullptr;
    return temp;
  }
  pointer get() const { return ptr; }
  pointer *put() noexcept {
    reset();
    return &ptr;
  }
  pointer *operator&() noexcept { return put(); }
  comptr &operator=(pointer other) noexcept {
    auto p = ptr;
    ptr = other;
    if (ptr != nullptr) {
      ptr->AddRef();
    }
    if (p != nullptr) {
      p->Relase();
    }
    return *this;
  }
  comptr &operator=(const comptr &other) noexcept { return operator=(other.get()); }

  comptr &operator=(comptr &&other) noexcept {
    if (this == std::addressof(other)) {
      return *this;
    }
    attach(other.detach());
    return *this;
  }

  void swap(comptr &other) {
    auto p = ptr;
    ptr = other.ptr;
    other.ptr = p;
  }
  pointer operator->() const noexcept { return ptr; }
  element_type_reference operator*() const noexcept { return *ptr; }
  ///
  template <typename U> HRESULT query_to(U **pp) const {
    if (pp == nullptr) {
      return S_FALSE;
    }
    return ptr->QueryInterface(IID_PPV_ARGS(pp));
  }
  ~comptr() noexcept {
    if (ptr != nullptr)
      ptr->Release();
  }

private:
  pointer ptr{nullptr};
};

template <typename Class, typename Interface = IUnknown>
bela::comptr<Class> CoCreateInstance(DWORD dwClsContext = CLSCTX_INPROC_SERVER) {
  bela::comptr<Class> result;
  ::CoCreateInstance(__uuidof(Class), nullptr, dwClsContext, IID_PPV_ARGS(&result));
  return result;
}

class comstr {
public:
  comstr() { str = nullptr; }
  comstr(const comstr &src) {
    if (src.str != nullptr) {
      str = ::SysAllocStringByteLen((char *)str, ::SysStringByteLen(str));
    } else {
      str = ::SysAllocStringByteLen(NULL, 0);
    }
  }
  comstr &operator=(const comstr &src) {
    if (str != src.str) {
      ::SysFreeString(str);
      if (src.str != nullptr) {
        str = ::SysAllocStringByteLen((char *)str, ::SysStringByteLen(str));
      } else {
        str = ::SysAllocStringByteLen(NULL, 0);
      }
    }
    return *this;
  }
  operator BSTR() const { return str; }
  BSTR *operator&() throw() { return &str; }
  ~comstr() throw() { ::SysFreeString(str); }

private:
  BSTR str;
};

} // namespace bela

#endif