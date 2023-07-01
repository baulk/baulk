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

class last_error_context {
  bool m_dismissed = false;
  DWORD m_error = 0;

public:
  last_error_context() noexcept : last_error_context(::GetLastError()) {}

  explicit last_error_context(DWORD error) noexcept : m_error(error) {}

  last_error_context(last_error_context &&other) noexcept { operator=(std::move(other)); }

  last_error_context &operator=(last_error_context &&other) noexcept {
    m_dismissed = std::exchange(other.m_dismissed, true);
    m_error = other.m_error;

    return *this;
  }

  ~last_error_context() noexcept {
    if (!m_dismissed) {
      ::SetLastError(m_error);
    }
  }

  //! last_error_context doesn't own a concrete resource, so therefore
  //! it just disarms its destructor and returns void.
  void release() noexcept {
    BELA_ASSERT(!m_dismissed);
    m_dismissed = true;
  }

  [[nodiscard]] auto value() const noexcept { return m_error; }
};

namespace details {
typedef std::integral_constant<size_t, 0> pointer_access_all; // get(), release(), addressof(), and '&' are available
typedef std::integral_constant<size_t, 1> pointer_access_noaddress; // get() and release() are available
typedef std::integral_constant<size_t, 2> pointer_access_none;      // the raw pointer is not available

template <bool is_fn_ptr, typename close_fn_t, close_fn_t close_fn, typename pointer_storage_t>
struct close_invoke_helper {
  BELA_FORCE_INLINE static void close(pointer_storage_t value) noexcept { std::invoke(close_fn, value); }
  inline static void close_reset(pointer_storage_t value) noexcept {
    auto preserveError = last_error_context();
    std::invoke(close_fn, value);
  }
};

template <typename close_fn_t, close_fn_t close_fn, typename pointer_storage_t>
struct close_invoke_helper<true, close_fn_t, close_fn, pointer_storage_t> {
  BELA_FORCE_INLINE static void close(pointer_storage_t value) noexcept { close_fn(value); }
  inline static void close_reset(pointer_storage_t value) noexcept {
    auto preserveError = last_error_context();
    close_fn(value);
  }
};

template <typename close_fn_t, close_fn_t close_fn, typename pointer_storage_t>
using close_invoker =
    close_invoke_helper<std::is_pointer_v<close_fn_t> ? std::is_function_v<std::remove_pointer_t<close_fn_t>> : false,
                        close_fn_t, close_fn, pointer_storage_t>;

template <typename pointer_t,                             // The handle type
          typename close_fn_t,                            // The handle close function type
          close_fn_t close_fn,                            //      * and function pointer
          typename pointer_access_t = pointer_access_all, // all, noaddress or none to control pointer method access
          typename pointer_storage_t =
              pointer_t,                   // The type used to store the handle (usually the same as the handle itself)
          typename invalid_t = pointer_t,  // The invalid handle value type
          invalid_t invalid = invalid_t{}, //      * and its value (default ZERO value)
          typename pointer_invalid_t =
              std::nullptr_t> // nullptr_t if the invalid handle value is compatible with nullptr, otherwise pointer
struct resource_policy : close_invoker<close_fn_t, close_fn, pointer_storage_t> {
  typedef pointer_storage_t pointer_storage;
  typedef pointer_t pointer;
  typedef pointer_invalid_t pointer_invalid;
  typedef pointer_access_t pointer_access;
  BELA_FORCE_INLINE static pointer_storage invalid_value() { return (pointer)invalid; }
  BELA_FORCE_INLINE static bool is_valid(pointer_storage value) noexcept {
    return (static_cast<pointer>(value) != (pointer)invalid);
  }
};

// This class provides the pointer storage behind the implementation of unique_any_t utilizing the given
// resource_policy.  It is separate from unique_any_t to allow a type-specific specialization class to plug
// into the inheritance chain between unique_any_t and unique_storage.  This allows classes like unique_event
// to be a unique_any formed class, but also expose methods like SetEvent directly.

template <typename Policy> class unique_storage {
protected:
  typedef Policy policy;
  typedef typename policy::pointer_storage pointer_storage;
  typedef typename policy::pointer pointer;
  typedef unique_storage<policy> base_storage;

public:
  unique_storage() noexcept : m_ptr(policy::invalid_value()) {}

  explicit unique_storage(pointer_storage ptr) noexcept : m_ptr(ptr) {}

  unique_storage(unique_storage &&other) noexcept : m_ptr(std::move(other.m_ptr)) {
    other.m_ptr = policy::invalid_value();
  }

  ~unique_storage() noexcept {
    if (policy::is_valid(m_ptr)) {
      policy::close(m_ptr);
    }
  }

  [[nodiscard]] bool is_valid() const noexcept { return policy::is_valid(m_ptr); }

  void reset(pointer_storage ptr = policy::invalid_value()) noexcept {
    if (policy::is_valid(m_ptr)) {
      policy::close_reset(m_ptr);
    }
    m_ptr = ptr;
  }

  void reset(std::nullptr_t) noexcept {
    static_assert(std::is_same<typename policy::pointer_invalid, std::nullptr_t>::value,
                  "reset(nullptr): valid only for handle types using nullptr as the invalid value");
    reset();
  }

  [[nodiscard]] pointer get() const noexcept { return static_cast<pointer>(m_ptr); }

  pointer_storage release() noexcept {
    static_assert(!std::is_same<typename policy::pointer_access, pointer_access_none>::value,
                  "release(): the raw handle value is not available for this resource class");
    auto ptr = m_ptr;
    m_ptr = policy::invalid_value();
    return ptr;
  }

  pointer_storage *addressof() noexcept {
    static_assert(std::is_same<typename policy::pointer_access, pointer_access_all>::value,
                  "addressof(): the address of the raw handle is not available for this resource class");
    return &m_ptr;
  }

protected:
  void replace(unique_storage &&other) noexcept {
    reset(other.m_ptr);
    other.m_ptr = policy::invalid_value();
  }

private:
  pointer_storage m_ptr;
};
} // namespace details

