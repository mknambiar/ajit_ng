#include "ajit_memory_shim.h"
#include "ajit_thread_bridge.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

// Shim bridge between the C++ wrapper and sitar port helpers.

// Port wrappers (implemented in sitar_port_wrapper.cpp)
extern "C" {
    bool pushbool(void *obj, bool value, bool sync);
    bool pullbool(void *obj, bool *value, bool sync);
    bool pushchar(void *obj, uint8_t value, bool sync);
    bool pullchar(void *obj, uint8_t *value, bool sync);
    bool pushword(void *obj, uint32_t value, bool sync);
    bool pullword(void *obj, uint32_t *value, bool sync);
    bool pushdword(void *obj, uint64_t value, bool sync);
    bool pulldword(void *obj, uint64_t *value, bool sync);
}

// Cop.sitar-style helpers (adapted from SSL_MarchCRamTest.c)
enum { SHIM_MAX_TB = 8 };
static void* g_act_port[SHIM_MAX_TB] = {};
static void* g_wr_port[SHIM_MAX_TB] = {};
static void* g_addr_port[SHIM_MAX_TB] = {};
static void* g_data_in_port[SHIM_MAX_TB] = {};   // read data from memory
static void* g_data_out_port[SHIM_MAX_TB] = {};  // write data to memory
static void* g_bm_port[SHIM_MAX_TB] = {};
static void* g_irq_port[SHIM_MAX_TB] = {};
static void* g_coh_fill_kind_port[SHIM_MAX_TB] = {};
static void* g_coh_fill_pa_line_port[SHIM_MAX_TB] = {};
static void* g_coh_fill_va_line_port[SHIM_MAX_TB] = {};
static void* g_coh_icache_inval_port[SHIM_MAX_TB] = {};
static void* g_coh_dcache_inval_port[SHIM_MAX_TB] = {};
static uint8_t g_irq_level[SHIM_MAX_TB] = {};
static bool g_ports_set[SHIM_MAX_TB] = {};
static bool g_console_inited = false;
static int g_stdin_flags = -1;
static FILE* g_console_input_fp = nullptr;
static FILE* g_console_output_fp = nullptr;
static bool g_trace_console_io = false;
static bool g_console_input_paced = false;
static uint64_t g_console_output_count = 0;
static uint64_t g_console_prompt_count = 0;
static uint8_t g_console_last_byte = 0;

static int require_port(int id);

static void ensure_console_init()
{
    if (g_console_inited) return;
    const char* input_path = getenv("AJIT_CONSOLE_INPUT_FILE");
    const char* output_path = getenv("AJIT_CONSOLE_OUTPUT_FILE");

    if (input_path != nullptr && input_path[0] != 0) {
        g_console_input_fp = fopen(input_path, "rb");
    }
    if (output_path != nullptr && output_path[0] != 0) {
        g_console_output_fp = fopen(output_path, "wb");
    }

    const char* trace_console = getenv("AJIT_TRACE_CONSOLE_IO");
    if (trace_console != nullptr && trace_console[0] != 0 && strcmp(trace_console, "0") != 0) {
        g_trace_console_io = true;
    }
    const char* paced_input = getenv("AJIT_CONSOLE_INPUT_PACED");
    if (paced_input != nullptr && paced_input[0] != 0 && strcmp(paced_input, "0") != 0) {
        g_console_input_paced = true;
    }

    g_stdin_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (g_stdin_flags >= 0) {
        (void) fcntl(STDIN_FILENO, F_SETFL, g_stdin_flags | O_NONBLOCK);
    }
    g_console_inited = true;
}

// Phase-aware transaction state
enum ShimTxnKind { SHIM_IDLE = 0, SHIM_WRITE = 1, SHIM_READ = 2 };
static ShimTxnKind g_txn_kind[SHIM_MAX_TB] = {};
static bool g_req_sent[SHIM_MAX_TB] = {};
static uint8_t g_req_stage[SHIM_MAX_TB] = {};
static uint32_t g_addr[SHIM_MAX_TB] = {};
static uint64_t g_wdata[SHIM_MAX_TB] = {};
static uint8_t g_bm[SHIM_MAX_TB] = {};
static uint64_t g_rdata[SHIM_MAX_TB] = {};

