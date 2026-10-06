#include "ajit_thread_bridge.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <string>

// Build real bridge only when Ajit CPU headers are visible in include path.
#if __has_include("RegisterFile.h") && __has_include("ThreadInterface.h")

#include "sitar_simulation.h"

extern "C" {
#include "../ajit_thread/include/AjitThread.h"
#include "Core.h"
#include "Ancillary.h"
#include "Traps.h"
#include "../memory/memory.h"
}

// Weak linkage lets this bridge compile/link even before full ajit_thread objects are linked in.
extern "C" CoreState* makeCoreState(uint32_t core_id,
                                    uint32_t number_of_threads,
                                    uint32_t isa_mode,
                                    uint32_t bp_table_size,
                                    uint32_t icache_number_of_lines,  uint32_t icache_associativity,
                                    uint32_t dcache_number_of_lines,  uint32_t dcache_associativity,
                                    uint32_t tlb0_log_mem_size, uint32_t tlb0_log_set_size,
                                    uint32_t tlb1_log_mem_size, uint32_t tlb1_log_set_size,
                                    uint32_t tlb2_log_mem_size, uint32_t tlb2_log_set_size,
                                    uint32_t tlb3_log_mem_size, uint32_t tlb3_log_set_size,
                                    uint8_t report_traps, uint32_t init_pc) __attribute__((weak));
extern "C" void setThreadSitarSimTime(ThreadState* state_ptr, uint64_t sim_time) __attribute__((weak));
extern "C" void ajit_step_dump_opcode_summary(void) __attribute__((weak));
extern "C" void ajit_step_get_contention_counters(uint64_t* icache_busy,
                                                   uint64_t* dcache_busy,
                                                   uint64_t* mmu_busy,
                                                   uint64_t* dcache_owner_busy) __attribute__((weak));
extern "C" void ajit_step_get_mmu_source_counters(uint64_t* mmu_req_ifetch,
                                                   uint64_t* mmu_req_dcache,
                                                   uint64_t* mmu_busy_ifetch,
                                                   uint64_t* mmu_busy_dcache) __attribute__((weak));
extern "C" void ajit_step_get_contention_counters_by_phase(uint64_t* icache_busy_p0,
                                                            uint64_t* icache_busy_p1,
                                                            uint64_t* dcache_busy_p0,
                                                            uint64_t* dcache_busy_p1,
                                                            uint64_t* mmu_busy_p0,
                                                            uint64_t* mmu_busy_p1,
                                                            uint64_t* dcache_owner_busy_p0,
                                                            uint64_t* dcache_owner_busy_p1) __attribute__((weak));
extern "C" void ajit_step_get_mmu_source_counters_by_phase(uint64_t* mmu_busy_ifetch_p0,
                                                            uint64_t* mmu_busy_ifetch_p1,
                                                            uint64_t* mmu_busy_dcache_p0,
                                                            uint64_t* mmu_busy_dcache_p1) __attribute__((weak));
extern "C" void ajit_step_dump_deep_bus_summary(void) __attribute__((weak));

static constexpr int kMaxCores = 4;
static constexpr int kMaxSlots = 8;

static CoreState* g_cores[kMaxCores];
static ThreadState* g_threads[kMaxSlots];
static uint8_t g_inited[kMaxSlots];
static uint8_t g_logged_real[kMaxSlots];
static uint8_t g_logged_inactive[kMaxSlots];
static volatile int g_init_state = 0;  // 0:not-started, 1:in-progress, 2:done
static int g_active_threads = -1;
static uint8_t g_active_slot[kMaxSlots] = {1,1,1,1,1,1,1,1};
static uint8_t g_active_slot_cfg_done = 0;
static uint8_t g_ta0_seen[kMaxSlots];
static uint8_t g_ta0_count[kMaxSlots];
static uint8_t g_runtime_seen[kMaxSlots];
static uint8_t g_runtime_done[kMaxSlots];
static uint64_t g_runtime_last_tick[kMaxSlots];
static uint64_t g_runtime_done_tick[kMaxSlots];
static volatile int g_ta0_stop_issued = 0;
static int g_ta0_stop_mode = -1; // -1 unset, 0 off, 1 any, 2 all
static int g_dump_regs_on_ta0 = -1; // -1 unset, 0 off, 1 on
static int g_dump_regs_on_summary = -1; // -1 unset, 0 off, 1 on
static int g_dump_memory_on_ta0 = -1; // -1 unset, 0 off, 1 on
static int g_num_cores = -1;          // 1..4
static int g_threads_per_core = -1;   // 1..2
static int g_total_model_threads = -1;
static uint8_t g_slot_valid[kMaxSlots] = {};
static int g_slot_core[kMaxSlots] = {};
static int g_slot_tid[kMaxSlots] = {};
static uint8_t g_topology_cfg_done = 0;
extern "C" uint64_t ajit_sitar_sim_time = 0;

static inline int in_range(int id) {
  return (id >= 0) && (id < kMaxSlots);
}

static uint64_t sitar_ticks_to_cpu_clock(uint64_t ticks)
{
  // Adjacent SiTAR ticks are the two port phases of one Ajit CPU clock cycle.
  return (ticks >> 1);
}

static void trim_in_place(char* s)
{
  if (s == nullptr) {
    return;
  }
  char* start = s;
  while (*start && std::isspace((unsigned char) *start)) {
    ++start;
  }
  if (start != s) {
    std::memmove(s, start, std::strlen(start) + 1);
  }
  size_t len = std::strlen(s);
  while (len > 0 && std::isspace((unsigned char) s[len - 1])) {
    s[--len] = '\0';
  }
}

static bool get_config_value_raw(const char* name, char* out, size_t out_sz);

