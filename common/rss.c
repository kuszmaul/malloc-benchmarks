#include <assert.h>
#include <stddef.h>
#include <sys/resource.h>

#include "rss.h"

static size_t base_rss = 0;

size_t get_rss(void) {
  struct rusage ru;
  int r = getrusage(RUSAGE_SELF, &ru);
  assert(r == 0);
  return ru.ru_maxrss*1024ul;
}

void init_rss(void) {
  base_rss = get_rss();
}

size_t get_base_rss(void) {
  return base_rss;
}

size_t get_adjusted_rss(void) {
  assert(base_rss != 0);
  return get_rss() - base_rss;
}