static bool g_coh_fill_pending[SHIM_MAX_TB] = {};
static uint8_t g_coh_fill_stage[SHIM_MAX_TB] = {};
static uint8_t g_coh_fill_kind[SHIM_MAX_TB] = {};
static uint32_t g_coh_fill_pa_line[SHIM_MAX_TB] = {};
static uint32_t g_coh_fill_va_line[SHIM_MAX_TB] = {};

extern "C" void ajit_memory_shim_set_ports(int id,
                                            void* act_port,
                                            void* wr_port,
                                            void* addr_port,
                                            void* data_in_port,
                                            void* data_out_port,
                                            void* bm_port,
                                            void* irq_port,
                                            void* coh_fill_kind_port,
                                            void* coh_fill_pa_line_port,
                                            void* coh_fill_va_line_port,
                                            void* coh_icache_inval_port,
                                            void* coh_dcache_inval_port)
{
    if (id < 0 || id >= SHIM_MAX_TB) {
        abort();
    }
    g_act_port[id] = act_port;
    g_wr_port[id] = wr_port;
    g_addr_port[id] = addr_port;
    g_data_in_port[id] = data_in_port;
    g_data_out_port[id] = data_out_port;
    g_bm_port[id] = bm_port;
    g_irq_port[id] = irq_port;
    g_coh_fill_kind_port[id] = coh_fill_kind_port;
    g_coh_fill_pa_line_port[id] = coh_fill_pa_line_port;
    g_coh_fill_va_line_port[id] = coh_fill_va_line_port;
    g_coh_icache_inval_port[id] = coh_icache_inval_port;
    g_coh_dcache_inval_port[id] = coh_dcache_inval_port;
    g_irq_level[id] = 0;
    g_ports_set[id] = true;
}

extern "C" void ajit_memory_shim_set_irq_port(int id, void* irq_port)
{
    if (id < 0 || id >= SHIM_MAX_TB) {
        abort();
    }
    g_irq_port[id] = irq_port;
    g_irq_level[id] = 0;
    g_ports_set[id] = true;
}

extern "C" void ajit_shim_sample_irq(int id)
{
    if (id < 0 || id >= SHIM_MAX_TB || !g_ports_set[id]) {
        return;
    }
    int pid = id;
    if (g_irq_port[pid] != nullptr) {
        uint8_t value = 0;
        if (pullchar(g_irq_port[pid], &value, false)) {
            g_irq_level[pid] = value;
        }
    }
}

extern "C" uint8_t ajit_shim_get_irq(int id)
{
    if (id < 0 || id >= SHIM_MAX_TB || !g_ports_set[id]) {
        return 0;
    }
    return g_irq_level[id];
}

extern "C" void ajit_shim_coherence_fill_begin(int id,
                                                uint8_t cache_kind,
                                                uint32_t pa_line_addr,
                                                uint32_t va_line_addr)
{
    (void) require_port(id);
    g_coh_fill_pending[id] = true;
    g_coh_fill_stage[id] = 0;
    g_coh_fill_kind[id] = cache_kind;
    g_coh_fill_pa_line[id] = pa_line_addr;
    g_coh_fill_va_line[id] = va_line_addr;
}

extern "C" int ajit_shim_coherence_fill(int id, uint64_t sim_time)
{
    int pid = require_port(id);
    if (!g_coh_fill_pending[id]) {
        return 1;
    }
    if ((sim_time & 0x1ull) == 0ull) {
        return 0;
    }

    bool ok = false;
    switch (g_coh_fill_stage[id]) {
        case 0:
            ok = pushchar(g_coh_fill_kind_port[pid], g_coh_fill_kind[id], false);
            break;
        case 1:
            ok = pushword(g_coh_fill_pa_line_port[pid], g_coh_fill_pa_line[id], false);
            break;
        case 2:
            ok = pushword(g_coh_fill_va_line_port[pid], g_coh_fill_va_line[id], false);
            break;
        default:
            ok = true;
            break;
    }

    if (!ok) {
        return 0;
    }
    g_coh_fill_stage[id]++;
    if (g_coh_fill_stage[id] >= 3) {
        g_coh_fill_pending[id] = false;
        g_coh_fill_stage[id] = 0;
        return 1;
    }
    return 0;
}