static uint32_t get_init_pc_override()
{
  char raw[64] = {};
  const char* ev = std::getenv("AJIT_INIT_PC");
  if (ev == nullptr || ev[0] == '\0') {
    if (!get_config_value_raw("AJIT_INIT_PC", raw, sizeof(raw))) {
      return 0;
    }
    ev = raw;
  }
  if (ev == nullptr || ev[0] == '\0') {
    return 0;
  }
  char* endp = nullptr;
  unsigned long v = std::strtoul(ev, &endp, 0); // supports decimal or 0x...
  if (endp == ev) {
    std::fprintf(stderr, "BRIDGE: invalid AJIT_INIT_PC='%s', using 0x0\n", ev);
    return 0;
  }
  return static_cast<uint32_t>(v);
}

static const char* get_thread_profile()
{
  const char* ev = std::getenv("AJIT_THREAD_PROFILE");
  if (ev == nullptr || ev[0] == '\0') {
    return nullptr;
  }
  return ev;
}

static const char* get_thread_config_file()
{
  const char* ev = std::getenv("AJIT_THREAD_CONFIG_FILE");
  if (ev != nullptr && ev[0] != '\0') {
    return ev;
  }
  const char* profile = get_thread_profile();
  if (profile != nullptr && !std::strcmp(profile, "krishna")) {
    return "krishna.config";
  }
  return nullptr;
}

static bool get_config_value_raw(const char* name, char* out, size_t out_sz)
{
  const char* path = get_thread_config_file();
  if (path == nullptr) {
    return false;
  }
  FILE* fp = std::fopen(path, "r");
  if (fp == nullptr) {
    return false;
  }

  bool found = false;
  char line[256];
  while (std::fgets(line, sizeof(line), fp) != nullptr) {
    trim_in_place(line);
    if (line[0] == '\0' || line[0] == '#') {
      continue;
    }
    char* hash = std::strchr(line, '#');
    if (hash != nullptr) {
      *hash = '\0';
      trim_in_place(line);
    }
    char* eq = std::strchr(line, '=');
    if (eq == nullptr) {
      continue;
    }
    *eq = '\0';
    char* key = line;
    char* value = eq + 1;
    trim_in_place(key);
    trim_in_place(value);
    if (!std::strcmp(key, name)) {
      std::snprintf(out, out_sz, "%s", value);
      found = true;
      break;
    }
  }
  std::fclose(fp);
  return found;
}

static bool get_env_or_config_raw(const char* name, char* out, size_t out_sz)
{
  const char* ev = std::getenv(name);
  if (ev != nullptr && ev[0] != '\0') {
    std::snprintf(out, out_sz, "%s", ev);
    return true;
  }
  return get_config_value_raw(name, out, out_sz);
}

static int get_active_threads_override()
{
  char raw[64] = {};
  if (!get_env_or_config_raw("AJIT_ACTIVE_THREADS", raw, sizeof(raw))) {
    return 8;
  }
  char* endp = nullptr;
  long v = std::strtol(raw, &endp, 0);
  if (endp == raw || v <= 0) {
    std::fprintf(stderr, "BRIDGE: invalid AJIT_ACTIVE_THREADS='%s', using 8\n", raw);
    return 8;
  }
  if (v > 8) v = 8;
  return (int) v;
}

static int get_num_cores_override()
{
  char raw[64] = {};
  if (!get_env_or_config_raw("AJIT_NUM_CORES", raw, sizeof(raw))) {
    return kMaxCores;
  }
  char* endp = nullptr;
  long v = std::strtol(raw, &endp, 0);
  if (endp == raw || v <= 0) {
    std::fprintf(stderr, "BRIDGE: invalid AJIT_NUM_CORES='%s', using %d\n", raw, kMaxCores);
    return kMaxCores;
  }
  if (v > kMaxCores) v = kMaxCores;
  return (int) v;
}

static int get_threads_per_core_override()
{
  char raw[64] = {};
  if (!get_env_or_config_raw("AJIT_THREADS_PER_CORE", raw, sizeof(raw))) {
    return 2;
  }
  char* endp = nullptr;
  long v = std::strtol(raw, &endp, 0);
  if (endp == raw || (v != 1 && v != 2)) {
    std::fprintf(stderr, "BRIDGE: invalid AJIT_THREADS_PER_CORE='%s', using 2\n", raw);
    return 2;
  }
  return (int) v;
}

static uint32_t get_uint32_env(const char* name, uint32_t default_val)
{
  char raw[128] = {};
  const char* ev = std::getenv(name);
  if (ev == nullptr || ev[0] == '\0') {
    if (!get_config_value_raw(name, raw, sizeof(raw))) {
      return default_val;
    }
    ev = raw;
  }
  char* endp = nullptr;
  unsigned long v = std::strtoul(ev, &endp, 0);
  if (endp == ev) {
    std::fprintf(stderr, "BRIDGE: invalid %s='%s', using %u\n", name, ev, default_val);
    return default_val;
  }
  return static_cast<uint32_t>(v);
}

static uint32_t get_uint32_env_min1(const char* name, uint32_t default_val)
{
  char raw[128] = {};
  const char* ev = std::getenv(name);
  if (ev == nullptr || ev[0] == '\0') {
    if (!get_config_value_raw(name, raw, sizeof(raw))) {
      return default_val;
    }
    ev = raw;
  }
  char* endp = nullptr;
  unsigned long v = std::strtoul(ev, &endp, 0);
  if (endp == ev || v == 0) {
    std::fprintf(stderr, "BRIDGE: invalid %s='%s', using %u\n", name, ev, default_val);
    return default_val;
  }
  return static_cast<uint32_t>(v);
}

static uint32_t get_tlb_override(const char* name, uint32_t default_val)
{
  return get_uint32_env(name, default_val);
}

static uint32_t get_isa_mode_override()
{
  uint32_t isa = get_uint32_env("AJIT_THREAD_ISA_MODE", 32);
  if (isa != 32 && isa != 64) {
    std::fprintf(stderr, "BRIDGE: invalid AJIT_THREAD_ISA_MODE='%u', using 32\n", isa);
    return 32;
  }
  return isa;
}

