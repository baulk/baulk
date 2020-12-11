//
#ifndef HAZEL_MACHO_HPP
#define HAZEL_MACHO_HPP
#include "hazel.hpp"

namespace hazel::macho {

#pragma pack(4)
struct FileHeader {
  uint32_t Magic;
  uint32_t Cpu;
  uint32_t SubCpu;
  uint32_t Type;
  uint32_t Ncmd;
  uint32_t Cmdsz;
  uint32_t Flags;
};
#pragma pack()

constexpr size_t fileHeaderSize32 = 7 * 4;
constexpr size_t fileHeaderSize64 = 8 * 4;

constexpr uint32_t Magic32 = 0xfeedface;
constexpr uint32_t Magic64 = 0xfeedfacf;
constexpr uint32_t MagicFat = 0xcafebabe;

constexpr uint32_t MachoObject = 1;
constexpr uint32_t MachoExec = 2;
constexpr uint32_t MachoDylib = 6;
constexpr uint32_t MachoBundle = 8;
constexpr uint32_t Machine64 = 0x01000000;

enum Machine : uint32_t {
  VAX = 1,
  MC680x0 = 0,
  I386 = 7,
  MC98000 = 9,
  HPPA = 11,
  MC88000 = 13,
  ARM = 12,
  SPARC = 14,
  I860 = 15,
  POWERPC = 18,
  AMD64 = I386 | Machine64,
  ARM64 = ARM | Machine64,
  POWERPC64 = POWERPC | Machine64,
};

enum LoadCmd : uint32_t {
  LoadCmdSegment = 0x1,
  LoadCmdSymtab = 0x2,
  LoadCmdThread = 0x4,
  LoadCmdUnixThread = 0x5, // thread+stack
  LoadCmdDysymtab = 0xb,
  LoadCmdDylib = 0xc,    // load dylib command
  LoadCmdDylinker = 0xf, // id dylinker command (not load dylinker command)
  LoadCmdSegment64 = 0x19,
  LoadCmdRpath = 0x8000001c,
};

enum MachoFlags : uint32_t {
  FlagNoUndefs = 0x1,
  FlagIncrLink = 0x2,
  FlagDyldLink = 0x4,
  FlagBindAtLoad = 0x8,
  FlagPrebound = 0x10,
  FlagSplitSegs = 0x20,
  FlagLazyInit = 0x40,
  FlagTwoLevel = 0x80,
  FlagForceFlat = 0x100,
  FlagNoMultiDefs = 0x200,
  FlagNoFixPrebinding = 0x400,
  FlagPrebindable = 0x800,
  FlagAllModsBound = 0x1000,
  FlagSubsectionsViaSymbols = 0x2000,
  FlagCanonical = 0x4000,
  FlagWeakDefines = 0x8000,
  FlagBindsToWeak = 0x10000,
  FlagAllowStackExecution = 0x20000,
  FlagRootSafe = 0x40000,
  FlagSetuidSafe = 0x80000,
  FlagNoReexportedDylibs = 0x100000,
  FlagPIE = 0x200000,
  FlagDeadStrippableDylib = 0x400000,
  FlagHasTLVDescriptors = 0x800000,
  FlagNoHeapExecution = 0x1000000,
  FlagAppExtensionSafe = 0x2000000,
};

class File {
public:
  File(HANDLE fd_) : fd(fd_) {}
  File(const File &) = delete;
  File &operator=(const File &) = delete;

private:
  HANDLE fd{INVALID_HANDLE_VALUE};
};
} // namespace hazel::macho

#endif