extern "C" uint32_t ajit_shim_probe_coherence_invalidate(int core_id, int icache_flag)
{
    int threads_per_core = ajit_thread_bridge_get_threads_per_core();
    int slot = core_id * ((threads_per_core > 0) ? threads_per_core : 1);
    if (slot < 0 || slot >= SHIM_MAX_TB || !g_ports_set[slot]) {
        return 0;
    }
    int pid = slot;
    uint32_t value = 0;
    void* port = icache_flag ? g_coh_icache_inval_port[pid] : g_coh_dcache_inval_port[pid];
    if (port != nullptr && pullword(port, &value, false)) {
        return value;
    }
    return 0;
}

extern "C" int ajit_shim_console_try_read(uint8_t* out)
{
    if (out == nullptr) return 0;
    ensure_console_init();

    if (g_console_input_fp != nullptr) {
        int c = fgetc(g_console_input_fp);
        if (c != EOF) {
            *out = (uint8_t) c;
            if (g_trace_console_io) {
                fprintf(stderr, "CONSOLE_IO: input byte=0x%02x '%c'\n",
                        (unsigned) *out,
                        ((*out >= 32) && (*out <= 126)) ? *out : '.');
            }
            return 1;
        }
        if (g_trace_console_io) {
            fprintf(stderr, "CONSOLE_IO: input EOF\n");
        }
        return 0;
    }

    uint8_t value = 0;
    ssize_t nread = read(STDIN_FILENO, &value, 1);
    if (nread == 1) {
        *out = value;
        if (g_trace_console_io) {
            fprintf(stderr, "CONSOLE_IO: stdin byte=0x%02x '%c'\n",
                    (unsigned) value,
                    ((value >= 32) && (value <= 126)) ? value : '.');
        }
        return 1;
    }
    if (nread < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        return 0;
    }
    return 0;
}

extern "C" void ajit_shim_console_write(uint8_t value)
{
    ensure_console_init();
    g_console_output_count++;
    if ((value == (uint8_t) '\n') && (g_console_last_byte == (uint8_t) '?')) {
        g_console_prompt_count++;
    }
    g_console_last_byte = value;
    if (g_console_output_fp != nullptr) {
        if (g_trace_console_io) {
            fprintf(stderr, "CONSOLE_IO: output byte=0x%02x '%c'\n",
                    (unsigned) value,
                    ((value >= 32) && (value <= 126)) ? value : '.');
        }
        (void) fputc((int) value, g_console_output_fp);
        (void) fflush(g_console_output_fp);
        return;
    }
    if (g_trace_console_io) {
        fprintf(stderr, "CONSOLE_IO: stdout byte=0x%02x '%c'\n",
                (unsigned) value,
                ((value >= 32) && (value <= 126)) ? value : '.');
    }
    (void) write(STDOUT_FILENO, &value, 1);
    (void) fsync(STDOUT_FILENO);
}

extern "C" uint64_t ajit_shim_console_output_count(void)
{
    ensure_console_init();
    return g_console_output_count;
}

extern "C" uint64_t ajit_shim_console_prompt_count(void)
{
    ensure_console_init();
    return g_console_prompt_count;
}

static int require_port(int id)
{
    if (id < 0 || id >= SHIM_MAX_TB || !g_ports_set[id]) {
        // fail-fast if used before initialization
        // Use abort() to avoid silent corruption.
        abort();
    }
    return id;
}

static bool PushForRead(void *mem_act_port, void *mem_wr_port, void *mem_addr_port, uint32_t addr)
{
    bool tval = true;
    bool fval = false;
    bool sync = true;
    bool ok = true;
    // Match Memory pull order: active -> write -> addr.
    ok &= pushbool(mem_act_port, tval, sync); // Activate
    ok &= pushbool(mem_wr_port, fval, sync); // Not writing
    ok &= pushword(mem_addr_port, addr, sync); // Push address
    return ok;
}

static bool writeDoubleWord(void *mem_act_port, void *mem_wr_port, void *mem_bm_port,
                            void *mem_data_in_port, void *mem_in_addr_port,
                            uint8_t bm, uint32_t addr, uint64_t data)
{
    bool tval = true;
    bool sync = true;
    bool ok = true;

    // Match Memory pull order: active -> write -> addr -> data -> byte_mask.
    ok &= pushbool(mem_act_port, tval, sync); // Activate
    ok &= pushbool(mem_wr_port, tval, sync); // writing
    ok &= pushword(mem_in_addr_port, addr, sync); // address
    ok &= pushdword(mem_data_in_port, data, sync); // data
    ok &= pushchar(mem_bm_port, bm, sync); // byte mask
    return ok;
}

