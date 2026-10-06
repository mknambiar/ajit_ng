#include "modular_async_cache.h"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstdlib>
extern "C" {
#include "MmuCommands.h"
#include "RequestTypeValues.h"
}
namespace {
ModularAsyncCache* instances[4][2] = {};
std::atomic<unsigned> buffered_stores[4] = {};
void put(uint8_t* p, uint64_t x, unsigned n) {
  for(unsigned i=0;i<n;++i) p[i]=(uint8_t)(x>>(8*i));
}
uint64_t get(const uint8_t* p, unsigned n) {
  uint64_t x=0; for(unsigned i=0;i<n;++i) x|=(uint64_t)p[i]<<(8*i); return x;
}
uint8_t kind(const ModularThreadDcacheRequest& r) {
  uint8_t k=r.kind&0x3f;
  if(k==REQUEST_TYPE_CCU_CACHE_READ) k=REQUEST_TYPE_READ;
  if(k==REQUEST_TYPE_CCU_CACHE_WRITE) k=REQUEST_TYPE_WRITE;
  return k;
}
}

sitar::token<40> ModularAsyncCache::pack(const Work& w) {
  sitar::token<40> t; auto p=t.data();
  p[0]=w.tid; p[1]=w.flags; p[2]=w.req.kind; p[3]=w.req.asi;
  p[4]=w.req.context; p[5]=w.req.byte_mask;
  put(p+8,w.req.addr,4); put(p+12,w.physical,4); put(p+16,w.req.data,8);
  put(p+24,w.sequence,8); put(p+32,w.eligible,8); return t;
}
ModularAsyncCache::Work ModularAsyncCache::unpack(sitar::token<40>& t) {
  Work w; auto p=t.data(); w.tid=p[0]; w.flags=p[1]; w.req.kind=p[2];
  w.req.asi=p[3]; w.req.context=p[4]; w.req.byte_mask=p[5];
  w.req.addr=get(p+8,4); w.physical=get(p+12,4); w.req.data=get(p+16,8);
  w.sequence=get(p+24,8); w.eligible=get(p+32,8); return w;
}
sitar::token<80> ModularAsyncCache::pack_result(const Result& r) {
  sitar::token<80> t; auto p=t.data(); auto w=pack(r.work);
  for(unsigned i=0;i<40;++i) p[i]=w.data()[i];
  p[40]=r.response.mae; p[41]=r.response.cacheable; p[42]=r.response.acc;
  p[43]=r.event; p[44]=r.first; p[45]=r.terminal;
  put(p+48,r.response.data,8); put(p+56,r.response.meta,8);
  put(p+64,r.response.mmu_fsr,4); put(p+68,r.response.synonym,4);
  put(p+72,r.epoch,8); return t;
}
ModularAsyncCache::Result ModularAsyncCache::unpack_result(sitar::token<80>& t) {
  Result r; auto p=t.data(); sitar::token<40> w;
  for(unsigned i=0;i<40;++i) w.data()[i]=p[i];
  r.work=unpack(w);
  r.response.mae=p[40]; r.response.cacheable=p[41]; r.response.acc=p[42];
  r.event=(Event)p[43]; r.first=p[44]; r.terminal=p[45];
  r.response.data=get(p+48,8); r.response.meta=get(p+56,8);
  r.response.mmu_fsr=get(p+64,4); r.response.synonym=get(p+68,4);
  r.epoch=get(p+72,8); return r;
}
void ModularAsyncCache::init(WriteThroughAllocateCache* c,int id,int n,bool i,
                            sitar::net<40>* q,sitar::net<40>* d,sitar::net<80>* r) {
  cache=c;core=id;threads=std::clamp(n,1,2);instruction=i;
  const char* tr=std::getenv("AJIT_MODULAR_TRACE_ADAPTER");
  const char* ft=std::getenv("AJIT_WT_FAULT_DIAGNOSTICS");
  trace=tr && tr[0] && tr[0]!='0';fault_trace=ft && ft[0] && ft[0]!='0';
  pending=q;direct=d;completed=r;
  assert(c && q->capacity()==4 && d->capacity()==1 && r->capacity()==2);
  assert(id>=0 && id<4);instances[id][i?0:1]=this;
  if(!instruction) buffered_stores[id].store(0);
}
uint8_t ModularAsyncCache::context(const Work& w) const {
  return cache->multi_context ? w.req.context : 0;
}
bool ModularAsyncCache::read(const Work& w) const {
  return instruction || kind(w.req)==REQUEST_TYPE_READ;
}
bool ModularAsyncCache::cached(const Work& w) const {
  unsigned a=w.req.asi&0x7f, k=kind(w.req);
  return instruction ? (k==REQUEST_TYPE_IFETCH && a>=8 && a<=9) :
      ((k==REQUEST_TYPE_READ || k==REQUEST_TYPE_WRITE) && a>=8 && a<=11);
}
bool ModularAsyncCache::ordered(const Work& w) const {
  return atomic_owner>=0 || !cached(w) || (w.req.kind&0x40) ||
      (!instruction && !isCacheableRequestBasedOnMmuStatus(cache,w.tid));
}
int ModularAsyncCache::hit(const Work& w) const {
  if(!cached(w) || (w.req.kind&0x40)) return -1;
  if(!instruction && !isCacheableRequestBasedOnMmuStatus(cache,w.tid)) return -1;
  int i=cacheLineId(cache,context(w),w.req.addr);
  if(i<0 || !cache->cache_lines[i].valid ||
     cache->cache_lines[i].va_tag!=vaTag(cache,context(w),w.req.addr) ||
     lines[i].tag!=cache->cache_lines[i].va_tag ||
     !(lines[i].words&(1u<<cacheLineOffset(w.req.addr))) ||
     !accessPermissionsOk(instruction?REQUEST_TYPE_IFETCH:kind(w.req),
                          w.req.asi&0x7f,cache->cache_lines[i].acc)) return -1;
  return i;
}
bool ModularAsyncCache::authorize(const Work& w,int i) const {
  return atomic_owner<0 && i>=0 && lines[i].authorized && lines[i].tid==w.tid &&
      lines[i].context==w.req.context && !(w.req.kind&0x40);
}
void ModularAsyncCache::respond(const Work& w,uint8_t mae,uint64_t data) {
  assert(unanswered[w.tid] && !response_ready[w.tid]);
  responses[w.tid]={mae,data};response_ready[w.tid]=true;
  if(fence==w.sequence) fence=0;
}
void ModularAsyncCache::invalidate(uint32_t va_line) {
  unsigned set=cacheSetId(cache,va_line<<LOG_BYTES_PER_CACHE_LINE);
  ++invalidate_epoch[set]; invalidateCacheLine(cache,va_line);
}
void ModularAsyncCache::flush() {
  flushCache(cache);for(int s=0;s<cache->number_of_sets;++s) ++invalidate_epoch[s];
  for(auto& l:lines) l={};
}
void ModularAsyncCache::count(const Work& w,bool h) {
  ++cache->number_of_accesses;
  if(w.req.kind&0x40) ++cache->number_of_locked_accesses;
  if(cached(w)) {
    if(h) {++cache->number_of_hits;if(!instruction) {
      if(read(w)) ++cache->number_of_read_hits; else ++cache->number_of_write_hits;}}
    else {++cache->number_of_misses;if(!instruction) {
      if(read(w)) ++cache->number_of_read_misses;else ++cache->number_of_write_misses;}}
  } else if(!instruction) ++cache->number_of_bypasses;
}
bool ModularAsyncCache::enqueue(Work& w,uint64_t tick) {
  assert(!(tick&1));
  // Phase 0 reserves previously sampled space, but never pushes a work net.
  // Do not observe same-phase worker dequeues: that would make acceptance
  // dependent on the generated parallel-branch execution order.
  assert(publish_count<2);
  const bool use_direct=(outstanding==0);
  if(use_direct) {
    assert(direct_credit==1);--direct_credit;
    w.eligible=tick;++bypassed;
  } else {
    if(!pending_credit) {++full_stalls;return false;}
    --pending_credit;w.eligible=tick+2;++queued;
  }
  publish_work[publish_count]=w;
  publish_direct[publish_count++]=use_direct;
  ++outstanding;peak=std::max(peak,outstanding);
  if(w.flags&1) {++enqueued;++stores_pending;buffered_stores[core].fetch_add(1);}
  return true;
}
void ModularAsyncCache::accept_result(const Result& r) {
  const Work& w=r.work;const auto& m=r.response;
  if((instruction && r.event==Hit) ? (m.mae&3) : m.mae) ++faults;
  if(fault_trace && m.mae && !instruction && (r.event==Write || r.event==Physical))
    std::fprintf(stderr,"MOD-WT-FAULT c%d t%u seq=%llu va=0x%08x kind=%u asi=0x%02x context=%u mask=0x%02x data=0x%016llx mae=%u fsr=0x%08x physical=%u outstanding=%llu\n",
        core,w.tid,(unsigned long long)w.sequence,w.req.addr,w.req.kind,w.req.asi,w.req.context,
        w.req.byte_mask,(unsigned long long)w.req.data,m.mae,m.mmu_fsr,r.event==Physical,
        (unsigned long long)outstanding);
  if(r.terminal && atomic_owner==w.tid && (m.mae || !(w.req.kind&0x40))) {
    atomic_owner=-1;cache->lock_flag=0;
  }
  if(r.event==Physical) {
    // The present bridge cannot report errors. An impossible late fault must
    // never be attached to a later CPU request.
    if(m.mae) {
      std::fprintf(stderr,"MOD-ASYNC-LATE-ERROR c%d t%u seq=%llu va=%08x pa=%08x mae=%u\n",
                   core,w.tid,(unsigned long long)w.sequence,w.req.addr,w.physical,m.mae);
      std::abort();
    }
    assert(stores_pending);--stores_pending;++drained;buffered_stores[core].fetch_sub(1);
  } else if(r.event==Fill) {
    unsigned set=cacheSetId(cache,w.req.addr);
    if(r.first) {
      fill_slot=-1;fill_sequence=w.sequence;fill_epoch=r.epoch;
      if(m.synonym) invalidate(m.synonym&0x7fffffff);
      if(!m.mae && m.cacheable && (m.meta&MOD_MMU_AUTHORIZED) &&
         invalidate_epoch[set]==r.epoch) {
        int i=cacheLineId(cache,context(w),w.req.addr);
        if(i<0) i=allocateCacheLine(cache,context(w),w.req.addr);
        auto& cl=cache->cache_lines[i];
        cl.valid=1;cl.acc=m.acc;cl.va_tag=vaTag(cache,context(w),w.req.addr);
        lines[i]={};lines[i].tag=cl.va_tag;lines[i].tid=w.tid;
        lines[i].context=w.req.context;lines[i].physical=(uint32_t)m.meta&LINE_ADDR_MASK;
        lines[i].authorized=true;fill_slot=i;fill_tag=cl.va_tag;
      }
    }
    if(fill_slot>=0 && fill_sequence==w.sequence) {
      auto& cl=cache->cache_lines[fill_slot];
      if(m.mae || !cl.valid || cl.va_tag!=fill_tag || invalidate_epoch[set]!=fill_epoch) {
        if(m.mae && cl.va_tag==fill_tag) cl.valid=0;
        fill_slot=-1;
      } else {
        unsigned word=(m.meta>>34)&7;
        // Each offset arrives once. Never recopy the entire line on completion:
        // that would overwrite younger byte stores to an already valid word.
        assert(!(lines[fill_slot].words&(1u<<word)));
        cl.cache_line[word]=m.data;lines[fill_slot].words|=1u<<word;
      }
    }
    if(r.first) {
      uint8_t mae=instruction ? (uint8_t)((m.cacheable<<7)|((m.acc&7)<<4)|(m.mae&3)) : m.mae;
      respond(w,mae,m.data);++critical_responses;
    } else if(m.mae) {
      // No asynchronous bus error is representable in the current bridge.
      // Translation faults always terminate before the first data beat.
      std::fprintf(stderr,"MOD-ASYNC-REFILL-ERROR c%d seq=%llu\n",core,(unsigned long long)w.sequence);
      std::abort();
    }
    if(r.terminal) fill_slot=-1;
  } else if(r.event==Write) {
    if(!m.mae) {
      int i=hit(w);
      if(cached(w) && (w.req.kind&0x40)) invalidate(w.req.addr>>LOG_BYTES_PER_CACHE_LINE);
      if(i>=0 && m.cacheable && (m.meta&MOD_MMU_AUTHORIZED)) {
        writeIntoLine(cache,w.req.data,context(w),w.req.addr,w.req.byte_mask);
        lines[i].authorized=true;lines[i].tid=w.tid;lines[i].context=w.req.context;
        lines[i].physical=(uint32_t)m.meta&LINE_ADDR_MASK;
      } else if(i>=0) invalidate(w.req.addr>>LOG_BYTES_PER_CACHE_LINE);
      if((w.req.asi&0x7f)==4 && kind(w.req)==REQUEST_TYPE_WRITE)
        updateMmuState(cache,w.tid,w.req.byte_mask,w.req.addr,w.req.data);
    }
    if(r.terminal) respond(w,m.mae,0);
  } else {
    respond(w,m.mae,m.data);
  }
  if(r.terminal) {assert(outstanding);--outstanding;}
}