static void init_topology_config_once()
{
  if (g_topology_cfg_done) {
    return;
  }
  g_topology_cfg_done = 1;
  g_num_cores = get_num_cores_override();
  g_threads_per_core = get_threads_per_core_override();
  g_total_model_threads = g_num_cores * g_threads_per_core;
  if (g_total_model_threads > kMaxSlots) {
    g_total_model_threads = kMaxSlots;
  }

  for (int i = 0; i < kMaxSlots; ++i) {
    g_slot_valid[i] = 0;
    g_slot_core[i] = -1;
    g_slot_tid[i] = -1;
  }

  if (g_threads_per_core == 1) {
    // One slot per core in contiguous order.
    for (int core = 0; core < g_num_cores; ++core) {
      int slot = core;
      if (slot >= kMaxSlots) break;
      g_slot_valid[slot] = 1;
      g_slot_core[slot] = core;
      g_slot_tid[slot] = 0;
    }
  } else {
    // Default 2 threads/core: slot i -> core i/2, tid i%2
    int slots = g_total_model_threads;
    for (int slot = 0; slot < slots; ++slot) {
      g_slot_valid[slot] = 1;
      g_slot_core[slot] = slot / 2;
      g_slot_tid[slot] = slot % 2;
    }
  }

  std::fprintf(stderr, "BRIDGE: topology cores=%d threads/core=%d total-threads=%d\n",
               g_num_cores, g_threads_per_core, g_total_model_threads);
}

static void init_active_slot_config_once()
{
  if (g_active_slot_cfg_done) {
    return;
  }
  init_topology_config_once();
  g_active_slot_cfg_done = 1;

  for (int i = 0; i < kMaxSlots; ++i) {
    g_active_slot[i] = 0;
  }

  const char* mask = std::getenv("AJIT_ACTIVE_THREAD_MASK");
  if (mask != nullptr && mask[0] != '\0') {
    char buf[128] = {};
    int bi = 0;
    for (int i = 0; mask[i] && bi < (int) sizeof(buf) - 1; ++i) {
      if (!std::isspace((unsigned char) mask[i])) {
        buf[bi++] = mask[i];
      }
    }
    buf[bi] = '\0';

    if (std::strchr(buf, ',') != nullptr) {
      // Comma-separated thread ids, e.g. "0,2,4,6"
      char* saveptr = nullptr;
      char* tok = ::strtok_r(buf, ",", &saveptr);
      while (tok != nullptr) {
        char* endp = nullptr;
        long v = std::strtol(tok, &endp, 0);
        if (endp != tok && v >= 0 && v < kMaxSlots && g_slot_valid[v]) {
          g_active_slot[v] = 1;
        }
        tok = ::strtok_r(nullptr, ",", &saveptr);
      }
    } else {
      // Bit-mask string, e.g. "10101010" => t0,t2,t4,t6 active.
      int n = (int) std::strlen(buf);
      if (n == kMaxSlots) {
        for (int i = 0; i < kMaxSlots; ++i) {
          g_active_slot[i] = (buf[i] == '1' && g_slot_valid[i]) ? 1 : 0;
        }
      } else {
        std::fprintf(stderr,
                     "BRIDGE: invalid AJIT_ACTIVE_THREAD_MASK='%s' (expected %d-bit string or csv ids); using AJIT_ACTIVE_THREADS\n",
                     buf, kMaxSlots);
      }
    }
  }

  int active = get_active_threads_override();
  if (active > g_total_model_threads) {
    active = g_total_model_threads;
  }
  g_active_threads = active;

  // If no slot was enabled by mask, fall back to contiguous slots [0..active-1].
  int any = 0;
  for (int i = 0; i < kMaxSlots; ++i) {
    if (g_active_slot[i]) {
      any = 1;
      break;
    }
  }
  if (!any) {
    int left = active;
    for (int i = 0; i < kMaxSlots; ++i) {
      if (g_slot_valid[i] && left > 0) {
        g_active_slot[i] = 1;
        left--;
      }
    }
  }

  std::fprintf(stderr, "BRIDGE: active thread slots mask=");
  for (int i = 0; i < kMaxSlots; ++i) {
    std::fprintf(stderr, "%c", g_active_slot[i] ? '1' : '0');
  }
  std::fprintf(stderr, " (AJIT_ACTIVE_THREADS=%d)\n", g_active_threads);
}

static inline int is_slot_active(int id)
{
  return in_range(id) && g_active_slot[id];
}

static int parse_ta0_stop_mode()
{
  if (g_ta0_stop_mode >= 0) {
    return g_ta0_stop_mode;
  }
  const char* ev = std::getenv("AJIT_STOP_ON_TA0");
  if (ev == nullptr || ev[0] == '\0') {
    // Legacy does not treat TA0 itself as a simulator stop condition.
    g_ta0_stop_mode = 0;
    return g_ta0_stop_mode;
  }
  char buf[16] = {};
  std::snprintf(buf, sizeof(buf), "%s", ev);
  for (int i = 0; buf[i]; ++i) {
    buf[i] = (char) std::tolower((unsigned char) buf[i]);
  }
  if (!std::strcmp(buf, "0") || !std::strcmp(buf, "off") || !std::strcmp(buf, "none")) {
    g_ta0_stop_mode = 0;
  } else if (!std::strcmp(buf, "1") || !std::strcmp(buf, "any")) {
    g_ta0_stop_mode = 1;
  } else {
    g_ta0_stop_mode = 2;
  }
  return g_ta0_stop_mode;
}

static int parse_bool_env(const char* name, int default_val)
{
  const char* ev = std::getenv(name);
  if (ev == nullptr || ev[0] == '\0') {
    return default_val;
  }
  if ((ev[0] == '0') || (ev[0] == 'n') || (ev[0] == 'N') || (ev[0] == 'f') || (ev[0] == 'F')) {
    return 0;
  }
  return 1;
}

static void maybe_init_ta0_config_once()
{
  if (g_dump_regs_on_ta0 < 0) {
    g_dump_regs_on_ta0 = parse_bool_env("AJIT_DUMP_REGS_ON_TA0", 1);
  }
  if (g_dump_regs_on_summary < 0) {
    g_dump_regs_on_summary = parse_bool_env("AJIT_DUMP_REGS_ON_SUMMARY", 1);
  }
  if (g_dump_memory_on_ta0 < 0) {
    g_dump_memory_on_ta0 = parse_bool_env("AJIT_DUMP_MEMORY_ON_TA0", 0);
  }
  (void) parse_ta0_stop_mode();
}