extern "C" void ajit_shim_write_begin(int id, uint32_t addr, uint64_t data, uint8_t bm)
{
    (void) require_port(id);
    g_txn_kind[id] = SHIM_WRITE;
    g_req_sent[id] = false;
    g_req_stage[id] = 0;
    g_addr[id] = addr;
    g_wdata[id] = data;
    g_bm[id] = bm;
}

extern "C" int ajit_shim_write(int id, uint64_t sim_time)
{
    int pid = require_port(id);
    if (g_txn_kind[id] != SHIM_WRITE) return 1;

    bool phase = (sim_time & 0x1) ? true : false;
    if (phase && !g_req_sent[id]) {
        while (g_req_stage[id] < 5) {
            bool ok = false;
            switch (g_req_stage[id]) {
                case 0:
                    ok = pushbool(g_act_port[pid], true, false);
                    break;
                case 1:
                    ok = pushbool(g_wr_port[pid], true, false);
                    break;
                case 2:
                    ok = pushword(g_addr_port[pid], g_addr[id], false);
                    break;
                case 3:
                    ok = pushdword(g_data_out_port[pid], g_wdata[id], false);
                    break;
                case 4:
                    ok = pushchar(g_bm_port[pid], g_bm[id], false);
                    break;
                default:
                    ok = true;
                    break;
            }
            if (!ok) {
                return 0;
            }
            g_req_stage[id]++;
        }
        g_req_sent[id] = true;
        g_req_stage[id] = 0;
        return 0;
    }

    if (!phase && g_req_sent[id]) {
        uint64_t ack = 0;
        if (pulldword(g_data_in_port[pid], &ack, false)) {
            g_txn_kind[id] = SHIM_IDLE;
            g_req_sent[id] = false;
            g_req_stage[id] = 0;
            return 1;
        }
    }
    return 0;
}

extern "C" void ajit_shim_read_begin(int id, uint32_t addr)
{
    (void) require_port(id);
    g_txn_kind[id] = SHIM_READ;
    g_req_sent[id] = false;
    g_req_stage[id] = 0;
    g_addr[id] = addr;
    g_rdata[id] = 0;
}

extern "C" int ajit_shim_read(int id, uint64_t sim_time, uint64_t* out)
{
    int pid = require_port(id);
    if (g_txn_kind[id] != SHIM_READ) return 1;

    bool phase = (sim_time & 0x1) ? true : false;
    if (phase && !g_req_sent[id]) {
        while (g_req_stage[id] < 3) {
            bool ok = false;
            switch (g_req_stage[id]) {
                case 0:
                    ok = pushbool(g_act_port[pid], true, false);
                    break;
                case 1:
                    ok = pushbool(g_wr_port[pid], false, false);
                    break;
                case 2:
                    ok = pushword(g_addr_port[pid], g_addr[id], false);
                    break;
                default:
                    ok = true;
                    break;
            }
            if (!ok) {
                return 0;
            }
            g_req_stage[id]++;
        }
        g_req_sent[id] = true;
        g_req_stage[id] = 0;
        return 0;
    }

    if (!phase && g_req_sent[id]) {
        uint64_t value = 0;
        if (pulldword(g_data_in_port[pid], &value, false)) {
            g_rdata[id] = value;
            if (out) *out = value;
            g_txn_kind[id] = SHIM_IDLE;
            g_req_sent[id] = false;
            g_req_stage[id] = 0;
            return 1;
        }
    }
    return 0;
}

static uint64_t shim_get_doubleword(uint32_t addr)
{
    int pid = require_port(0);
    while (!PushForRead(g_act_port[pid], g_wr_port[pid], g_addr_port[pid], addr)) {}
    uint64_t value = 0;
    pulldword(g_data_in_port[pid], &value, true);
    return value;
}

static void shim_set_doubleword(uint32_t addr, uint64_t data, uint8_t bm)
{
    int pid = require_port(0);
    while (!writeDoubleWord(g_act_port[pid], g_wr_port[pid], g_bm_port[pid], g_data_out_port[pid], g_addr_port[pid], bm, addr, data)) {}
    // pull write-ack token
    uint64_t ack = 0;
    pulldword(g_data_in_port[pid], &ack, true);
    (void)ack;
}

