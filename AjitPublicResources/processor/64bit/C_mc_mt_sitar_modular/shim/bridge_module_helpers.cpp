#include "bridge_module_helpers.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {
constexpr uint32_t kScratchPadMin = 0xFFFF2C00u;
constexpr uint32_t kScratchPadMax = 0xFFFF2FFCu;
constexpr uint32_t kIrcMin = 0xFFFF3000u;
constexpr uint32_t kIrcMax = 0xFFFF307Cu;
constexpr uint32_t kTimerMin = 0xFFFF3100u;
constexpr uint32_t kTimerMax = 0xFFFF3103u;
constexpr uint32_t kSerialTxMin = 0xFFFF3200u;
constexpr uint32_t kSerialTxMax = 0xFFFF321Fu;
constexpr uint32_t kSerialRxMin = 0xFFFF3220u;
constexpr uint32_t kSerialRxMax = 0xFFFF323Fu;

constexpr uint8_t kCoherenceMaxCores = 8;
constexpr uint8_t kCoherenceKinds = 2;
constexpr uint32_t kCoherenceRlutEntries = 2048;
constexpr uint32_t kCoherenceInvalidateFlag = 0x80000000u;
constexpr uint32_t kCoherenceLineMask = 0x7fffffffu;
constexpr uint32_t kSnoopFilterEntries = 8;
constexpr uint32_t kL2BytesPerLine = 64;
constexpr uint32_t kL2DwordsPerLine = kL2BytesPerLine / 8;
constexpr uint32_t kL2DefaultAssociativity = 8;

struct CoherenceRlutEntry {
    bool valid;
    uint32_t pa_line_addr;
    uint32_t va_line_addr;
};

struct L2Line {
    bool valid = false;
    uint32_t pa_tag = 0;
    uint8_t dirty[kL2DwordsPerLine] = {};
    uint64_t data[kL2DwordsPerLine] = {};
};

struct L2State {
    bool initialized = false;
    bool enabled = false;
    bool trace = false;
    uint32_t number_of_lines = 0;
    uint32_t associativity = kL2DefaultAssociativity;
    uint32_t number_of_sets = 0;
    std::vector<uint8_t> mru;
    std::vector<L2Line> lines;
    uint64_t accesses = 0;
    uint64_t read_hits = 0;
    uint64_t read_misses = 0;
    uint64_t write_hits = 0;
    uint64_t write_misses = 0;

    bool op_write = false;
    uint8_t op_byte_mask = 0xff;
    uint32_t op_addr = 0;
    uint64_t op_data = 0;
    bool op_hit = false;
    bool op_replace = false;
    uint32_t op_line_index = 0;
    uint32_t op_set_id = 0;
    uint32_t op_index_in_set = 0;
    uint32_t op_replace_base = 0;
    uint8_t op_mem_stage = 0;
    uint32_t op_wb_index = 0;
    uint32_t op_fill_index = 0;
    uint64_t op_mem_response = 0;
};

CoherenceRlutEntry g_coherence_rlut[kCoherenceKinds][kCoherenceMaxCores][kCoherenceRlutEntries] = {};
uint32_t g_coherence_rlut_next[kCoherenceKinds][kCoherenceMaxCores] = {};
bool g_snoop_filter_valid[kCoherenceMaxCores][kSnoopFilterEntries] = {};
uint32_t g_snoop_filter_line[kCoherenceMaxCores][kSnoopFilterEntries] = {};
uint32_t g_snoop_filter_next[kCoherenceMaxCores] = {};
L2State g_l2;

uint32_t parse_env_u32(const char* name, uint32_t fallback)
{
    const char* raw = std::getenv(name);
    if (raw == nullptr || raw[0] == '\0') {
        return fallback;
    }
    char* end = nullptr;
    const unsigned long parsed = std::strtoul(raw, &end, 0);
    if (end == raw) {
        return fallback;
    }
    return (uint32_t) parsed;
}

