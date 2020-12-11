///
#include <cstdint>

#if defined(_WIN32)
#include <intrin.h>
#endif
#include <bela/base.hpp>

#if defined(__powerpc__) || defined(__ppc__)
#ifdef __GLIBC__
#include <sys/platform/ppc.h>
#elif defined(__FreeBSD__)
#include <sys/sysctl.h>
#include <sys/types.h>
#endif
#endif

#include <thread>
#include <mutex>

namespace bela {
//
namespace base_internal {
static double GetNominalCPUFrequency() {
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) && !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
  // UWP apps don't have access to the registry and currently don't provide an
  // API informing about CPU nominal frequency.
  return 1.0;
#else
#pragma comment(lib, "advapi32.lib") // For Reg* functions.
  HKEY key;
  // Use the Reg* functions rather than the SH functions because shlwapi.dll
  // pulls in gdi32.dll which makes process destruction much more costly.
  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &key) ==
      ERROR_SUCCESS) {
    DWORD type = 0;
    DWORD data = 0;
    DWORD data_size = sizeof(data);
    auto result = RegQueryValueExA(key, "~MHz", 0, &type, reinterpret_cast<LPBYTE>(&data), &data_size);
    RegCloseKey(key);
    if (result == ERROR_SUCCESS && type == REG_DWORD && data_size == sizeof(data)) {
      return data * 1e6; // Value is MHz.
    }
  }
  return 1.0;
#endif                               // WINAPI_PARTITION_APP && !WINAPI_PARTITION_DESKTOP
}
std::once_flag init_nominal_cpu_frequency_once;
static double nominal_cpu_frequency = 1.0;
double NominalCPUFrequency() {
  std::call_once(init_nominal_cpu_frequency_once, [] { nominal_cpu_frequency = GetNominalCPUFrequency(); });
  return nominal_cpu_frequency;
};

#if defined(__i386__)

int64_t UnscaledCycleClockNow() {
  int64_t ret;
  __asm__ volatile("rdtsc" : "=A"(ret));
  return ret;
}

double UnscaledCycleClockFrequency() { return base_internal::NominalCPUFrequency(); }

#elif defined(__x86_64__)

int64_t UnscaledCycleClockNow() {
  uint64_t low, high;
  __asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
  return (high << 32) | low;
}

double UnscaledCycleClockFrequency() { return base_internal::NominalCPUFrequency(); }

#elif defined(__powerpc__) || defined(__ppc__)

int64_t UnscaledCycleClockNow() {
#ifdef __GLIBC__
  return __ppc_get_timebase();
#else
#ifdef __powerpc64__
  int64_t tbr;
  asm volatile("mfspr %0, 268" : "=r"(tbr));
  return tbr;
#else
  int32_t tbu, tbl, tmp;
  asm volatile("0:\n"
               "mftbu %[hi32]\n"
               "mftb %[lo32]\n"
               "mftbu %[tmp]\n"
               "cmpw %[tmp],%[hi32]\n"
               "bne 0b\n"
               : [ hi32 ] "=r"(tbu), [ lo32 ] "=r"(tbl), [ tmp ] "=r"(tmp));
  return (static_cast<int64_t>(tbu) << 32) | tbl;
#endif
#endif
}

double UnscaledCycleClockFrequency() {
#ifdef __GLIBC__
  return __ppc_get_timebase_freq();
#elif defined(__FreeBSD__)
  static once_flag init_timebase_frequency_once;
  static double timebase_frequency = 0.0;
  base_internal::LowLevelCallOnce(&init_timebase_frequency_once, [&]() {
    size_t length = sizeof(timebase_frequency);
    sysctlbyname("kern.timecounter.tc.timebase.frequency", &timebase_frequency, &length, nullptr, 0);
  });
  return timebase_frequency;
#else
#error Must implement UnscaledCycleClockFrequency()
#endif
}

#elif defined(__aarch64__)

// System timer of ARMv8 runs at a different frequency than the CPU's.
// The frequency is fixed, typically in the range 1-50MHz.  It can be
// read at CNTFRQ special register.  We assume the OS has set up
// the virtual timer properly.
int64_t UnscaledCycleClockNow() {
  int64_t virtual_timer_value;
  asm volatile("mrs %0, cntvct_el0" : "=r"(virtual_timer_value));
  return virtual_timer_value;
}

double UnscaledCycleClockFrequency() {
  uint64_t aarch64_timer_frequency;
  asm volatile("mrs %0, cntfrq_el0" : "=r"(aarch64_timer_frequency));
  return aarch64_timer_frequency;
}

#elif defined(_M_IX86) || defined(_M_X64)

#pragma intrinsic(__rdtsc)

int64_t UnscaledCycleClockNow() { return __rdtsc(); }

double UnscaledCycleClockFrequency() { return base_internal::NominalCPUFrequency(); }

#elif defined(_M_ARM)
// __rdpmccntr64
#pragma intrinsic(__rdpmccntr64)
int64_t UnscaledCycleClockNow() { return __rdpmccntr64(); }
double UnscaledCycleClockFrequency() { return base_internal::NominalCPUFrequency(); }

#elif defined(_M_ARM64)
// https://github.com/microsoft/SymCrypt/blob/master/test/indirect_call_perf/main.cpp#L21
int64_t UnscaledCycleClockNow() { return _ReadStatusReg(ARM64_PMCCNTR_EL0); }
double UnscaledCycleClockFrequency() { return base_internal::NominalCPUFrequency(); }
#endif

} // namespace base_internal
} // namespace bela