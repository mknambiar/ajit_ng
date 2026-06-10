#include "ajit_memory_api_shim.h"
#include "ajit_memory_shim.h"

// Keep in sync with ajit_memory_shim.cpp
#define AJIT_SHIM_MAX_TB 8

static uint8_t g_access_active[AJIT_SHIM_MAX_TB] = {0};
static uint8_t g_access_rwbar[AJIT_SHIM_MAX_TB] = {0};
static uint8_t g_access_bmask[AJIT_SHIM_MAX_TB] = {0};
static uint32_t g_access_addr[AJIT_SHIM_MAX_TB] = {0};
static uint64_t g_access_wdata[AJIT_SHIM_MAX_TB] = {0};

void ajit_api_setDoubleWord_begin(int id, uint32_t addr, uint64_t data, uint8_t bm)
{
    ajit_shim_write_begin(id, addr, data, bm);
}

int ajit_api_setDoubleWord(int id, uint64_t sim_time)
{
    return ajit_shim_write(id, sim_time);
}

void ajit_api_getDoubleWord_begin(int id, uint32_t addr)
{
    ajit_shim_read_begin(id, addr);
}

int ajit_api_getDoubleWord(int id, uint64_t sim_time, uint64_t* out)
{
    return ajit_shim_read(id, sim_time, out);
}

void ajit_api_accessMemU64_begin(int id, uint8_t rwbar, uint8_t bmask,
                                 uint32_t addr, uint64_t wdata)
{
    if (id < 0 || id >= AJIT_SHIM_MAX_TB) return;
    g_access_active[id] = 1;
    g_access_rwbar[id] = rwbar;
    g_access_bmask[id] = bmask;
    g_access_addr[id] = addr;
    g_access_wdata[id] = wdata;

    if (rwbar == 0) {
        ajit_shim_write_begin(id, addr, wdata, bmask);
    } else {
        ajit_shim_read_begin(id, addr);
    }
}

int ajit_api_accessMemU64(int id, uint64_t sim_time, uint64_t* rdata)
{
    if (id < 0 || id >= AJIT_SHIM_MAX_TB) return 1;
    if (!g_access_active[id]) return 1;

    if (g_access_rwbar[id] == 0) {
        if (ajit_shim_write(id, sim_time)) {
            g_access_active[id] = 0;
            if (rdata) *rdata = 0;
            return 1;
        }
        return 0;
    }

    if (ajit_shim_read(id, sim_time, rdata)) {
        g_access_active[id] = 0;
        return 1;
    }
    return 0;
}