template <typename struct_t, typename close_fn_t, close_fn_t close_fn, typename init_fn_t = std::nullptr_t,
          init_fn_t init_fn = std::nullptr_t()>
class unique_struct : public struct_t {
  using closer = details::close_invoker<close_fn_t, close_fn, struct_t *>;

public:
  //! Initializes the managed struct using the user-provided initialization function, or ZeroMemory if no function is
  //! specified
  unique_struct() { call_init(use_default_init_fn()); }

  //! Takes ownership of the struct by doing a shallow copy. Must explicitly be type struct_t
  explicit unique_struct(const struct_t &other) noexcept : struct_t(other) {}

  //! Initializes the managed struct by taking the ownership of the other managed struct
  //! Then resets the other managed struct by calling the custom close function
  unique_struct(unique_struct &&other) noexcept : struct_t(other.release()) {}

  //! Resets this managed struct by calling the custom close function and takes ownership of the other managed struct
  //! Then resets the other managed struct by calling the custom close function
  unique_struct &operator=(unique_struct &&other) noexcept {
    if (this != std::addressof(other)) {
      reset(other.release());
    }
    return *this;
  }

  //! Calls the custom close function
  ~unique_struct() noexcept { closer::close(this); }

  void reset(const unique_struct &) = delete;

  //! Resets this managed struct by calling the custom close function and begins management of the other struct
  void reset(const struct_t &other) noexcept {
    closer::close_reset(this);
    struct_t::operator=(other);
  }

  //! Resets this managed struct by calling the custom close function
  //! Then initializes this managed struct using the user-provided initialization function, or ZeroMemory if no function
  //! is specified
  void reset() noexcept {
    closer::close(this);
    call_init(use_default_init_fn());
  }

  void swap(struct_t &) = delete;

  //! Swaps the managed structs
  void swap(unique_struct &other) noexcept {
    struct_t self(*this);
    struct_t::operator=(other);
    *(other.addressof()) = self;
  }

  //! Returns the managed struct
  //! Then initializes this managed struct using the user-provided initialization function, or ZeroMemory if no function
  //! is specified
  struct_t release() noexcept {
    struct_t value(*this);
    call_init(use_default_init_fn());
    return value;
  }

  //! Returns address of the managed struct
  struct_t *addressof() noexcept { return this; }

  //! Resets this managed struct by calling the custom close function
  //! Then initializes this managed struct using the user-provided initialization function, or ZeroMemory if no function
  //! is specified Returns address of the managed struct
  struct_t *reset_and_addressof() noexcept {
    reset();
    return this;
  }

  unique_struct(const unique_struct &) = delete;
  unique_struct &operator=(const unique_struct &) = delete;
  unique_struct &operator=(const struct_t &) = delete;

private:
  typedef typename std::is_same<init_fn_t, std::nullptr_t>::type use_default_init_fn;

  void call_init(std::true_type) { RtlZeroMemory(this, sizeof(*this)); }

  void call_init(std::false_type) { init_fn(this); }
};
using unique_variant =
    unique_struct<VARIANT, decltype(&::VariantClear), ::VariantClear, decltype(&::VariantInit), ::VariantInit>;
} // namespace bela

#endif