static void dump_selected_memory_words(const char* tag)
{
  if (!g_dump_memory_on_ta0) {
    return;
  }
  const char* ev = std::getenv("AJIT_EXPECT_MEM_ADDRS");
  if (ev == nullptr || ev[0] == '\0') {
    return;
  }

  char buf[4096] = {};
  std::snprintf(buf, sizeof(buf), "%s", ev);
  char* saveptr = nullptr;
  char* tok = ::strtok_r(buf, ",", &saveptr);
  while (tok != nullptr) {
    while (*tok && std::isspace((unsigned char) *tok)) {
      ++tok;
    }
    if (*tok) {
      char* endp = nullptr;
      unsigned long a = std::strtoul(tok, &endp, 0);
      if (endp != tok) {
        uint32_t addr = static_cast<uint32_t>(a);
        uint32_t data = getWordInMemory(addr);
        std::fprintf(stderr, "BRIDGE-MEM-%s addr=0x%08x data=0x%08x\n", tag, addr, data);
      }
    }
    tok = ::strtok_r(nullptr, ",", &saveptr);
  }
}

static int is_ta0_exit(ThreadState* ts)
{
  if (ts == nullptr) {
    return 0;
  }
  return (getBit32(ts->trap_vector, _TRAP_INSTRUCTION_) != 0) && (ts->ticc_trap_type == 0);
}

static uint8_t default_data_asi(const ThreadState* ts)
{
  if ((ts != nullptr) && ts->last_data_asi_valid) {
    return (uint8_t) (ts->last_data_asi & 0x7f);
  }
  // Legacy-style baseline expectation for normal data-space checks.
  return 0x0a;
}

