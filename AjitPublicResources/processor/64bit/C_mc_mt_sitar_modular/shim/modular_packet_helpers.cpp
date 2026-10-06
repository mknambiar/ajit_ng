#include "modular_packet_helpers.h"
#include <cstdlib>

bool modular_async_enabled()
{
  static const bool enabled = [] {
    const char* v = std::getenv("AJIT_MODULAR_ASYNC");
    return v && v[0] && v[0] != '0';
  }();
  return enabled;
}

extern "C" {
bool pullbool(void* obj, bool* value, bool sync);
bool pushbool(void* obj, bool value, bool sync);
bool pullchar(void* obj, uint8_t* value, bool sync);
bool pushchar(void* obj, uint8_t value, bool sync);
bool pullword(void* obj, uint32_t* value, bool sync);
bool pushword(void* obj, uint32_t value, bool sync);
bool pulldword(void* obj, uint64_t* value, bool sync);
bool pushdword(void* obj, uint64_t value, bool sync);
}

bool modular_pull_thread_icache_request(void* valid,
                                        void* kind,
                                        void* asi,
                                        void* context,
                                        void* addr,
                                        ModularThreadIcacheRequest* req)
{
  uint8_t stage = 0;
  return modular_pull_thread_icache_request_step(&stage,
                                                 valid,
                                                 kind,
                                                 asi,
                                                 context,
                                                 addr,
                                                 req);
}

bool modular_pull_thread_icache_request_step(uint8_t* stage,
                                             void* valid,
                                             void* kind,
                                             void* asi,
                                             void* context,
                                             void* addr,
                                             ModularThreadIcacheRequest* req)
{
  if (stage == nullptr || req == nullptr) {
    return false;
  }
  while (1) {
    bool request_valid = false;
    switch (*stage) {
      case 0:
        if (!pullbool(valid, &request_valid, false)) return false;
        if (!request_valid) {
          *stage = 0;
          return false;
        }
        *stage = 1;
        break;
      case 1:
        if (!pullchar(kind, &req->kind, false)) return false;
        *stage = 2;
        break;
      case 2:
        if (!pullchar(asi, &req->asi, false)) return false;
        *stage = 3;
        break;
      case 3:
        if (!pullchar(context, &req->context, false)) return false;
        *stage = 4;
        break;
      case 4:
        if (!pullword(addr, &req->addr, false)) return false;
        *stage = 0;
        return true;
      default:
        *stage = 0;
        return false;
    }
  }
}

bool modular_push_thread_icache_request(void* valid,
                                        void* kind,
                                        void* asi,
                                        void* context,
                                        void* addr,
                                        const ModularThreadIcacheRequest& req)
{
  uint8_t stage = 0;
  return modular_push_thread_icache_request_step(&stage,
                                                 valid,
                                                 kind,
                                                 asi,
                                                 context,
                                                 addr,
                                                 req);
}

bool modular_push_thread_icache_request_step(uint8_t* stage,
                                             void* valid,
                                             void* kind,
                                             void* asi,
                                             void* context,
                                             void* addr,
                                             const ModularThreadIcacheRequest& req)
{
  if (stage == nullptr) {
    return false;
  }
  while (1) {
    switch (*stage) {
      case 0:
        if (!pushbool(valid, true, false)) return false;
        *stage = 1;
        break;
      case 1:
        if (!pushchar(kind, req.kind, false)) return false;
        *stage = 2;
        break;
      case 2:
        if (!pushchar(asi, req.asi, false)) return false;
        *stage = 3;
        break;
      case 3:
        if (!pushchar(context, req.context, false)) return false;
        *stage = 4;
        break;
      case 4:
        if (!pushword(addr, req.addr, false)) return false;
        *stage = 0;
        return true;
      default:
        *stage = 0;
        return false;
    }
  }
}

bool modular_pull_thread_dcache_request(void* valid,
                                        void* kind,
                                        void* asi,
                                        void* context,
                                        void* addr,
                                        void* data,
                                        void* byte_mask,
                                        ModularThreadDcacheRequest* req)
{
  uint8_t stage = 0;
  return modular_pull_thread_dcache_request_step(&stage,
                                                 valid,
                                                 kind,
                                                 asi,
                                                 context,
                                                 addr,
                                                 data,
                                                 byte_mask,
                                                 req);
}

