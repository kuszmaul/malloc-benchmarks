#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void write_h_expected(char *str, size_t n, char *prefix, char *fname) {
  {
    int off = snprintf(str, n, "%s", prefix);
    assert(off < (int)n);
    str += off;
    n -= off;
  }
  for (size_t i = 0; i < strlen(fname); i++) {
    assert(n > 0);
    char c = fname[i];
    if (c == '-' || c == '.') {
      c = '_';
    } else if (isalnum(c)) {
      c = toupper(c);
    } else {
      fprintf(stderr, "Expected fname to comprise only alphanumeric and _ and .\n");
      exit(1);
    }
    *str++ = c;
    n--;
  }
  assert(n >= 2);
  *str++ = '\n';
  n--;
  *str++ = 0;
  n--;
}

/* Check include guard for a header file */
int main(int argc, char *argv[]) {
  assert(argc == 2);
  char *fname = argv[1];
  assert(strlen(fname) > 2);
  assert(strcmp(fname + strlen(fname) - 2, ".h") == 0);
  FILE *f = fopen(argv[1], "r");
  size_t llen = 20 + strlen(fname);
  char expect_line[llen];
  {

    write_h_expected(expect_line, llen, "#ifndef ", fname);
    {
      char *line = NULL;
      size_t n = 0;
      int r = getline(&line, &n, f);
      assert(r >= 0);
      if (strcmp(line, expect_line) != 0) {
        fprintf(stderr, "First line for %s: Expected %s got %s", fname, expect_line, line);
        return 1;
      }
    }
  }
  {
    char expect_second_line[10 + strlen(fname)];
    write_h_expected(expect_second_line, llen, "#define ", fname);
    {
      char *line = NULL;
      size_t n = 0;
      int r = getline(&line, &n, f);
      assert(r >= 0);
      if (strcmp(line, expect_second_line) != 0) {
        fprintf(stderr, "Second line for %s: Expected %s got %s", fname, expect_second_line, line);
        return 1;
      }
    }
  }
  return 0;
}
