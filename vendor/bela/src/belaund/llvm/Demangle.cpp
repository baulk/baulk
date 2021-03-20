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

static bool isItaniumEncoding(std::string_view MangledName) {
  size_t Pos = MangledName.find_first_not_of('_');
  // A valid Itanium encoding requires 1-4 leading underscores, followed by 'Z'.
  return Pos > 0 && Pos <= 4 && MangledName[Pos] == 'Z';
}

std::string llvm::demangle(const std::string_view MangledName) {
  char *Demangled;
  if (isItaniumEncoding(MangledName))
    Demangled = itaniumDemangle(MangledName, nullptr, nullptr, nullptr);
  else
    Demangled = microsoftDemangle(MangledName, nullptr, nullptr,
                                  nullptr, nullptr);

  if (!Demangled)
    return std::string(MangledName);

  std::string Ret = Demangled;
  std::free(Demangled);
  return Ret;
}