bool modular_pull_thread_dcache_request_step(uint8_t* stage,
                                             void* valid,
                                             void* kind,
                                             void* asi,
                                             void* context,
                                             void* addr,
                                             void* data,
                                             void* byte_mask,
                                             ModularThreadDcacheRequest* req)
{
  if (stage == nullptr || req == nullptr) {
    return false;
  }
  while (1) {
    bool request_valid = false;
    switch (*stage) {
      case 0:
        if (!pullbool(valid, &request_valid, false)) return false;
        if (!request_valid) {
          *stage = 0;
          return false;
        }
        *stage = 1;
        break;
      case 1:
        if (!pullchar(kind, &req->kind, false)) return false;
        *stage = 2;
        break;
      case 2:
        if (!pullchar(asi, &req->asi, false)) return false;
        *stage = 3;
        break;
      case 3:
        if (!pullchar(context, &req->context, false)) return false;
        *stage = 4;
        break;
      case 4:
        if (!pullword(addr, &req->addr, false)) return false;
        *stage = 5;
        break;
      case 5:
        if (!pulldword(data, &req->data, false)) return false;
        *stage = 6;
        break;
      case 6:
        if (!pullchar(byte_mask, &req->byte_mask, false)) return false;
        *stage = 0;
        return true;
      default:
        *stage = 0;
        return false;
    }
  }
}

bool modular_push_thread_dcache_request(void* valid,
                                        void* kind,
                                        void* asi,
                                        void* context,
                                        void* addr,
                                        void* data,
                                        void* byte_mask,
                                        const ModularThreadDcacheRequest& req)
{
  uint8_t stage = 0;
  return modular_push_thread_dcache_request_step(&stage,
                                                 valid,
                                                 kind,
                                                 asi,
                                                 context,
                                                 addr,
                                                 data,
                                                 byte_mask,
                                                 req);
}

bool modular_push_thread_dcache_request_step(uint8_t* stage,
                                             void* valid,
                                             void* kind,
                                             void* asi,
                                             void* context,
                                             void* addr,
                                             void* data,
                                             void* byte_mask,
                                             const ModularThreadDcacheRequest& req)
{
  if (stage == nullptr) {
    return false;
  }
  while (1) {
    switch (*stage) {
      case 0:
        if (!pushbool(valid, true, false)) return false;
        *stage = 1;
        break;
      case 1:
        if (!pushchar(kind, req.kind, false)) return false;
        *stage = 2;
        break;
      case 2:
        if (!pushchar(asi, req.asi, false)) return false;
        *stage = 3;
        break;
      case 3:
        if (!pushchar(context, req.context, false)) return false;
        *stage = 4;
        break;
      case 4:
        if (!pushword(addr, req.addr, false)) return false;
        *stage = 5;
        break;
      case 5:
        if (!pushdword(data, req.data, false)) return false;
        *stage = 6;
        break;
      case 6:
        if (!pushchar(byte_mask, req.byte_mask, false)) return false;
        *stage = 0;
        return true;
      default:
        *stage = 0;
        return false;
    }
  }
}

bool modular_pull_mmu_request(void* valid,
                              void* source,
                              void* thread_id,
                              void* mmu_command,
                              void* kind,
                              void* asi,
                              void* context,
                              void* addr,
                              void* data,
                              void* byte_mask,
                              ModularMmuRequest* req)
{
  uint8_t stage = 0;
  return modular_pull_mmu_request_step(&stage,
                                       valid,
                                       source,
                                       thread_id,
                                       mmu_command,
                                       kind,
                                       asi,
                                       context,
                                       addr,
                                       data,
                                       byte_mask,
                                       req);
}

bool modular_pull_mmu_request_step(uint8_t* stage,
                                   void* valid,
                                   void* source,
                                   void* thread_id,
                                   void* mmu_command,
                                   void* kind,
                                   void* asi,
                                   void* context,
                                   void* addr,
                                   void* data,
                                   void* byte_mask,
                                   ModularMmuRequest* req)
{
  if (stage == nullptr || req == nullptr) {
    return false;
  }
  while (1) {
    bool request_valid = false;
    switch (*stage) {
      case 0:
        if (!pullbool(valid, &request_valid, false)) return false;
        if (!request_valid) {
          *stage = 0;
          return false;
        }
        *stage = 1;
        break;
      case 1:
        if (!pullchar(source, &req->source, false)) return false;
        *stage = 2;
        break;
      case 2:
        if (!pullchar(thread_id, &req->thread_id, false)) return false;
        *stage = 3;
        break;
      case 3:
        if (!pullchar(mmu_command, &req->mmu_command, false)) return false;
        *stage = 4;
        break;
      case 4:
        if (!pullchar(kind, &req->kind, false)) return false;
        *stage = 5;
        break;
      case 5:
        if (!pullchar(asi, &req->asi, false)) return false;
        *stage = 6;
        break;
      case 6:
        if (!pullchar(context, &req->context, false)) return false;
        *stage = 7;
        break;
      case 7:
        if (!pullword(addr, &req->addr, false)) return false;
        *stage = 8;
        break;
      case 8:
        if (!pulldword(data, &req->data, false)) return false;
        *stage = 9;
        break;
      case 9:
        if (!pullchar(byte_mask, &req->byte_mask, false)) return false;
        *stage = 0;
        return true;
      default:
        *stage = 0;
        return false;
    }
  }
}