void ModularAsyncCache::front(uint64_t tick) {
  if(tick&1) {
    // Publish reserved work before acknowledging stores to the CPU.
    for(unsigned n=0;n<publish_count;++n) {
      auto* net=publish_direct[n]?direct:pending;
      if(!net->push(pack(publish_work[n]))) {
        std::fprintf(stderr,"MOD-ASYNC reserved work FIFO capacity violated\n");
        std::abort();
      }
    }
    publish_count=0;
    pending_credit=pending->remainingCapacity();
    direct_credit=direct->remainingCapacity();
    for(int t=0;t<threads;++t) if(response_ready[t]) {
      if(modular_push_response_step(&response_stage[t],thread_resp_valid[t],
             thread_resp_mae[t],thread_resp_data[t],responses[t])) {
        response_ready[t]=false;unanswered[t]=false;
      }
    }
    return;
  }
  // Phase 0 is the sole cache mutation phase, including refill publication.
  for(;;) {uint32_t inv=probeCoherencyInvalidateRequest(core,instruction?1:0);
    if(!inv) break;
    invalidate(inv&0x7fffffff);
  }
  sitar::token<80> done;
  if(completed->pull(done)) accept_result(unpack_result(done));
  for(int offset=0;offset<threads;++offset) {
    int t=(next+offset)%threads;
    if(unanswered[t] || held[t]) continue;
    bool ok;
    if(instruction) {
      ok=modular_pull_thread_icache_request_step(&pull_stage[t],thread_valid[t],
          thread_kind[t],thread_asi[t],thread_context[t],thread_addr[t],&irequest[t]);
      if(ok) {auto& a=accepted[t].req;const auto& b=irequest[t];
        a.kind=b.kind;a.asi=b.asi;a.context=b.context;a.addr=b.addr;a.data=0;a.byte_mask=255;}
    } else ok=modular_pull_thread_dcache_request_step(&pull_stage[t],thread_valid[t],
          thread_kind[t],thread_asi[t],thread_context[t],thread_addr[t],thread_data[t],
          thread_byte_mask[t],&accepted[t].req);
    if(ok) {
      accepted[t].tid=t;accepted[t].flags=0;accepted[t].sequence=++serial;
      held[t]=true;unanswered[t]=true;next=(t+1)%threads;
      if(trace) std::fprintf(stderr,"MOD-%sCACHE c%d accepted t%d kind=%u addr=0x%08x seq=%llu tick=%llu\n",
          instruction?"I":"D",core,t,accepted[t].req.kind,accepted[t].req.addr,
          (unsigned long long)serial,(unsigned long long)tick);
      if(ordered(accepted[t]) && !fence && (atomic_owner<0 || atomic_owner==t)) fence=serial;
      break;
    }
  }
  // Acceptance order, not thread ID, determines FIFO order.
  int first=0;
  if(held[1] && (!held[0] || accepted[1].sequence<accepted[0].sequence)) first=1;
  for(int n=0;n<2;++n) {
    int t=(first+n)%2;if(t>=threads || !held[t]) continue;
    auto& w=accepted[t];unsigned k=kind(w.req), a=w.req.asi&0x7f;
    if(atomic_owner>=0 && atomic_owner!=t) {++ordering_stalls;continue;}
    if(!fence && ordered(w)) fence=w.sequence;
    if((fence && w.sequence>fence && atomic_owner!=t) || (ordered(w) && outstanding)) {++ordering_stalls;continue;}
    if(k==REQUEST_TYPE_NOP || k==REQUEST_TYPE_STBAR ||
       ((!instruction && a>=0x10 && a<=0x14) ||
        (instruction && k==REQUEST_TYPE_WRITE && ((a>=0x10&&a<=0x14)||(a>=0x18&&a<=0x1c))))) {
      ++cache->number_of_accesses;
      if(k!=REQUEST_TYPE_NOP && k!=REQUEST_TYPE_STBAR) {++cache->number_of_flushes;flush();}
      respond(w,0,0);held[t]=false;continue;
    }
    if((instruction && !cached(w)) || (!instruction && k!=REQUEST_TYPE_READ &&
        k!=REQUEST_TYPE_WRITE && k!=REQUEST_TYPE_WRFSRFAR)) {
      count(w,false);respond(w,1,0);held[t]=false;continue;
    }
    int i=hit(w);
    if(i>=0 && read(w)) {
      count(w,true);++fast_hits;if(stores_pending) ++forwarded;
      uint8_t mae=instruction ? (uint8_t)(0x80|((cache->cache_lines[i].acc&7)<<4)) : 0;
      respond(w,mae,getDwordFromCache(cache,context(w),w.req.addr));held[t]=false;continue;
    }
    if(i>=0 && !read(w) && authorize(w,i)) {
      w.flags=1;w.physical=lines[i].physical|(w.req.addr&~LINE_ADDR_MASK);
      if(!enqueue(w,tick)) continue;
      count(w,true);writeIntoLine(cache,w.req.data,context(w),w.req.addr,w.req.byte_mask);
      respond(w,0,0);held[t]=false;continue;
    }
    // MMU/context operations are a cache-wide fence and revoke all certificates.
    if(ordered(w) && a<8 && k!=REQUEST_TYPE_READ) {++cache->number_of_flushes;flush();}
    if(!enqueue(w,tick)) continue;
    if(w.req.kind&0x40) {atomic_owner=t;cache->lock_flag=1;cache->lock_core_id=core;}
    count(w,i>=0);held[t]=false;
  }
}