static void dump_thread_registers_line(ThreadState* ts, int id, const char* tag)
{
  if (ts == nullptr || ts->register_file == nullptr) {
    return;
  }
  const uint8_t cwp = (uint8_t) getSlice32(ts->status_reg.psr, 4, 0);
  std::fprintf(stderr,
               "BRIDGE-REGS-%s t%d c%d cwp=%u psr=0x%08x wim=0x%08x tbr=0x%08x y=0x%08x pc=0x%08x npc=0x%08x fpsr=0x%08x trap=0x%08x ticc=0x%02x",
               tag,
               id, (int) ts->core_id, (unsigned) cwp,
               ts->status_reg.psr, ts->status_reg.wim, ts->status_reg.tbr, ts->status_reg.y,
               ts->status_reg.pc, ts->status_reg.npc, ts->status_reg.fsr, ts->trap_vector, ts->ticc_trap_type);
  uint8_t asi_to_print = default_data_asi(ts);
  std::fprintf(stderr, " asi=0x%02x", (unsigned) asi_to_print);
  std::fprintf(stderr, "\n");
  std::fprintf(stderr,
               "BRIDGE-REGS-%s t%d g0=0x%08x g1=0x%08x g2=0x%08x g3=0x%08x g4=0x%08x g5=0x%08x g6=0x%08x g7=0x%08x\n",
               tag, id,
               readRegister(ts->register_file, 0, cwp),
               readRegister(ts->register_file, 1, cwp),
               readRegister(ts->register_file, 2, cwp),
               readRegister(ts->register_file, 3, cwp),
               readRegister(ts->register_file, 4, cwp),
               readRegister(ts->register_file, 5, cwp),
               readRegister(ts->register_file, 6, cwp),
               readRegister(ts->register_file, 7, cwp));
  std::fprintf(stderr,
               "BRIDGE-REGS-%s t%d o0=0x%08x o1=0x%08x o2=0x%08x o3=0x%08x o4=0x%08x o5=0x%08x o6=0x%08x o7=0x%08x\n",
               tag, id,
               readRegister(ts->register_file, 8, cwp),
               readRegister(ts->register_file, 9, cwp),
               readRegister(ts->register_file, 10, cwp),
               readRegister(ts->register_file, 11, cwp),
               readRegister(ts->register_file, 12, cwp),
               readRegister(ts->register_file, 13, cwp),
               readRegister(ts->register_file, 14, cwp),
               readRegister(ts->register_file, 15, cwp));
  std::fprintf(stderr,
               "BRIDGE-REGS-%s t%d l0=0x%08x l1=0x%08x l2=0x%08x l3=0x%08x l4=0x%08x l5=0x%08x l6=0x%08x l7=0x%08x\n",
               tag, id,
               readRegister(ts->register_file, 16, cwp),
               readRegister(ts->register_file, 17, cwp),
               readRegister(ts->register_file, 18, cwp),
               readRegister(ts->register_file, 19, cwp),
               readRegister(ts->register_file, 20, cwp),
               readRegister(ts->register_file, 21, cwp),
               readRegister(ts->register_file, 22, cwp),
               readRegister(ts->register_file, 23, cwp));
  std::fprintf(stderr,
               "BRIDGE-REGS-%s t%d i0=0x%08x i1=0x%08x i2=0x%08x i3=0x%08x i4=0x%08x i5=0x%08x i6=0x%08x i7=0x%08x\n",
               tag, id,
               readRegister(ts->register_file, 24, cwp),
               readRegister(ts->register_file, 25, cwp),
               readRegister(ts->register_file, 26, cwp),
               readRegister(ts->register_file, 27, cwp),
               readRegister(ts->register_file, 28, cwp),
               readRegister(ts->register_file, 29, cwp),
               readRegister(ts->register_file, 30, cwp),
               readRegister(ts->register_file, 31, cwp));
  std::fprintf(stderr,
               "BRIDGE-REGS-%s t%d asr0=0x%08x asr1=0x%08x asr2=0x%08x asr3=0x%08x asr4=0x%08x asr5=0x%08x asr6=0x%08x asr7=0x%08x\n",
               tag, id,
               ts->status_reg.asr[0], ts->status_reg.asr[1], ts->status_reg.asr[2], ts->status_reg.asr[3],
               ts->status_reg.asr[4], ts->status_reg.asr[5], ts->status_reg.asr[6], ts->status_reg.asr[7]);
  std::fprintf(stderr,
               "BRIDGE-REGS-%s t%d asr8=0x%08x asr9=0x%08x asr10=0x%08x asr11=0x%08x asr12=0x%08x asr13=0x%08x asr14=0x%08x asr15=0x%08x\n",
               tag, id,
               ts->status_reg.asr[8], ts->status_reg.asr[9], ts->status_reg.asr[10], ts->status_reg.asr[11],
               ts->status_reg.asr[12], ts->status_reg.asr[13], ts->status_reg.asr[14], ts->status_reg.asr[15]);
  std::fprintf(stderr,
               "BRIDGE-REGS-%s t%d asr16=0x%08x asr17=0x%08x asr18=0x%08x asr19=0x%08x asr20=0x%08x asr21=0x%08x asr22=0x%08x asr23=0x%08x\n",
               tag, id,
               ts->status_reg.asr[16], ts->status_reg.asr[17], ts->status_reg.asr[18], ts->status_reg.asr[19],
               ts->status_reg.asr[20], ts->status_reg.asr[21], ts->status_reg.asr[22], ts->status_reg.asr[23]);
  std::fprintf(stderr,
               "BRIDGE-REGS-%s t%d asr24=0x%08x asr25=0x%08x asr26=0x%08x asr27=0x%08x asr28=0x%08x asr29=0x%08x asr30=0x%08x asr31=0x%08x\n",
               tag, id,
               ts->status_reg.asr[24], ts->status_reg.asr[25], ts->status_reg.asr[26], ts->status_reg.asr[27],
               ts->status_reg.asr[28], ts->status_reg.asr[29], ts->status_reg.asr[30], ts->status_reg.asr[31]);
  std::fprintf(stderr,
               "BRIDGE-REGS-%s t%d f0=0x%08x f1=0x%08x f2=0x%08x f3=0x%08x f4=0x%08x f5=0x%08x f6=0x%08x f7=0x%08x\n",
               tag, id,
               readFRegister(ts->register_file, 0), readFRegister(ts->register_file, 1),
               readFRegister(ts->register_file, 2), readFRegister(ts->register_file, 3),
               readFRegister(ts->register_file, 4), readFRegister(ts->register_file, 5),
               readFRegister(ts->register_file, 6), readFRegister(ts->register_file, 7));
  std::fprintf(stderr,
               "BRIDGE-REGS-%s t%d f8=0x%08x f9=0x%08x f10=0x%08x f11=0x%08x f12=0x%08x f13=0x%08x f14=0x%08x f15=0x%08x\n",
               tag, id,
               readFRegister(ts->register_file, 8), readFRegister(ts->register_file, 9),
               readFRegister(ts->register_file, 10), readFRegister(ts->register_file, 11),
               readFRegister(ts->register_file, 12), readFRegister(ts->register_file, 13),
               readFRegister(ts->register_file, 14), readFRegister(ts->register_file, 15));
  std::fprintf(stderr,
               "BRIDGE-REGS-%s t%d f16=0x%08x f17=0x%08x f18=0x%08x f19=0x%08x f20=0x%08x f21=0x%08x f22=0x%08x f23=0x%08x\n",
               tag, id,
               readFRegister(ts->register_file, 16), readFRegister(ts->register_file, 17),
               readFRegister(ts->register_file, 18), readFRegister(ts->register_file, 19),
               readFRegister(ts->register_file, 20), readFRegister(ts->register_file, 21),
               readFRegister(ts->register_file, 22), readFRegister(ts->register_file, 23));
  std::fprintf(stderr,
               "BRIDGE-REGS-%s t%d f24=0x%08x f25=0x%08x f26=0x%08x f27=0x%08x f28=0x%08x f29=0x%08x f30=0x%08x f31=0x%08x\n",
               tag, id,
               readFRegister(ts->register_file, 24), readFRegister(ts->register_file, 25),
               readFRegister(ts->register_file, 26), readFRegister(ts->register_file, 27),
               readFRegister(ts->register_file, 28), readFRegister(ts->register_file, 29),
               readFRegister(ts->register_file, 30), readFRegister(ts->register_file, 31));
}

static void maybe_handle_ta0(ThreadState* ts, int id)
{
  if (!in_range(id) || ts == nullptr || !is_ta0_exit(ts)) {
    return;
  }
  maybe_init_ta0_config_once();
  if (g_ta0_count[id] < 0xff) {
    g_ta0_count[id]++;
  }
  if (!g_ta0_seen[id]) {
    g_ta0_seen[id] = 1;
    g_runtime_done[id] = 1;
    g_runtime_done_tick[id] = ajit_sitar_sim_time;
    std::fprintf(stderr,
                 "BRIDGE-TA0 t%d c%d sim=%llu pc=0x%08x npc=0x%08x\n",
                 id, (int) ts->core_id,
                 (unsigned long long) ajit_sitar_sim_time,
                 ts->status_reg.pc, ts->status_reg.npc);
    if (g_dump_regs_on_ta0) {
      dump_thread_registers_line(ts, id, "TA0");
    }
  }

  const int mode = parse_ta0_stop_mode();
  int should_stop = 0;
  if (mode == 1) {
    should_stop = 1;
  } else if (mode == 2) {
    init_active_slot_config_once();
    should_stop = 1;
    for (int i = 0; i < kMaxSlots; ++i) {
      if (is_slot_active(i) && !g_ta0_seen[i]) {
        should_stop = 0;
        break;
      }
    }
  }

  if (should_stop && __sync_bool_compare_and_swap(&g_ta0_stop_issued, 0, 1)) {
    std::fprintf(stderr,
                 "BRIDGE-TA0-STOP mode=%s sim=%llu\n",
                 (mode == 1 ? "any" : "all"),
                 (unsigned long long) ajit_sitar_sim_time);
    dump_selected_memory_words("TA0");
    sitar::stop_simulation();
  }
}