uint32_t floor_log2_u32(uint32_t value)
{
    uint32_t log = 0;
    while (value > 1u) {
        value >>= 1u;
        ++log;
    }
    return log;
}

uint64_t insert_bytes_into_u64(uint64_t old_val, uint64_t new_val, uint8_t byte_mask)
{
    uint64_t mask = 0;
    for (uint32_t i = 0; i < 8; ++i) {
        if (((byte_mask >> i) & 0x1u) != 0) {
            mask |= (0xffULL << (i * 8u));
        }
    }
    return (old_val & ~mask) | (new_val & mask);
}

void l2_init_once()
{
    if (g_l2.initialized) {
        return;
    }
    g_l2.initialized = true;

    uint32_t lines = parse_env_u32("AJIT_L2_CACHE_LINES", 0);
    if (lines == 0) {
        return;
    }

    uint32_t associativity = parse_env_u32("AJIT_L2_ASSOC", kL2DefaultAssociativity);
    if (associativity == 0) {
        associativity = kL2DefaultAssociativity;
    }
    if (associativity > lines) {
        associativity = lines;
    }

    uint32_t sets = lines / associativity;
    if (sets == 0) {
        sets = 1;
        associativity = lines;
    }
    lines = sets * associativity;

    g_l2.enabled = true;
    g_l2.trace = (parse_env_u32("AJIT_TRACE_L2", 0) != 0);
    g_l2.number_of_lines = lines;
    g_l2.associativity = associativity;
    g_l2.number_of_sets = sets;
    g_l2.mru.assign(sets, 0);
    g_l2.lines.assign(lines, L2Line{});

    std::fprintf(stderr,
                 "Info: sitar L2 cache enabled lines=%u associativity=%u sets=%u log_lines=%u\n",
                 g_l2.number_of_lines,
                 g_l2.associativity,
                 g_l2.number_of_sets,
                 floor_log2_u32(g_l2.number_of_lines));
}

uint32_t l2_set_id(uint32_t pa)
{
    return (pa >> 6u) % g_l2.number_of_sets;
}

uint32_t l2_offset(uint32_t pa)
{
    return (pa >> 3u) & 0x7u;
}

uint32_t l2_pa_tag(uint32_t pa)
{
    return pa >> (6u + floor_log2_u32(g_l2.number_of_sets));
}

uint32_t l2_line_pa(uint32_t pa_tag, uint32_t set_id)
{
    return (pa_tag << (6u + floor_log2_u32(g_l2.number_of_sets))) | (set_id << 6u);
}

bool l2_lookup(uint32_t pa, uint32_t* line_index, uint32_t* index_in_set)
{
    const uint32_t set_id = l2_set_id(pa);
    const uint32_t tag = l2_pa_tag(pa);
    for (uint32_t i = 0; i < g_l2.associativity; ++i) {
        const uint32_t candidate = (set_id * g_l2.associativity) + i;
        const L2Line& line = g_l2.lines[candidate];
        if (line.valid && line.pa_tag == tag) {
            *line_index = candidate;
            *index_in_set = i;
            return true;
        }
    }
    return false;
}

uint32_t l2_allocate_line(uint32_t pa, bool* hit, bool* replace, uint32_t* replace_base, uint32_t* index_in_set)
{
    uint32_t line_index = 0;
    if (l2_lookup(pa, &line_index, index_in_set)) {
        *hit = true;
        *replace = false;
        *replace_base = 0;
        return line_index;
    }

    const uint32_t set_id = l2_set_id(pa);
    for (uint32_t i = 0; i < g_l2.associativity; ++i) {
        const uint32_t candidate = (set_id * g_l2.associativity) + i;
        if (!g_l2.lines[candidate].valid) {
            *hit = false;
            *replace = false;
            *replace_base = 0;
            *index_in_set = i;
            return candidate;
        }
    }

    const uint32_t victim_index_in_set = (g_l2.mru[set_id] + 1u) % g_l2.associativity;
    const uint32_t victim = (set_id * g_l2.associativity) + victim_index_in_set;
    *hit = false;
    *replace = true;
    *replace_base = l2_line_pa(g_l2.lines[victim].pa_tag, set_id);
    *index_in_set = victim_index_in_set;
    return victim;
}

