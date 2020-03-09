////
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

size_t escape_length(char *s, size_t *rlen, bool *hp) {
  size_t len = 0;
  size_t i = 0;
  bool hasspace = false;
  len = strlen(s);
  if (len == 0) {
    // ""
    return 2;
  }
  size_t n = len;
  for (; i < len; i++) {
    switch (s[i]) {
    case '"':
    case '\\':
      n++;
      break;
    case ' ':
    case '\t':
      hasspace = true;
      break;
    default:
      break;
    }
  }
  *rlen = len;
  if (hasspace) {
    n += 2;
  }
  *hp = hasspace;
  return n;
}

bool argv_escape_join(const char *const *string_array, int array_length,
                      char **result) {
  assert(string_array);
  assert(array_length >= 0);
  assert(result);
  // Determine the length of the concatenated string first.
  size_t string_length = 1; // Count the null terminator.

  for (int i = 0; i < array_length; i++) {
    size_t rlen = 0; // NOT use
    bool hp = false; // NOT use
    string_length += escape_length(string_array[i], &rlen, &hp);
    if (i < array_length - 1) {
      string_length++; // Count whitespace.
    }
  }

  char *string_out = malloc(sizeof(char) * string_length);
  if (string_out == NULL) {
    return false;
  }

  char *current = string_out;
  for (int i = 0; i < array_length; i++) {
    size_t rlen = 0;
    bool hp = false;
    size_t part_length = escape_length(string_array[i], &rlen, &hp);
    if (rlen == 0) {
      strcpy(current, "\"\"");
      current += 2;
      if (i < array_length - 1) {
        *current = ' ';
        current += 1;
      }
      continue;
    }

    if (rlen == part_length) {
      // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.strcpy)
      strcpy(current, string_array[i]);
      current += part_length;
      // We add a space after every part of the string except for the last one.
      if (i < array_length - 1) {
        *current = ' ';
        current += 1;
      }
      continue;
    }

    int slashes = 0;
    size_t j = 0;
    if (hp) {
      current[j] = '"';
      j++;
    }
    char *s = string_array[i];
    for (size_t k = 0; k < rlen; k++) {
      switch (s[k]) {
      case '\\':
        slashes++;
        current[j] = s[k];
        break;
      case '"': {
        for (; slashes > 0; slashes--) {
          current[j] = '\\';
          j++;
        }
        current[j] = '\\';
        j++;
        current[j] = s[k];
      } break;
      default:
        slashes = 0;
        current[j] = s[k];
        break;
      }
      j++;
    }
    if (hp) {
      for (; slashes > 0; slashes--) {
        current[j] = '\\';
        j++;
      }
      current[j] = '"';
      j++;
    }
    current += part_length;
    // We add a space after every part of the string except for the last one.
    if (i < array_length - 1) {
      *current = ' ';
      current += 1;
    }
  }
  *current = '\0';
  *result = string_out;
  return true;
}

int main(int argc, char **argv) {

  char *out = NULL;
  if (argv_escape_join(argv, argc, &out)) {
    fprintf(stderr, "%s\n", out);
    free(out);
  }
  return 0;
}