bool modular_push_mmu_request(void* valid,
                              void* source,
                              void* thread_id,
                              void* mmu_command,
                              void* kind,
                              void* asi,
                              void* context,
                              void* addr,
                              void* data,
                              void* byte_mask,
                              const ModularMmuRequest& req)
{
  uint8_t stage = 0;
  return modular_push_mmu_request_step(&stage,
                                       valid,
                                       source,
                                       thread_id,
                                       mmu_command,
                                       kind,
                                       asi,
                                       context,
                                       addr,
                                       data,
                                       byte_mask,
                                       req);
}

bool modular_push_mmu_request_step(uint8_t* stage,
                                   void* valid,
                                   void* source,
                                   void* thread_id,
                                   void* mmu_command,
                                   void* kind,
                                   void* asi,
                                   void* context,
                                   void* addr,
                                   void* data,
                                   void* byte_mask,
                                   const ModularMmuRequest& req)
{
  if (stage == nullptr) {
    return false;
  }
  while (1) {
    switch (*stage) {
      case 0:
        if (!pushbool(valid, true, false)) return false;
        *stage = 1;
        break;
      case 1:
        if (!pushchar(source, req.source, false)) return false;
        *stage = 2;
        break;
      case 2:
        if (!pushchar(thread_id, req.thread_id, false)) return false;
        *stage = 3;
        break;
      case 3:
        if (!pushchar(mmu_command, req.mmu_command, false)) return false;
        *stage = 4;
        break;
      case 4:
        if (!pushchar(kind, req.kind, false)) return false;
        *stage = 5;
        break;
      case 5:
        if (!pushchar(asi, req.asi, false)) return false;
        *stage = 6;
        break;
      case 6:
        if (!pushchar(context, req.context, false)) return false;
        *stage = 7;
        break;
      case 7:
        if (!pushword(addr, req.addr, false)) return false;
        *stage = 8;
        break;
      case 8:
        if (!pushdword(data, req.data, false)) return false;
        *stage = 9;
        break;
      case 9:
        if (!pushchar(byte_mask, req.byte_mask, false)) return false;
        *stage = 0;
        return true;
      default:
        *stage = 0;
        return false;
    }
  }
}

bool modular_push_bridge_request(void* valid,
                                 void* source,
                                 void* thread_id,
                                 void* kind,
                                 void* addr,
                                 void* data,
                                 void* byte_mask,
                                 const ModularBridgeRequest& req)
{
  return pushbool(valid, true, false) &&
         pushchar(source, req.source, false) &&
         pushchar(thread_id, req.thread_id, false) &&
         pushchar(kind, req.kind, false) &&
         pushword(addr, req.addr, false) &&
         pushdword(data, req.data, false) &&
         pushchar(byte_mask, req.byte_mask, false);
}

bool modular_pull_response(void* valid, void* mae, void* data, ModularResponse* resp)
{
  uint8_t stage = 0;
  return modular_pull_response_step(&stage, valid, mae, data, resp);
}

bool modular_pull_response_step(uint8_t* stage,
                                void* valid,
                                void* mae,
                                void* data,
                                ModularResponse* resp)
{
  if (stage == nullptr || resp == nullptr) {
    return false;
  }
  while (1) {
    bool response_valid = false;
    switch (*stage) {
      case 0:
        if (!pullbool(valid, &response_valid, false)) return false;
        if (!response_valid) {
          *stage = 0;
          return false;
        }
        *stage = 1;
        break;
      case 1:
        if (!pullchar(mae, &resp->mae, false)) return false;
        *stage = 2;
        break;
      case 2:
        if (!pulldword(data, &resp->data, false)) return false;
        *stage = 0;
        return true;
      default:
        *stage = 0;
        return false;
    }
  }
}

bool modular_push_response(void* valid, void* mae, void* data, const ModularResponse& resp)
{
  uint8_t stage = 0;
  return modular_push_response_step(&stage, valid, mae, data, resp);
}