void l2_start_operation(bool write_val, uint8_t byte_mask, uint32_t addr, uint64_t data)
{
    g_l2.op_write = write_val;
    g_l2.op_byte_mask = byte_mask;
    g_l2.op_addr = addr;
    g_l2.op_data = data;
    g_l2.op_mem_stage = 0;
    g_l2.op_wb_index = 0;
    g_l2.op_fill_index = 0;
    g_l2.op_mem_response = 0;

    uint32_t line_index = 0;
    uint32_t index_in_set = 0;
    bool hit = false;
    bool replace = false;
    uint32_t replace_base = 0;
    line_index = l2_allocate_line(addr, &hit, &replace, &replace_base, &index_in_set);

    g_l2.op_line_index = line_index;
    g_l2.op_hit = hit;
    g_l2.op_replace = replace;
    g_l2.op_set_id = l2_set_id(addr);
    g_l2.op_index_in_set = index_in_set;
    g_l2.op_replace_base = replace_base;
    g_l2.accesses++;
    if (hit) {
        if (write_val) g_l2.write_hits++;
        else g_l2.read_hits++;
    } else {
        if (write_val) g_l2.write_misses++;
        else g_l2.read_misses++;
    }
}

bool l2_line_has_dirty_dwords(uint32_t line_index)
{
    const L2Line& line = g_l2.lines[line_index];
    if (!line.valid) {
        return false;
    }
    for (uint32_t i = 0; i < kL2DwordsPerLine; ++i) {
        if (line.dirty[i] != 0) {
            return true;
        }
    }
    return false;
}

uint64_t l2_finish_access()
{
    L2Line& line = g_l2.lines[g_l2.op_line_index];
    g_l2.mru[g_l2.op_set_id] = (uint8_t) g_l2.op_index_in_set;
    const uint32_t offset = l2_offset(g_l2.op_addr);
    if (!g_l2.op_write) {
        return line.data[offset];
    }
    line.data[offset] = insert_bytes_into_u64(line.data[offset], g_l2.op_data, g_l2.op_byte_mask);
    line.dirty[offset] = 1;
    return line.data[offset];
}

uint32_t bridge_lookup_coherence_rlut(uint8_t core_id, uint8_t cache_kind, uint32_t pa_line_addr)
{
    if (core_id >= kCoherenceMaxCores || cache_kind >= kCoherenceKinds) {
        return 0;
    }
    for (uint32_t i = 0; i < kCoherenceRlutEntries; ++i) {
        const CoherenceRlutEntry& e = g_coherence_rlut[cache_kind][core_id][i];
        if (e.valid && e.pa_line_addr == pa_line_addr) {
            return kCoherenceInvalidateFlag | (e.va_line_addr & kCoherenceLineMask);
        }
    }
    return 0;
}

void bridge_update_coherence_rlut(uint8_t core_id,
                                  uint8_t cache_kind,
                                  uint32_t pa_line_addr,
                                  uint32_t va_line_addr)
{
    if (core_id >= kCoherenceMaxCores || cache_kind >= kCoherenceKinds) {
        return;
    }
    for (uint32_t i = 0; i < kCoherenceRlutEntries; ++i) {
        CoherenceRlutEntry& e = g_coherence_rlut[cache_kind][core_id][i];
        if (e.valid && e.pa_line_addr == pa_line_addr) {
            e.va_line_addr = va_line_addr;
            return;
        }
    }

    CoherenceRlutEntry& victim =
        g_coherence_rlut[cache_kind][core_id][g_coherence_rlut_next[cache_kind][core_id]];
    g_coherence_rlut_next[cache_kind][core_id] =
        (g_coherence_rlut_next[cache_kind][core_id] + 1u) % kCoherenceRlutEntries;
    victim.valid = true;
    victim.pa_line_addr = pa_line_addr;
    victim.va_line_addr = va_line_addr;
}