static int bridge_do_init()
{
  if (!(makeCoreState && setThreadSitarSimTime)) {
    return 0;
  }

  if (g_init_state == 2) {
    return 1;
  }

  if (__sync_bool_compare_and_swap(&g_init_state, 0, 1)) {
    init_topology_config_once();
    init_active_slot_config_once();
    uint32_t init_pc = get_init_pc_override();
    uint32_t isa_mode = get_isa_mode_override();
    uint32_t bp_table_size = get_uint32_env_min1("AJIT_THREAD_BP_TABLE_SIZE", 16);
    uint32_t icache_number_of_lines = get_uint32_env_min1("AJIT_THREAD_ICACHE_NUMBER_OF_LINES", 512);
    uint32_t icache_associativity = get_uint32_env_min1("AJIT_THREAD_ICACHE_ASSOCIATIVITY", 1);
    uint32_t dcache_number_of_lines = get_uint32_env_min1("AJIT_THREAD_DCACHE_NUMBER_OF_LINES", 512);
    uint32_t dcache_associativity = get_uint32_env_min1("AJIT_THREAD_DCACHE_ASSOCIATIVITY", 1);
    uint32_t tlb0_log_mem_size = get_tlb_override("AJIT_THREAD_TLB0_LOG_MEM_SIZE", 1);
    uint32_t tlb0_log_set_size = get_tlb_override("AJIT_THREAD_TLB0_LOG_SET_SIZE", 1);
    uint32_t tlb1_log_mem_size = get_tlb_override("AJIT_THREAD_TLB1_LOG_MEM_SIZE", 3);
    uint32_t tlb1_log_set_size = get_tlb_override("AJIT_THREAD_TLB1_LOG_SET_SIZE", 3);
    uint32_t tlb2_log_mem_size = get_tlb_override("AJIT_THREAD_TLB2_LOG_MEM_SIZE", 4);
    uint32_t tlb2_log_set_size = get_tlb_override("AJIT_THREAD_TLB2_LOG_SET_SIZE", 4);
    uint32_t tlb3_log_mem_size = get_tlb_override("AJIT_THREAD_TLB3_LOG_MEM_SIZE", 6);
    uint32_t tlb3_log_set_size = get_tlb_override("AJIT_THREAD_TLB3_LOG_SET_SIZE", 3);
    std::fprintf(stderr, "BRIDGE: using init_pc=0x%08x\n", init_pc);
    std::fprintf(stderr,
                 "BRIDGE: thread cfg isa_mode=%u bp_table_size=%u icache(lines=%u assoc=%u) dcache(lines=%u assoc=%u)\n",
                 isa_mode,
                 bp_table_size,
                 icache_number_of_lines,
                 icache_associativity,
                 dcache_number_of_lines,
                 dcache_associativity);
    std::fprintf(stderr,
                 "BRIDGE: tlb cfg tlb0(mem=%u set=%u) tlb1(mem=%u set=%u) tlb2(mem=%u set=%u) tlb3(mem=%u set=%u)\n",
                 tlb0_log_mem_size,
                 tlb0_log_set_size,
                 tlb1_log_mem_size,
                 tlb1_log_set_size,
                 tlb2_log_mem_size,
                 tlb2_log_set_size,
                 tlb3_log_mem_size,
                 tlb3_log_set_size);

    for (int core_id = 0; core_id < kMaxCores; core_id++) {
      g_cores[core_id] = nullptr;
    }
    for (int core_id = 0; core_id < g_num_cores; core_id++) {
      g_cores[core_id] = makeCoreState((uint32_t) core_id,
                                       (uint32_t) g_threads_per_core,
                                       isa_mode,
                                       bp_table_size,
                                       icache_number_of_lines,
                                       icache_associativity,
                                       dcache_number_of_lines,
                                       dcache_associativity,
                                       tlb0_log_mem_size,
                                       tlb0_log_set_size,
                                       tlb1_log_mem_size,
                                       tlb1_log_set_size,
                                       tlb2_log_mem_size,
                                       tlb2_log_set_size,
                                       tlb3_log_mem_size,
                                       tlb3_log_set_size,
                                       0,    // report_traps default
                                       init_pc);
      if (g_cores[core_id] == nullptr) {
        std::fprintf(stderr, "BRIDGE: makeCoreState failed for core %d\n", core_id);
        g_init_state = 0;
        return 0;
      }
    }

    for (int i = 0; i < kMaxSlots; ++i) {
      g_threads[i] = nullptr;
      g_inited[i] = 0;
      if (!g_slot_valid[i]) {
        continue;
      }
      const int core_id = g_slot_core[i];
      const int tid = g_slot_tid[i];
      if (core_id < 0 || core_id >= g_num_cores || tid < 0 || tid >= g_threads_per_core) {
        std::fprintf(stderr, "BRIDGE: invalid slot mapping id=%d core=%d tid=%d\n", i, core_id, tid);
        g_init_state = 0;
        return 0;
      }
      g_threads[i] = g_cores[core_id]->threads[tid];
      if (g_threads[i] == nullptr) {
        std::fprintf(stderr, "BRIDGE: null thread pointer for id %d\n", i);
        g_init_state = 0;
        return 0;
      }
      g_inited[i] = 1;
    }
    g_init_state = 2;
    return 1;
  }

  // Another worker is initializing.
  return (g_init_state == 2);
}

extern "C" int ajit_thread_bridge_init(void)
{
  return bridge_do_init();
}

extern "C" int ajit_thread_bridge_get_threads_per_core(void)
{
  init_topology_config_once();
  if (g_threads_per_core <= 0) {
    return 2;
  }
  return g_threads_per_core;
}