extern "C" void ajit_shim_setRandomizeFlag(int val)
{
    (void)val;
}

extern "C" int ajit_shim_allocateMemory(unsigned int log_memory_size)
{
    (void)log_memory_size;
    return 1;
}

extern "C" int ajit_shim_initializeMemory(char* memoryMapFile)
{
    (void)memoryMapFile;
    return 1;
}

extern "C" void ajit_shim_setMemoryTraceFile(FILE* fp)
{
    (void)fp;
}

extern "C" uint8_t ajit_shim_getByteInMemory(uint32_t address)
{
    uint32_t base = address & ~0x7u;
    uint64_t dw = shim_get_doubleword(base);
    uint32_t offset = address & 0x7u;
    uint32_t shift = 8 * (7 - offset);
    return (uint8_t)((dw >> shift) & 0xFFu);
}

extern "C" void ajit_shim_setByteInMemory(uint32_t address, uint8_t byte)
{
    uint32_t base = address & ~0x7u;
    uint32_t offset = address & 0x7u;
    uint8_t mask = (uint8_t)(1u << (7 - offset));
    uint64_t dw = ((uint64_t)byte) << (8 * (7 - offset));
    shim_set_doubleword(base, dw, mask);
}

extern "C" uint32_t ajit_shim_getWordInMemory(uint32_t address)
{
    uint32_t base4 = address & ~0x3u;
    uint32_t base8 = base4 & ~0x7u;
    uint64_t dw = shim_get_doubleword(base8);
    uint32_t offset = base4 & 0x7u; // 0 or 4
    uint32_t shift = 8 * (7 - offset);
    uint32_t b0 = (uint32_t)((dw >> shift) & 0xFFu);
    uint32_t b1 = (uint32_t)((dw >> (shift - 8)) & 0xFFu);
    uint32_t b2 = (uint32_t)((dw >> (shift - 16)) & 0xFFu);
    uint32_t b3 = (uint32_t)((dw >> (shift - 24)) & 0xFFu);
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

extern "C" void ajit_shim_setWordInMemory(uint32_t address, uint32_t word, uint8_t byte_mask)
{
    uint32_t base4 = address & ~0x3u;
    uint32_t base8 = base4 & ~0x7u;
    uint32_t offset = base4 & 0x7u; // 0 or 4

    uint64_t dw = 0;
    uint8_t dw_mask = 0;
    for (int i = 0; i < 4; ++i) {
        int word_bit = 3 - i;
        if (byte_mask & (1u << word_bit)) {
            uint8_t byte = (uint8_t)((word >> (8 * word_bit)) & 0xFFu);
            int pos = offset + i;
            dw_mask |= (uint8_t)(1u << (7 - pos));
            dw |= ((uint64_t)byte) << (8 * (7 - pos));
        }
    }
    if (dw_mask != 0)
        shim_set_doubleword(base8, dw, dw_mask);
}

extern "C" uint64_t ajit_shim_getDoubleWordInMemory(uint32_t address)
{
    return shim_get_doubleword(address);
}

extern "C" void ajit_shim_vGetDoubleWordInMemory(uint32_t address, uint64_t* rd)
{
    if (rd) *rd = shim_get_doubleword(address);
}

extern "C" void ajit_shim_setDoubleWordInMemory(uint32_t address, uint64_t double_word, uint8_t byte_mask)
{
    shim_set_doubleword(address, double_word, byte_mask);
}

extern "C" void ajit_shim_accessMemU64 (uint8_t rwbar, uint8_t bmask, uint32_t addr, uint64_t wdata, uint64_t* rdata)
{
    if (rwbar == 0) {
        shim_set_doubleword(addr, wdata, bmask);
        if (rdata) *rdata = 0;
    } else {
        if (rdata) *rdata = shim_get_doubleword(addr);
    }
}

extern "C" void ajit_shim_getQuadWordInMemory(uint32_t address, uint64_t* data_h, uint64_t* data_l)
{
    if (data_h) *data_h = shim_get_doubleword(address);
    if (data_l) *data_l = shim_get_doubleword(address + 8);
}

extern "C" void ajit_shim_setQuadWordInMemory(uint32_t address, uint64_t data_h, uint64_t data_l)
{
    shim_set_doubleword(address, data_h, 0xFF);
    shim_set_doubleword(address + 8, data_l, 0xFF);
}