void bridge_snoop_filter_erase(uint8_t core_id, uint32_t pa_line_addr)
{
    if (core_id >= kCoherenceMaxCores) {
        return;
    }
    for (uint32_t i = 0; i < kSnoopFilterEntries; ++i) {
        if (g_snoop_filter_valid[core_id][i] &&
            g_snoop_filter_line[core_id][i] == pa_line_addr) {
            g_snoop_filter_valid[core_id][i] = false;
        }
    }
}

bool bridge_snoop_filter_test_and_mark(uint8_t core_id, uint32_t pa_line_addr)
{
    if (core_id >= kCoherenceMaxCores) {
        return false;
    }
    for (uint32_t i = 0; i < kSnoopFilterEntries; ++i) {
        if (g_snoop_filter_valid[core_id][i] &&
            g_snoop_filter_line[core_id][i] == pa_line_addr) {
            return true;
        }
    }
    uint32_t* next = &g_snoop_filter_next[core_id];
    g_snoop_filter_valid[core_id][*next] = true;
    g_snoop_filter_line[core_id][*next] = pa_line_addr;
    *next = (*next + 1u) % kSnoopFilterEntries;
    return false;
}
}

extern "C" {
bool pullbool(void *obj, bool *value, bool sync);
bool pushbool(void *obj, bool value, bool sync);
bool pullchar(void *obj, uint8_t *value, bool sync);
bool pushchar(void *obj, uint8_t value, bool sync);
bool pullword(void *obj, uint32_t *value, bool sync);
bool pushword(void *obj, uint32_t value, bool sync);
bool pulldword(void *obj, uint64_t *value, bool sync);
bool pushdword(void *obj, uint64_t value, bool sync);
}

extern "C" bool bridge_pull_cpu_request_step(uint8_t* stage,
                                               bool* write_val,
                                               uint32_t* addr,
                                               uint64_t* data,
                                               uint8_t* bm,
                                               void* active_port,
                                               void* write_port,
                                               void* addr_port,
                                               void* data_port,
                                               void* bm_port)
{
    bool active_val = false;
    while (1) {
        switch (*stage) {
            case 0:
                if (!pullbool(active_port, &active_val, false)) return false;
                *stage = 1;
                break;
            case 1:
                if (!pullbool(write_port, write_val, false)) return false;
                *stage = 2;
                break;
            case 2:
                if (!pullword(addr_port, addr, false)) return false;
                *stage = (*write_val ? 3 : 5);
                break;
            case 3:
                if (!pulldword(data_port, data, false)) return false;
                *stage = 4;
                break;
            case 4:
                if (!pullchar(bm_port, bm, false)) return false;
                *stage = 5;
                break;
            default:
                return true;
        }
    }
}

extern "C" bool bridge_issue_mem_read_step(uint8_t* stage,
                                             void* mem_act_port,
                                             void* mem_wr_port,
                                             void* mem_addr_port,
                                             uint32_t addr)
{
    while (1) {
        switch (*stage) {
            case 0:
                if (!pushbool(mem_act_port, true, false)) return false;
                *stage = 1;
                break;
            case 1:
                if (!pushbool(mem_wr_port, false, false)) return false;
                *stage = 2;
                break;
            case 2:
                if (!pushword(mem_addr_port, addr, false)) return false;
                *stage = 3;
                break;
            default:
                return true;
        }
    }
}

extern "C" bool bridge_issue_mem_write_step(uint8_t* stage,
                                              void* mem_act_port,
                                              void* mem_wr_port,
                                              void* mem_addr_port,
                                              void* mem_data_port,
                                              void* mem_bm_port,
                                              uint32_t addr,
                                              uint64_t data,
                                              uint8_t bm)
{
    while (1) {
        switch (*stage) {
            case 0:
                if (!pushbool(mem_act_port, true, false)) return false;
                *stage = 1;
                break;
            case 1:
                if (!pushbool(mem_wr_port, true, false)) return false;
                *stage = 2;
                break;
            case 2:
                if (!pushword(mem_addr_port, addr, false)) return false;
                *stage = 3;
                break;
            case 3:
                if (!pushdword(mem_data_port, data, false)) return false;
                *stage = 4;
                break;
            case 4:
                if (!pushchar(mem_bm_port, bm, false)) return false;
                *stage = 5;
                break;
            default:
                return true;
        }
    }
}