StepTaskT<int> ajit_thread_bridge_step_task(CoroutineOwner* owner, int id, uint64_t sim_time)
{
  if (!in_range(id)) {
    co_return 0;
  }

  if (!(makeCoreState && setThreadSitarSimTime)) {
    std::fprintf(stderr, "BRIDGE[%d]: coroutine path unavailable (required symbols not linked)\n", id);
    co_return 0;
  }

  if (g_active_threads < 0) {
    init_active_slot_config_once();
  }
  if (!is_slot_active(id)) {
    if (!g_logged_inactive[id]) {
      std::fprintf(stderr, "BRIDGE[%d]: inactive by active-slot mask\n", id);
      g_logged_inactive[id] = 1;
    }
    co_return 1;
  }

  if (g_init_state != 2) {
    if (!bridge_do_init()) {
      co_return 0;
    }
  }

  ThreadState* ts = g_threads[id];
  if (ts == nullptr) {
    co_return 0;
  }

  setThreadSitarSimTime(ts, sim_time);
  ajit_sitar_sim_time = sim_time;
  g_runtime_seen[id] = 1;
  g_runtime_last_tick[id] = sim_time;

  StepTaskT<int> task = ajit_thread(owner, ts);
  int ok = 0;
  CO_AWAIT_OWNED_VAL(ok, owner, task);
  maybe_handle_ta0(ts, id);

  if (!g_logged_real[id]) {
    std::fprintf(stderr, "BRIDGE[%d]: real ajit_thread coroutine active\n", id);
    g_logged_real[id] = 1;
  }
  co_return ok;
}

