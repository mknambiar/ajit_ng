#include "modular_memory_init.h"

#include "memory.h"

#include <cstdio>
#include <cstdlib>

namespace {

volatile int g_memory_init_state = 0;

const char* modular_memory_mmap_file()
{
  const char* mmap_file = std::getenv("AJIT_TEST_MEMMAP");
  if (mmap_file == nullptr || mmap_file[0] == 0) {
    mmap_file = "../test_memmap_input_0.txt";
  }
  return mmap_file;
}

} // namespace

extern "C" int modular_memory_init_once(void)
{
  if (g_memory_init_state == 2) {
    return 1;
  }
  if (g_memory_init_state == 1) {
    while (g_memory_init_state == 1) {
    }
    return g_memory_init_state == 2;
  }

  if (!__sync_bool_compare_and_swap(&g_memory_init_state, 0, 1)) {
    return modular_memory_init_once();
  }

  const char* mmap_file = modular_memory_mmap_file();
  if (!allocateMemory(32)) {
    std::fprintf(stderr, "MOD-MEM ERROR: allocateMemory failed\n");
    g_memory_init_state = 0;
    return 0;
  }
  if (!initializeMemory((char*) mmap_file)) {
    std::fprintf(stderr, "MOD-MEM ERROR: initializeMemory failed for %s\n", mmap_file);
    g_memory_init_state = 0;
    return 0;
  }

  std::fprintf(stderr, "MOD-MEM initialized mmap=%s\n", mmap_file);
  g_memory_init_state = 2;
  return 1;
}