extern "C" bool bridge_collect_mem_response(void* mem_data_port, uint64_t* value)
{
    return pulldword(mem_data_port, value, false);
}

extern "C" bool bridge_send_cpu_response_step(uint8_t* stage, void* cpu_data_port, uint64_t value)
{
    (void) stage;
    return pushdword(cpu_data_port, value, false);
}

extern "C" uint8_t bridge_decode_target(uint32_t addr)
{
    if ((addr >= kScratchPadMin) && (addr <= kScratchPadMax)) return BRIDGE_TARGET_SCRATCHPAD;
    if ((addr >= kIrcMin) && (addr <= kIrcMax)) return BRIDGE_TARGET_IRC;
    if ((addr >= kTimerMin) && (addr <= kTimerMax)) return BRIDGE_TARGET_TIMER;
    if ((addr >= kSerialTxMin) && (addr <= kSerialTxMax)) return BRIDGE_TARGET_SERIAL_TX;
    if ((addr >= kSerialRxMin) && (addr <= kSerialRxMax)) return BRIDGE_TARGET_SERIAL_RX;
    return BRIDGE_TARGET_MEMORY;
}

extern "C" bool bridge_issue_peripheral_access_step(uint8_t* stage,
                                                     void* req_port,
                                                     bool rwbar,
                                                     uint8_t byte_mask,
                                                     uint32_t addr,
                                                     uint32_t data)
{
    (void) stage;
    // Peripheral command format:
    //   [63]    = rwbar
    //   [62:59] = byte mask
    //   [58:43] = low 16 bits of MMIO address (0xFFFFxxxx range only)
    //   [31:0]  = data
    // All explicit SITAR peripherals currently live in the 0xFFFF0000 MMIO page,
    // so carrying addr[15:0] is sufficient and avoids colliding with the control bits.
    uint64_t cmd =
        ((uint64_t) (rwbar ? 1u : 0u) << 63) |
        ((uint64_t) (byte_mask & 0xFu) << 59) |
        ((uint64_t) (addr & 0xFFFFu) << 43) |
        (uint64_t) data;
    return pushdword(req_port, cmd, false);
}

extern "C" bool bridge_collect_peripheral_response(void* resp_port, uint32_t* value)
{
    return pullword(resp_port, value, false);
}

extern "C" bool bridge_issue_target_access_step(uint8_t* stage,
                                                     uint8_t target_kind,
                                                     void* sp_req_port,
                                                     void* timer_req_port,
                                                     void* irc_req_port,
                                                     void* serial_tx_req_port,
                                                     void* serial_rx_req_port,
                                                     bool rwbar,
                                                     uint8_t byte_mask,
                                                     uint32_t addr,
                                                     uint32_t data)
{
    void* req_port = serial_rx_req_port;
    if (target_kind == BRIDGE_TARGET_SCRATCHPAD) req_port = sp_req_port;
    else if (target_kind == BRIDGE_TARGET_TIMER) req_port = timer_req_port;
    else if (target_kind == BRIDGE_TARGET_IRC) req_port = irc_req_port;
    else if (target_kind == BRIDGE_TARGET_SERIAL_TX) req_port = serial_tx_req_port;
    return bridge_issue_peripheral_access_step(stage, req_port, rwbar, byte_mask, addr, data);
}

