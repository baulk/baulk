//
#include <bela/str_replace.hpp>
#include <bela/strcat.hpp>

namespace bela {
namespace strings_internal {

using FixedMapping = std::initializer_list<std::pair<std::wstring_view, std::wstring_view>>;

// Applies the ViableSubstitutions in subs_ptr to the std::wstring_view s, and
// stores the result in *result_ptr. Returns the number of substitutions that
// occurred.
int ApplySubstitutions(std::wstring_view s,
                       std::vector<strings_internal::ViableSubstitution> *subs_ptr,
                       std::wstring *result_ptr) {
  auto &subs = *subs_ptr;
  int substitutions = 0;
  size_t pos = 0;
  while (!subs.empty()) {
    auto &sub = subs.back();
    if (sub.offset >= pos) {
      if (pos <= s.size()) {
        StrAppend(result_ptr, s.substr(pos, sub.offset - pos), sub.replacement);
      }
      pos = sub.offset + sub.old.size();
      substitutions += 1;
    }
    sub.offset = s.find(sub.old, pos);
    if (sub.offset == s.npos) {
      subs.pop_back();
    } else {
      // Insertion sort to ensure the last ViableSubstitution continues to be
      // before all the others.
      size_t index = subs.size();
      while (--index && subs[index - 1].OccursBefore(subs[index])) {
        std::swap(subs[index], subs[index - 1]);
      }
    }
  }
  result_ptr->append(s.data() + pos, s.size() - pos);
  return substitutions;
}

} // namespace strings_internal

// We can implement this in terms of the generic StrReplaceAll, but
// we must specify the template overload because C++ cannot deduce the type
// of an initializer_list parameter to a function, and also if we don't specify
// the type, we just call ourselves.
//
// Note that we implement them here, rather than in the header, so that they
// aren't inlined.

std::wstring StrReplaceAll(std::wstring_view s, strings_internal::FixedMapping replacements) {
  return StrReplaceAll<strings_internal::FixedMapping>(s, replacements);
}

int StrReplaceAll(strings_internal::FixedMapping replacements, std::wstring *target) {
  return StrReplaceAll<strings_internal::FixedMapping>(replacements, target);
}
} // namespace bela