bool modular_push_response_step(uint8_t* stage,
                                void* valid,
                                void* mae,
                                void* data,
                                const ModularResponse& resp)
{
  if (stage == nullptr) {
    return false;
  }
  while (1) {
    switch (*stage) {
      case 0:
        if (!pushbool(valid, true, false)) return false;
        *stage = 1;
        break;
      case 1:
        if (!pushchar(mae, resp.mae, false)) return false;
        *stage = 2;
        break;
      case 2:
        if (!pushdword(data, resp.data, false)) return false;
        *stage = 0;
        return true;
      default:
        *stage = 0;
        return false;
    }
  }
}

bool modular_pull_mmu_response(void* valid,
                               void* mae,
                               void* data,
                               void* cacheable,
                               void* acc,
                               void* mmu_fsr,
                               void* synonym,
                               ModularMmuResponse* resp, void* meta)
{
  uint8_t stage = 0;
  return modular_pull_mmu_response_step(&stage,
                                        valid,
                                        mae,
                                        data,
                                        cacheable,
                                        acc,
                                        mmu_fsr,
                                        synonym,
                                        resp, meta);
}

bool modular_pull_mmu_response_step(uint8_t* stage,
                                    void* valid,
                                    void* mae,
                                    void* data,
                                    void* cacheable,
                                    void* acc,
                                    void* mmu_fsr,
                                    void* synonym,
                                    ModularMmuResponse* resp, void* meta)
{
  if (stage == nullptr || resp == nullptr) {
    return false;
  }
  while (1) {
    bool response_valid = false;
    switch (*stage) {
      case 0:
        if (!pullbool(valid, &response_valid, false)) return false;
        if (!response_valid) {
          *stage = 0;
          return false;
        }
        *stage = 1;
        break;
      case 1:
        if (!pullchar(mae, &resp->mae, false)) return false;
        *stage = 2;
        break;
      case 2:
        if (!pulldword(data, &resp->data, false)) return false;
        *stage = 3;
        break;
      case 3:
        if (!pullchar(cacheable, &resp->cacheable, false)) return false;
        *stage = 4;
        break;
      case 4:
        if (!pullchar(acc, &resp->acc, false)) return false;
        *stage = 5;
        break;
      case 5:
        if (!pullword(mmu_fsr, &resp->mmu_fsr, false)) return false;
        *stage = 6;
        break;
      case 6:
        if (!pullword(synonym, &resp->synonym, false)) return false;
        *stage = 7;
        break;
      case 7:
        if (meta && !pulldword(meta, &resp->meta, false)) return false;
        *stage = 0;
        return true;
      default:
        *stage = 0;
        return false;
    }
  }
}

bool modular_push_mmu_response(void* valid,
                               void* mae,
                               void* data,
                               void* cacheable,
                               void* acc,
                               void* mmu_fsr,
                               void* synonym,
                               const ModularMmuResponse& resp, void* meta)
{
  uint8_t stage = 0;
  return modular_push_mmu_response_step(&stage,
                                        valid,
                                        mae,
                                        data,
                                        cacheable,
                                        acc,
                                        mmu_fsr,
                                        synonym,
                                        resp, meta);
}

bool modular_push_mmu_response_step(uint8_t* stage,
                                    void* valid,
                                    void* mae,
                                    void* data,
                                    void* cacheable,
                                    void* acc,
                                    void* mmu_fsr,
                                    void* synonym,
                                    const ModularMmuResponse& resp, void* meta)
{
  if (stage == nullptr) {
    return false;
  }
  while (1) {
    switch (*stage) {
      case 0:
        if (!pushbool(valid, true, false)) return false;
        *stage = 1;
        break;
      case 1:
        if (!pushchar(mae, resp.mae, false)) return false;
        *stage = 2;
        break;
      case 2:
        if (!pushdword(data, resp.data, false)) return false;
        *stage = 3;
        break;
      case 3:
        if (!pushchar(cacheable, resp.cacheable, false)) return false;
        *stage = 4;
        break;
      case 4:
        if (!pushchar(acc, resp.acc, false)) return false;
        *stage = 5;
        break;
      case 5:
        if (!pushword(mmu_fsr, resp.mmu_fsr, false)) return false;
        *stage = 6;
        break;
      case 6:
        if (!pushword(synonym, resp.synonym, false)) return false;
        *stage = 7;
        break;
      case 7:
        if (meta && !pushdword(meta, resp.meta, false)) return false;
        *stage = 0;
        return true;
      default:
        *stage = 0;
        return false;
    }
  }
}