extern "C" bool bridge_collect_target_response(uint8_t target_kind,
                                                void* sp_resp_port,
                                                void* timer_resp_port,
                                                void* irc_resp_port,
                                                void* serial_tx_resp_port,
                                                void* serial_rx_resp_port,
                                                uint32_t* value)
{
    void* resp_port = serial_rx_resp_port;
    if (target_kind == BRIDGE_TARGET_SCRATCHPAD) resp_port = sp_resp_port;
    else if (target_kind == BRIDGE_TARGET_TIMER) resp_port = timer_resp_port;
    else if (target_kind == BRIDGE_TARGET_IRC) resp_port = irc_resp_port;
    else if (target_kind == BRIDGE_TARGET_SERIAL_TX) resp_port = serial_tx_resp_port;
    return bridge_collect_peripheral_response(resp_port, value);
}

extern "C" uint32_t bridge_insert_using_byte_mask32(uint32_t old_val, uint32_t new_val, uint8_t byte_mask)
{
    uint32_t wmask = 0;
    if (byte_mask & 0x1u) wmask |= 0x000000FFu;
    if (byte_mask & 0x2u) wmask |= 0x0000FF00u;
    if (byte_mask & 0x4u) wmask |= 0x00FF0000u;
    if (byte_mask & 0x8u) wmask |= 0xFF000000u;
    return (old_val & ~wmask) | (new_val & wmask);
}

extern "C" bool memory_pull_request_step(uint8_t* stage,
                                           bool* write_val,
                                           uint32_t* addr,
                                           uint64_t* data,
                                           uint8_t* bm,
                                           void* active_port,
                                           void* write_port,
                                           void* addr_port,
                                           void* data_port,
                                           void* bm_port)
{
    return bridge_pull_cpu_request_step(stage,
                                        write_val,
                                        addr,
                                        data,
                                        bm,
                                        active_port,
                                        write_port,
                                        addr_port,
                                        data_port,
                                        bm_port);
}

extern "C" bool memory_send_response_step(uint8_t* stage, void* data_out_port, uint64_t value)
{
    (void) stage;
    return pushdword(data_out_port, value, false);
}

