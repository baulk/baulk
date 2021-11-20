//===-- Demangle.cpp - Common demangling functions ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file This file contains definitions of common demangling functions.
///
//===----------------------------------------------------------------------===//

#include "Demangle.h"
#include <string_view>
#include <cstdlib>
#include <cstring>

static bool isItaniumEncoding(std::string_view MangledName) {
  return MangledName.starts_with("_Z") || MangledName.starts_with("___Z");
}

static bool isRustEncoding(std::string_view MangledName) { return MangledName.starts_with("_R"); }

static bool isDLangEncoding(std::string_view MangledName) {
  return MangledName.starts_with("_D");;
}

std::string llvm::demangle(std::string_view MangledName) {
  std::string Result;
  if (nonMicrosoftDemangle(MangledName, Result))
    return Result;
  if (MangledName[0] == '_' && nonMicrosoftDemangle(MangledName.substr(1), Result))
    return Result;
  if (char *Demangled = microsoftDemangle(MangledName, nullptr, nullptr, nullptr, nullptr)) {
    Result = Demangled;
    std::free(Demangled);
    return Result;
  }
  return std::string(MangledName);
}

bool llvm::nonMicrosoftDemangle(const std::string_view MangledName, std::string &Result) {
  char *Demangled = nullptr;
  if (isItaniumEncoding(MangledName))
    Demangled = itaniumDemangle(MangledName, nullptr, nullptr, nullptr);
  else if (isRustEncoding(MangledName))
    Demangled = rustDemangle(MangledName, nullptr, nullptr, nullptr);
  else if (isDLangEncoding(MangledName))
    Demangled = dlangDemangle(MangledName);

  if (!Demangled)
    return false;

  Result = Demangled;
  std::free(Demangled);
  return true;
}