void ModularAsyncCache::prepare_mmu(uint8_t command) {
  request={};request.source=instruction?0:1;request.thread_id=current.tid;
  request.mmu_command=command;request.kind=kind(current.req)|(current.req.kind&0x40);
  request.asi=current.req.asi&0x7f;request.context=context(current);
  request.addr=(current.flags&1)?current.physical:current.req.addr;
  request.data=current.req.data;request.byte_mask=current.req.byte_mask;
  sent=false;request_stage=0;mmu_stage=0;got=false;
}
void ModularAsyncCache::start(const Work& w) {
  current=w;busy=true;first_beat=true;refill_after_write=false;
  transaction_epoch=invalidate_epoch[cacheSetId(cache,w.req.addr)];
  if(w.flags&1) {operation=Physical;prepare_mmu(MOD_MMU_WRITE_PHYSICAL);return;}
  int i=hit(w); // Phase-1 read-only recheck after older work has completed.
  if(i>=0 && read(w)) {
    result={};result.work=w;result.event=Hit;
    result.response.data=getDwordFromCache(cache,context(w),w.req.addr);
    result.response.mae=instruction?(uint8_t)(0x80|((cache->cache_lines[i].acc&7)<<4)):0;
    have_result=true;operation=Hit;return;
  }
  if(cached(w) && read(w) && !(w.req.kind&0x40) &&
     (instruction || isCacheableRequestBasedOnMmuStatus(cache,w.tid))) {
    operation=Fill;prepare_mmu(MMU_READ_LINE);return;
  }
  if(!read(w)) {
    operation=Write;
    refill_after_write=cached(w) && i<0 && !(w.req.kind&0x40) &&
        isCacheableRequestBasedOnMmuStatus(cache,w.tid);
    prepare_mmu(kind(w.req)==REQUEST_TYPE_WRFSRFAR ? MMU_WRITE_FSR : MMU_WRITE_DWORD);
  } else {operation=Dword;prepare_mmu(MMU_READ_DWORD);}
}
void ModularAsyncCache::worker(uint64_t tick) {
  if(!(tick&1)) {
    if(busy && sent && !got && !have_result) {
      got=modular_pull_mmu_response_step(&mmu_stage,mmu_resp_valid,mmu_resp_mae,
          mmu_resp_data,mmu_resp_cacheable,mmu_resp_acc,mmu_resp_fsr,mmu_resp_synonym,
          &response,mmu_resp_meta);
    }
    // Receive work in phase 0; defer shared-cache reads until phase 1.
    if(!busy && !dispatch_ready) {
      sitar::token<40> t;
      if(direct->pull(t)) {
        dispatch_work=unpack(t);dispatch_ready=true;
      } else if(pending->peek(t)) {
        Work w=unpack(t);
        if(tick>=w.eligible) {
          const bool pulled=pending->pull(t);assert(pulled);
          dispatch_work=unpack(t);dispatch_ready=true;
        }
      }
    }
    return;
  }
  if(got && !have_result) {
    result={};result.work=current;result.response=response;result.event=operation;
    result.first=first_beat;result.epoch=transaction_epoch;
    result.terminal=operation!=Fill || (response.meta&MOD_MMU_LAST);
    if(operation==Write && refill_after_write && !response.mae && response.cacheable)
      result.terminal=false;
    have_result=true;got=false;
  }
  if(have_result) {
    if(!completed->push(pack_result(result))) return;
    have_result=false;
    if(result.terminal) {busy=false;sent=false;}
    else if(operation==Write) {
      operation=Fill;first_beat=true;prepare_mmu(MMU_READ_LINE);
    } else first_beat=false;
    // Terminal publication must be consumed in phase 0 before dispatching
    // another request, so queued rechecks see all completed cache mutations.
    if(!busy) return;
  }
  if(!busy) {
    if(!dispatch_ready) return;
    start(dispatch_work);dispatch_ready=false;
  }
  if(have_result) return;
  if(!sent) sent=modular_push_mmu_request_step(&request_stage,mmu_valid,mmu_source,
      mmu_thread_id,mmu_command,mmu_kind,mmu_asi,mmu_context,mmu_addr,mmu_data,
      mmu_byte_mask,request);
}
void ModularAsyncCache::summary() const {
  std::fprintf(stderr,"MOD-ASYNC c%d %s enq=%llu drain=%llu full-stall=%llu ordering-stall=%llu faults=%llu outstanding=%llu max-outstanding=%llu stores-pending=%llu fast-hits=%llu critical=%llu loads-with-buffered-stores=%llu queued=%llu direct=%llu capacity=4\n",
      core,instruction?"I":"D",(unsigned long long)enqueued,(unsigned long long)drained,
      (unsigned long long)full_stalls,(unsigned long long)ordering_stalls,(unsigned long long)faults,
      (unsigned long long)outstanding,(unsigned long long)peak,(unsigned long long)stores_pending,
      (unsigned long long)fast_hits,(unsigned long long)critical_responses,(unsigned long long)forwarded,
      (unsigned long long)queued,(unsigned long long)bypassed);
}
extern "C" void modular_async_cache_dump_summary() {
  for(auto& core:instances) for(auto p:core) if(p) p->summary();
}

extern "C" int modular_async_buffered_stores_pending() {
  for(auto& n:buffered_stores) if(n.load()) return 1;
  return 0;
}