extern "C" bool l2_cache_access_step(uint8_t* stage,
                                       uint8_t this_phase,
                                       bool write_val,
                                       uint8_t byte_mask,
                                       uint32_t addr,
                                       uint64_t data,
                                       uint64_t* response,
                                       void* mem_act_port,
                                       void* mem_wr_port,
                                       void* mem_addr_port,
                                       void* mem_data_port,
                                       void* mem_bm_port,
                                       void* mem_resp_port)
{
    l2_init_once();
    while (1) {
        switch (*stage) {
            case 0:
                if (!g_l2.enabled) {
                    g_l2.op_mem_stage = 0;
                    *stage = write_val ? 101 : 100;
                    break;
                }
                l2_start_operation(write_val, byte_mask, addr, data);
                if (g_l2.trace) {
                    std::fprintf(stderr,
                                 "SITAR-L2 %s addr=0x%08x bm=0x%02x data=0x%016llx %s\n",
                                 write_val ? "write" : "read",
                                 addr,
                                 (unsigned) byte_mask,
                                 (unsigned long long) data,
                                 g_l2.op_hit ? "hit" : "miss");
                }
                if (g_l2.op_hit) {
                    *stage = 40;
                    break;
                }
                if (g_l2.op_replace && l2_line_has_dirty_dwords(g_l2.op_line_index)) {
                    g_l2.op_wb_index = 0;
                    *stage = 10;
                    break;
                }
                *stage = 20;
                break;

            case 10: {
                L2Line& line = g_l2.lines[g_l2.op_line_index];
                while ((g_l2.op_wb_index < kL2DwordsPerLine) &&
                       (line.dirty[g_l2.op_wb_index] == 0)) {
                    ++g_l2.op_wb_index;
                }
                if (g_l2.op_wb_index >= kL2DwordsPerLine) {
                    *stage = 20;
                    break;
                }
                g_l2.op_mem_stage = 0;
                *stage = 11;
                break;
            }

            case 11: {
                if (this_phase != 1) return false;
                L2Line& line = g_l2.lines[g_l2.op_line_index];
                const uint32_t wb_addr = g_l2.op_replace_base + (g_l2.op_wb_index << 3u);
                if (!bridge_issue_mem_write_step(&g_l2.op_mem_stage,
                                                 mem_act_port,
                                                 mem_wr_port,
                                                 mem_addr_port,
                                                 mem_data_port,
                                                 mem_bm_port,
                                                 wb_addr,
                                                 line.data[g_l2.op_wb_index],
                                                 0xff)) {
                    return false;
                }
                g_l2.op_mem_stage = 0;
                *stage = 12;
                break;
            }

            case 12:
                if (this_phase != 0) return false;
                if (!bridge_collect_mem_response(mem_resp_port, &g_l2.op_mem_response)) {
                    return false;
                }
                g_l2.lines[g_l2.op_line_index].dirty[g_l2.op_wb_index] = 0;
                ++g_l2.op_wb_index;
                *stage = 10;
                break;

            case 20:
                g_l2.lines[g_l2.op_line_index].valid = true;
                g_l2.lines[g_l2.op_line_index].pa_tag = l2_pa_tag(g_l2.op_addr);
                g_l2.op_fill_index = 0;
                *stage = 21;
                break;

            case 21:
                if (g_l2.op_fill_index >= kL2DwordsPerLine) {
                    *stage = 40;
                    break;
                }
                g_l2.op_mem_stage = 0;
                *stage = 22;
                break;

            case 22: {
                if (this_phase != 1) return false;
                const uint32_t fill_base = g_l2.op_addr & ~(kL2BytesPerLine - 1u);
                const uint32_t fill_addr = fill_base + (g_l2.op_fill_index << 3u);
                if (!bridge_issue_mem_read_step(&g_l2.op_mem_stage,
                                                mem_act_port,
                                                mem_wr_port,
                                                mem_addr_port,
                                                fill_addr)) {
                    return false;
                }
                g_l2.op_mem_stage = 0;
                *stage = 23;
                break;
            }

            case 23:
                if (this_phase != 0) return false;
                if (!bridge_collect_mem_response(mem_resp_port, &g_l2.op_mem_response)) {
                    return false;
                }
                g_l2.lines[g_l2.op_line_index].data[g_l2.op_fill_index] = g_l2.op_mem_response;
                g_l2.lines[g_l2.op_line_index].dirty[g_l2.op_fill_index] = 0;
                ++g_l2.op_fill_index;
                *stage = 21;
                break;

            case 40:
                *response = l2_finish_access();
                *stage = 0;
                return true;

            case 100:
                if (this_phase != 1) return false;
                if (!bridge_issue_mem_read_step(&g_l2.op_mem_stage,
                                                mem_act_port,
                                                mem_wr_port,
                                                mem_addr_port,
                                                addr)) {
                    return false;
                }
                g_l2.op_mem_stage = 0;
                *stage = 102;
                break;

            case 101:
                if (this_phase != 1) return false;
                if (!bridge_issue_mem_write_step(&g_l2.op_mem_stage,
                                                 mem_act_port,
                                                 mem_wr_port,
                                                 mem_addr_port,
                                                 mem_data_port,
                                                 mem_bm_port,
                                                 addr,
                                                 data,
                                                 byte_mask)) {
                    return false;
                }
                g_l2.op_mem_stage = 0;
                *stage = 102;
                break;

            case 102:
                if (this_phase != 0) return false;
                if (!bridge_collect_mem_response(mem_resp_port, response)) {
                    return false;
                }
                *stage = 0;
                return true;

            default:
                *stage = 0;
                return false;
        }
    }
}