extern "C" void ajit_thread_bridge_dump_summary(void)
{
  if (!(makeCoreState && setThreadSitarSimTime)) {
    std::fprintf(stderr, "BRIDGE-SUMMARY: coroutine bridge symbols unavailable\n");
    return;
  }
  if (g_init_state != 2) {
    std::fprintf(stderr, "BRIDGE-SUMMARY: bridge not initialized\n");
    return;
  }

  std::fprintf(stderr, "=== BRIDGE CACHE SUMMARY BEGIN ===\n");
  maybe_init_ta0_config_once();
  for (int id = 0; id < 8; ++id) {
    ThreadState* ts = g_threads[id];
    if (ts == nullptr || ts->icache == nullptr || ts->dcache == nullptr) {
      continue;
    }
    std::fprintf(stderr,
                 "BRIDGE-SUMMARY t%d c%u IF(a=%llu h=%llu m=%llu f=%llu) "
                 "DF(a=%llu h=%llu m=%llu rh=%llu rm=%llu wh=%llu wm=%llu by=%llu fl=%llu lk=%u)\n",
                 id, ts->core_id,
                 (unsigned long long) ts->icache->number_of_accesses,
                 (unsigned long long) ts->icache->number_of_hits,
                 (unsigned long long) ts->icache->number_of_misses,
                 (unsigned long long) ts->icache->number_of_flushes,
                 (unsigned long long) ts->dcache->number_of_accesses,
                 (unsigned long long) ts->dcache->number_of_hits,
                 (unsigned long long) ts->dcache->number_of_misses,
                 (unsigned long long) ts->dcache->number_of_read_hits,
                 (unsigned long long) ts->dcache->number_of_read_misses,
                 (unsigned long long) ts->dcache->number_of_write_hits,
                 (unsigned long long) ts->dcache->number_of_write_misses,
                 (unsigned long long) ts->dcache->number_of_bypasses,
                 (unsigned long long) ts->dcache->number_of_flushes,
                 ts->dcache->number_of_locked_accesses);
    std::fprintf(stderr,
                 "BRIDGE-THREAD-STATS t%d c%u instructions=%llu traps=%u cti=%u "
                 "bp-mispredicts=%u ras-push=%u ras-pop=%u ras-mispredicts=%u "
                 "cycle-estimate=%llu\n",
                 id, ts->core_id,
                 (unsigned long long) ts->num_instructions_executed,
                 (unsigned) ts->num_traps,
                 (unsigned) ts->branch_predictor.branch_count,
                 (unsigned) ts->branch_predictor.mispredicts,
                 (unsigned) ts->return_address_stack.push_count,
                 (unsigned) ts->return_address_stack.pop_count,
                 (unsigned) ts->return_address_stack.mispredicts,
                 (unsigned long long) getCycleEstimate(ts));
  }
  for (int id = 0; id < 8; ++id) {
    ThreadState* ts = g_threads[id];
    if (ts == nullptr) {
      continue;
    }
    std::fprintf(stderr, "BRIDGE-TA0-STATUS t%d seen=%u\n", id, (unsigned) g_ta0_seen[id]);
    if (g_dump_regs_on_summary) {
      dump_thread_registers_line(ts, id, "END");
    }
  }
  uint64_t max_runtime_cycles = 0;
  for (int id = 0; id < kMaxSlots; ++id) {
    if (!g_slot_valid[id]) {
      continue;
    }
    const int active = is_slot_active(id) ? 1 : 0;
    const int seen = g_runtime_seen[id] ? 1 : 0;
    const int done = g_runtime_done[id] ? 1 : 0;
    const uint64_t runtime_tick =
        done ? g_runtime_done_tick[id] : (seen ? g_runtime_last_tick[id] : 0);
    const uint64_t runtime_cycles = sitar_ticks_to_cpu_clock(runtime_tick);
    if (active && seen && runtime_cycles > max_runtime_cycles) {
      max_runtime_cycles = runtime_cycles;
    }
    std::fprintf(stderr,
                 "BRIDGE-RUNTIME t%d c%d h%d active=%d seen=%d done=%d tick=%llu cycles=%llu\n",
                 id,
                 g_slot_core[id],
                 g_slot_tid[id],
                 active,
                 seen,
                 done,
                 (unsigned long long) runtime_tick,
                 (unsigned long long) runtime_cycles);
  }
  std::fprintf(stderr,
               "BRIDGE-RUNTIME total cycles=%llu\n",
               (unsigned long long) max_runtime_cycles);
  dump_selected_memory_words("END");
  if (ajit_step_get_contention_counters) {
    uint64_t icache_busy[4] = {};
    uint64_t dcache_busy[4] = {};
    uint64_t mmu_busy[4] = {};
    uint64_t dcache_owner_busy[4] = {};
    ajit_step_get_contention_counters(icache_busy,
                                      dcache_busy,
                                      mmu_busy,
                                      dcache_owner_busy);
    for (int core_id = 0; core_id < 4; ++core_id) {
      std::fprintf(stderr,
                   "BRIDGE-CONTENTION c%d icache-busy=%llu dcache-busy=%llu mmu-busy=%llu dcache-owner-busy=%llu\n",
                   core_id,
                   (unsigned long long) icache_busy[core_id],
                   (unsigned long long) dcache_busy[core_id],
                   (unsigned long long) mmu_busy[core_id],
                   (unsigned long long) dcache_owner_busy[core_id]);
    }
  }
  if (ajit_step_get_contention_counters_by_phase) {
    uint64_t icache_busy_p0[4] = {}, icache_busy_p1[4] = {};
    uint64_t dcache_busy_p0[4] = {}, dcache_busy_p1[4] = {};
    uint64_t mmu_busy_p0[4] = {}, mmu_busy_p1[4] = {};
    uint64_t dcache_owner_busy_p0[4] = {}, dcache_owner_busy_p1[4] = {};
    ajit_step_get_contention_counters_by_phase(icache_busy_p0,
                                               icache_busy_p1,
                                               dcache_busy_p0,
                                               dcache_busy_p1,
                                               mmu_busy_p0,
                                               mmu_busy_p1,
                                               dcache_owner_busy_p0,
                                               dcache_owner_busy_p1);
    for (int core_id = 0; core_id < 4; ++core_id) {
      std::fprintf(stderr,
                   "BRIDGE-CONTENTION-PHASE c%d icache-busy-p0=%llu icache-busy-p1=%llu "
                   "dcache-busy-p0=%llu dcache-busy-p1=%llu mmu-busy-p0=%llu mmu-busy-p1=%llu "
                   "dcache-owner-busy-p0=%llu dcache-owner-busy-p1=%llu\n",
                   core_id,
                   (unsigned long long) icache_busy_p0[core_id],
                   (unsigned long long) icache_busy_p1[core_id],
                   (unsigned long long) dcache_busy_p0[core_id],
                   (unsigned long long) dcache_busy_p1[core_id],
                   (unsigned long long) mmu_busy_p0[core_id],
                   (unsigned long long) mmu_busy_p1[core_id],
                   (unsigned long long) dcache_owner_busy_p0[core_id],
                   (unsigned long long) dcache_owner_busy_p1[core_id]);
    }
  }
  if (ajit_step_get_mmu_source_counters) {
    uint64_t mmu_req_ifetch[4] = {};
    uint64_t mmu_req_dcache[4] = {};
    uint64_t mmu_busy_ifetch[4] = {};
    uint64_t mmu_busy_dcache[4] = {};
    ajit_step_get_mmu_source_counters(mmu_req_ifetch,
                                      mmu_req_dcache,
                                      mmu_busy_ifetch,
                                      mmu_busy_dcache);
    for (int core_id = 0; core_id < 4; ++core_id) {
      std::fprintf(stderr,
                   "BRIDGE-MMU-SOURCE c%d req-ifetch=%llu req-dcache=%llu busy-ifetch=%llu busy-dcache=%llu\n",
                   core_id,
                   (unsigned long long) mmu_req_ifetch[core_id],
                   (unsigned long long) mmu_req_dcache[core_id],
                   (unsigned long long) mmu_busy_ifetch[core_id],
                   (unsigned long long) mmu_busy_dcache[core_id]);
    }
  }
  if (ajit_step_get_mmu_source_counters_by_phase) {
    uint64_t mmu_busy_ifetch_p0[4] = {}, mmu_busy_ifetch_p1[4] = {};
    uint64_t mmu_busy_dcache_p0[4] = {}, mmu_busy_dcache_p1[4] = {};
    ajit_step_get_mmu_source_counters_by_phase(mmu_busy_ifetch_p0,
                                               mmu_busy_ifetch_p1,
                                               mmu_busy_dcache_p0,
                                               mmu_busy_dcache_p1);
    for (int core_id = 0; core_id < 4; ++core_id) {
      std::fprintf(stderr,
                   "BRIDGE-MMU-SOURCE-PHASE c%d busy-ifetch-p0=%llu busy-ifetch-p1=%llu "
                   "busy-dcache-p0=%llu busy-dcache-p1=%llu\n",
                   core_id,
                   (unsigned long long) mmu_busy_ifetch_p0[core_id],
                   (unsigned long long) mmu_busy_ifetch_p1[core_id],
                   (unsigned long long) mmu_busy_dcache_p0[core_id],
                   (unsigned long long) mmu_busy_dcache_p1[core_id]);
    }
  }
  if (ajit_step_dump_opcode_summary) {
    ajit_step_dump_opcode_summary();
  }
  if (ajit_step_dump_deep_bus_summary) {
    ajit_step_dump_deep_bus_summary();
  }
  std::fprintf(stderr, "=== BRIDGE CACHE SUMMARY END ===\n");
}

#else

// Header path not fully available in this build: compile bridge in fallback mode.
extern "C" int ajit_thread_bridge_init(void)
{
  return 0;
}

extern "C" void ajit_thread_bridge_dump_summary(void)
{
  std::fprintf(stderr, "BRIDGE-SUMMARY: coroutine bridge unavailable (Ajit CPU headers not in include path)\n");
}

StepTaskT<int> ajit_thread_bridge_step_task(CoroutineOwner* owner, int id, uint64_t sim_time)
{
  (void) owner;
  (void) sim_time;
  static uint8_t logged[8] = {0};
  if ((id >= 0) && (id < 8) && !logged[id]) {
    std::fprintf(stderr,
                 "BRIDGE[%d]: coroutine bridge unavailable (Ajit CPU headers not in include path)\n",
                 id);
    logged[id] = 1;
  }
  co_return 0;
}

#endif