extern "C" bool peripheral_pull_request_step(uint8_t* stage,
                                               bool* rwbar,
                                               uint8_t* byte_mask,
                                               uint32_t* addr,
                                               uint32_t* data,
                                               void* req_port)
{
    uint64_t cmd = 0;
    if (!pulldword(req_port, &cmd, false)) return false;
    *rwbar = ((cmd >> 63) & 0x1u) != 0;
    *byte_mask = (uint8_t) ((cmd >> 59) & 0xFu);
    *addr = 0xFFFF0000u | (uint32_t) ((cmd >> 43) & 0xFFFFu);
    *data = (uint32_t) cmd;
    *stage = 1;
    return true;
}

extern "C" bool peripheral_send_response_step(uint8_t* stage, void* resp_port, uint32_t value)
{
    (void) stage;
    return pushword(resp_port, value, false);
}

extern "C" bool signal_pull_u8(void* port, uint8_t* value)
{
    return pullchar(port, value, false);
}

extern "C" bool signal_push_u8(void* port, uint8_t value)
{
    return pushchar(port, value, false);
}

extern "C" bool signal_pull_u32(void* port, uint32_t* value)
{
    return pullword(port, value, false);
}

extern "C" bool signal_push_u32(void* port, uint32_t value)
{
    return pushword(port, value, false);
}

extern "C" bool bridge_poll_coherence_fill_step(uint8_t* stage,
                                                  uint8_t core_id,
                                                  uint8_t* cache_kind,
                                                  uint32_t* pa_line_addr,
                                                  uint32_t* va_line_addr,
                                                  void* cache_kind_port,
                                                  void* pa_line_addr_port,
                                                  void* va_line_addr_port)
{
    while (1) {
        switch (*stage) {
            case 0:
                if (!pullchar(cache_kind_port, cache_kind, false)) return false;
                *stage = 1;
                break;
            case 1:
                if (!pullword(pa_line_addr_port, pa_line_addr, false)) return false;
                *stage = 2;
                break;
            case 2:
                if (!pullword(va_line_addr_port, va_line_addr, false)) return false;
                bridge_update_coherence_rlut(core_id,
                                             (uint8_t) (*cache_kind & 0x1u),
                                             *pa_line_addr,
                                             *va_line_addr);
                *stage = 0;
                return true;
            default:
                *stage = 0;
                return false;
        }
    }
}

extern "C" void bridge_snoop_filter_note_memory_read(uint8_t core_id, uint32_t pa_line_addr)
{
    bridge_snoop_filter_erase(core_id, pa_line_addr);
}

extern "C" bool bridge_emit_coherence_invalidate_step(uint8_t* core_index,
                                                        uint8_t* cache_kind,
                                                        uint8_t src_core_id,
                                                        uint32_t pa_line_addr,
                                                        uint8_t num_cores,
                                                        void* icache_inval_port,
                                                        void* dcache_inval_port)
{
    if (core_index == nullptr || cache_kind == nullptr) {
        return true;
    }
    if (*core_index >= num_cores) {
        return true;
    }
    if (*core_index == src_core_id) {
        *cache_kind = 0;
        *core_index = (uint8_t) (*core_index + 1u);
        return (*core_index >= num_cores);
    }

    const uint8_t kind = (uint8_t) (*cache_kind & 0x1u);
    const bool snoop_checked = ((*cache_kind & 0x80u) != 0);
    if (kind == 0 && !snoop_checked && bridge_snoop_filter_test_and_mark(*core_index, pa_line_addr)) {
        *cache_kind = 0;
        *core_index = (uint8_t) (*core_index + 1u);
        return (*core_index >= num_cores);
    }
    if (kind == 0) {
        *cache_kind = (uint8_t) (*cache_kind | 0x80u);
    }

    const uint32_t invalidate_word = bridge_lookup_coherence_rlut(*core_index, kind, pa_line_addr);
    if (invalidate_word != 0) {
        void* port = (kind == 0) ? dcache_inval_port : icache_inval_port;
        if (!pushword(port, invalidate_word, false)) {
            return false;
        }
    }

    if (kind == 0) {
        *cache_kind = 0x81;
    } else {
        *cache_kind = 0;
        *core_index = (uint8_t) (*core_index + 1u);
    }
    return (*core_index >= num_cores);
}
