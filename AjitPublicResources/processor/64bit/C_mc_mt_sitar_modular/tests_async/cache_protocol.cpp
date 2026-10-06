#include "modular_async_cache.h"
#include "sitar_inport.h"
#include "sitar_outport.h"
#include "DCache.h"
#include <memory>
static bool native_mode=false;
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <map>
#include <vector>
extern "C" {
#include "MmuCommands.h"
#include "RequestTypeValues.h"
int global_verbose_flag=0;
int hasMultiContextMunit(int) {return 0;}
int calculate_log2(int n) {int x=0;while(n>1){n>>=1;++x;}return x;}
uint64_t insert_bytes_into_dword(uint64_t x,uint8_t mask,uint64_t value) {
  for(unsigned i=0;i<8;++i) if(mask&(1u<<i)) {
    uint64_t bits=255ull<<(i*8);x=(x&~bits)|(value&bits);
  }return x;
}
uint32_t invalidation=0;
uint32_t ajit_shim_probe_coherence_invalidate(int,int) {
  uint32_t x=invalidation;invalidation=0;return x;
}
}
template<unsigned N,unsigned Capacity=1> struct Channel {
  sitar::net<N> net;sitar::token<N> buffer[Capacity];
  sitar::inport<N> in;sitar::outport<N> out;
  Channel(){net.setBuffer(buffer,Capacity);in.setNet(&net);out.setNet(&net);}
};
struct Cpu {
  Channel<1> valid,rv;Channel<8> kind,asi,context,mask,mae;
  Channel<32> addr;Channel<64> data,rd;
};
struct MmuChannels {
  Channel<1> valid,rv;Channel<8> source,tid,command,kind,asi,context,mask,mae,cacheable,acc;
  Channel<32> addr,fsr,synonym;Channel<64> data,rd,meta;
};
struct Reply {int thread;uint64_t tick;ModularResponse value;};
struct Harness {
  ModularAsyncCache engine;std::unique_ptr<sitar::DCache<0,2>> native;Cpu cpu[2];MmuChannels mmu;
  Channel<40,4> queue;Channel<40> direct;Channel<80,2> results;
  WriteThroughAllocateCache* cache;
  uint64_t tick=0,due=0,write_delay=5;
  bool reverse=false,active=false,deny_write=false,nofault=false,uncached=false;
  unsigned beat=0;uint8_t permission=3,mmu_pull=0,mmu_push=0,cpu_pull[2]={},cpu_push[2]={};
  ModularMmuRequest request;
  std::vector<ModularMmuRequest> requests;
  std::vector<uint64_t> issue_ticks;
  std::vector<Reply> replies;
  std::deque<ModularThreadDcacheRequest> waiting[2];
  std::map<uint32_t,uint64_t> memory;
  Harness(bool rev=false):reverse(rev) {
    invalidation=0;cache=makeCache(0,0,16,1);cache->mmu_control_register[0]=1;cache->mmu_control_register[1]=1;
    engine.init(cache,0,2,false,&queue.net,&direct.net,&results.net);
    if(native_mode) native.reset(new sitar::DCache<0,2>);
    for(int t=0;t<2;++t) {
#define CPU_BIND(field, chan, side) engine.field[t]=&cpu[t].chan.side; if(native) native->field[t].setNet(&cpu[t].chan.net)
      CPU_BIND(thread_valid,valid,in);CPU_BIND(thread_kind,kind,in);CPU_BIND(thread_asi,asi,in);
      CPU_BIND(thread_context,context,in);CPU_BIND(thread_addr,addr,in);CPU_BIND(thread_data,data,in);
      CPU_BIND(thread_byte_mask,mask,in);CPU_BIND(thread_resp_valid,rv,out);
      CPU_BIND(thread_resp_mae,mae,out);CPU_BIND(thread_resp_data,rd,out);
#undef CPU_BIND
    }
#define MMU_BIND(field, chan, side) engine.field=&mmu.chan.side; if(native) native->field.setNet(&mmu.chan.net)
    MMU_BIND(mmu_valid,valid,out);MMU_BIND(mmu_source,source,out);MMU_BIND(mmu_thread_id,tid,out);
    MMU_BIND(mmu_command,command,out);MMU_BIND(mmu_kind,kind,out);MMU_BIND(mmu_asi,asi,out);
    MMU_BIND(mmu_context,context,out);MMU_BIND(mmu_addr,addr,out);MMU_BIND(mmu_data,data,out);
    MMU_BIND(mmu_byte_mask,mask,out);MMU_BIND(mmu_resp_valid,rv,in);MMU_BIND(mmu_resp_mae,mae,in);
    MMU_BIND(mmu_resp_data,rd,in);MMU_BIND(mmu_resp_cacheable,cacheable,in);
    MMU_BIND(mmu_resp_acc,acc,in);MMU_BIND(mmu_resp_fsr,fsr,in);MMU_BIND(mmu_resp_synonym,synonym,in);
    MMU_BIND(mmu_resp_meta,meta,in);
#undef MMU_BIND
    if(native) {
      native->setInstanceId("DC");native->runBehavior(sitar::time(uint64_t(0)));
      native->adapter->cache->mmu_control_register[0]=1;
      native->adapter->cache->mmu_control_register[1]=1;
    }
  }
  ~Harness(){pthread_mutex_destroy(&cache->cache_mutex);std::free(cache);}
  uint64_t word(uint32_t a) {a&=~7u;auto it=memory.find(a);return it==memory.end()?0x1122334455660000ull+a:it->second;}
  void send(int t,uint8_t kind,uint32_t addr,uint64_t data=0,uint8_t mask=255,uint8_t asi=11) {
    waiting[t].push_back({kind,asi,0,addr,data,mask});
  }
  void step() {
    if(!(tick&1)) {
      ModularMmuRequest r;
      if(modular_pull_mmu_request_step(&mmu_pull,&mmu.valid.in,&mmu.source.in,&mmu.tid.in,
          &mmu.command.in,&mmu.kind.in,&mmu.asi.in,&mmu.context.in,&mmu.addr.in,&mmu.data.in,&mmu.mask.in,&r)) {
        assert(!active); // no new transaction during any active line fill
        request=r;requests.push_back(r);issue_ticks.push_back(tick);active=true;beat=0;
        due=tick+((r.kind&63)==REQUEST_TYPE_WRITE ? write_delay:3);
      }
      for(int t=0;t<2;++t) {
        ModularResponse r;
        if(modular_pull_response_step(&cpu_pull[t],&cpu[t].rv.in,&cpu[t].mae.in,&cpu[t].rd.in,&r))
          replies.push_back({t,tick,r});
      }
    } else {
      for(int t=0;t<2;++t) if(!waiting[t].empty()) {
        auto& c=cpu[t];
        if(modular_push_thread_dcache_request_step(&cpu_push[t],&c.valid.out,&c.kind.out,&c.asi.out,
            &c.context.out,&c.addr.out,&c.data.out,&c.mask.out,waiting[t].front())) waiting[t].pop_front();
      }
      if(active && tick>=due) {
        ModularMmuResponse r;r.cacheable=uncached?0:1;r.acc=permission;
        bool line=request.mmu_command==MMU_READ_LINE;
        unsigned offset=line?(((request.addr>>3)+beat)&7):((request.addr>>3)&7);
        uint32_t addr=line?((request.addr&~63u)+offset*8):request.addr;
        bool write=(request.kind&63)==REQUEST_TYPE_WRITE && !line;
        bool bad=write && deny_write && request.mmu_command!=MOD_MMU_WRITE_PHYSICAL;
        bool last=!line || beat==7 || nofault || uncached;
        r.meta=(uint64_t)addr|(last?MOD_MMU_LAST:0)|
            ((!bad && !nofault && !uncached)?MOD_MMU_AUTHORIZED:0)|((uint64_t)offset<<34);
        r.mae=bad?1:0;
        if(nofault){r.cacheable=0;r.acc=0;r.data=0;}
        else r.data=word(addr);
        if(modular_push_mmu_response_step(&mmu_push,&mmu.rv.out,&mmu.mae.out,&mmu.rd.out,
            &mmu.cacheable.out,&mmu.acc.out,&mmu.fsr.out,&mmu.synonym.out,r,&mmu.meta.out)) {
          if(write && !bad) memory[addr&~7u]=insert_bytes_into_dword(word(addr),request.byte_mask,request.data);
          if(last) active=false;else {++beat;due=tick+8;}
        }
      }
    }
    // Observe actual net occupancy across cache execution, after peer activity.
    // These assertions catch wrong-phase transfers in either branch order and
    // in generated native SiTAR behavior, without accessing private engine state.
    auto* q=native ? &native->pending_work : &queue.net;
    auto* d=native ? &native->idle_dispatch : &direct.net;
    auto* r=native ? &native->refill_results : &results.net;
    unsigned nq=q->numTokens(),nd=d->numTokens(),nr=r->numTokens();
    unsigned nm=mmu.valid.net.numTokens(),nmr=mmu.rv.net.numTokens();
    unsigned nc[2]={cpu[0].valid.net.numTokens(),cpu[1].valid.net.numTokens()};
    unsigned ncr[2]={cpu[0].rv.net.numTokens(),cpu[1].rv.net.numTokens()};
    if(native) native->runBehavior(sitar::time(tick));
    else if(reverse){engine.worker(tick);engine.front(tick);}else{engine.front(tick);engine.worker(tick);}
    if(tick&1) {
      assert(q->numTokens()>=nq && d->numTokens()>=nd && r->numTokens()>=nr);
      assert(mmu.valid.net.numTokens()>=nm && mmu.rv.net.numTokens()==nmr);
      for(int t=0;t<2;++t) {
        assert(cpu[t].valid.net.numTokens()==nc[t]);
        assert(cpu[t].rv.net.numTokens()>=ncr[t]);
      }
    } else {
      assert(q->numTokens()<=nq && d->numTokens()<=nd && r->numTokens()<=nr);
      assert(mmu.valid.net.numTokens()==nm && mmu.rv.net.numTokens()<=nmr);
      for(int t=0;t<2;++t) {
        assert(cpu[t].valid.net.numTokens()<=nc[t]);
        assert(cpu[t].rv.net.numTokens()==ncr[t]);
      }
    }
    ++tick;
  }
  void run(unsigned n){for(unsigned i=0;i<n;++i)step();}
  void until(size_t n){unsigned budget=20000;while(replies.size()<n && budget--)step();assert(replies.size()==n);}
};
void exact_phase_timing(bool reverse) {
  Harness h(reverse);
  h.send(0,REQUEST_TYPE_READ,0x9000);
  h.until(1);
  assert(h.issue_ticks.size()==1 && h.issue_ticks[0]==6);
  assert(h.replies[0].tick==14); // request1, reserve2, push3, pull4, MMU send5
  assert(h.active); // CPU resumed before the remaining seven refill words
  const uint64_t issued=h.tick|1ull;
  h.send(1,REQUEST_TYPE_READ,0x9000);h.until(2);
  assert(h.replies[1].tick==issued+3); // same direct hit latency during refill
  assert(h.requests.size()==1);
  h.run(100);
  const uint64_t store_issued=h.tick|1ull;
  h.send(0,REQUEST_TYPE_WRITE,0x9000,123);h.until(3);
  assert(h.replies[2].tick==store_issued+3); // reservation preserves early ACK
  h.run(100);assert(h.word(0x9000)==123);
}
void offsets_and_recheck(bool reverse) {
  for(unsigned off=0;off<8;++off) {
    Harness h(reverse);uint32_t a=0x1000+off*8;
    h.send(0,REQUEST_TYPE_READ,a);h.until(1);
    assert(h.replies[0].value.data==h.word(a));assert(h.active && h.beat<7);
    size_t n=h.requests.size();uint64_t start=h.tick;
    h.send(1,REQUEST_TYPE_READ,a);h.until(2);
    assert(h.replies[1].value.data==h.word(a));assert(h.tick-start<=6);assert(h.requests.size()==n);
    uint32_t missing=0x1000+((off+7)&7)*8;
    h.send(0,REQUEST_TYPE_READ,missing);h.until(3);
    assert(h.replies[2].value.data==h.word(missing));assert(h.requests.size()==1);
    h.run(20);
  }
}
void stores_and_forwarding(bool reverse) {
  Harness h(reverse);h.send(0,REQUEST_TYPE_READ,0x2000);h.until(1);
  h.send(0,REQUEST_TYPE_WRITE,0x2000,0xaabbccdd00000000ull,0xf0);h.until(2);
  assert(h.active && h.requests.size()==1); // acknowledged during refill
  assert(modular_async_buffered_stores_pending());
  h.send(0,REQUEST_TYPE_READ,0x2000);h.until(3);
  uint64_t expected=0xaabbccdd55662000ull;
  assert(h.replies[2].value.data==expected);h.run(200);
  assert(h.word(0x2000)==expected);
  assert(!modular_async_buffered_stores_pending());
  h.send(0,REQUEST_TYPE_READ,0x2000);h.until(4);assert(h.replies[3].value.data==expected);
}
void full_and_barrier(bool reverse) {
  Harness h(reverse);h.send(0,REQUEST_TYPE_READ,0x3000);h.until(1);h.run(120);h.write_delay=101;
  for(unsigned i=0;i<5;++i){h.send(0,REQUEST_TYPE_WRITE,0x3000,100+i);h.until(2+i);}
  h.send(0,REQUEST_TYPE_WRITE,0x3000,105);h.run(12);assert(h.replies.size()==6);
  h.send(1,REQUEST_TYPE_STBAR,0,0,0,0);h.run(20);assert(h.replies.size()==6);
  h.until(8);h.run(20);assert(h.word(0x3000)==105);
  assert(h.requests.size()==7);
  for(unsigned i=1;i<7;++i) {assert(h.requests[i].mmu_command==MOD_MMU_WRITE_PHYSICAL);assert(h.requests[i].data==99+i);}
}
void faults_and_invalidation(bool reverse) {
  Harness h(reverse);h.permission=0;h.deny_write=true;
  h.send(0,REQUEST_TYPE_READ,0x4000);h.until(1);h.run(100);
  uint64_t original=h.word(0x4000);
  h.send(0,REQUEST_TYPE_WRITE,0x4000,999);h.until(2);assert(h.replies[1].value.mae==1);
  h.send(0,REQUEST_TYPE_READ,0x4000);h.until(3);
  assert(h.replies[2].value.mae==0 && h.replies[2].value.data==original);
  assert(h.requests[1].mmu_command==MMU_WRITE_DWORD);
  Harness nf(reverse);nf.nofault=true;nf.send(0,REQUEST_TYPE_READ,0x5000);nf.until(1);
  nf.run(30);assert(nf.replies[0].value.mae==0 && nf.replies[0].value.data==0 && !nf.active);
  Harness u(reverse);u.uncached=true;u.send(0,REQUEST_TYPE_READ,0x5800);u.until(1);
  u.run(30);u.send(0,REQUEST_TYPE_READ,0x5800);u.until(2);assert(u.requests.size()==2);
  Harness inv(reverse);inv.send(0,REQUEST_TYPE_READ,0x6000);inv.until(1);
  invalidation=0x80000000u|(0x6000>>6);inv.run(100);
  inv.send(0,REQUEST_TYPE_READ,0x6000);inv.until(2);assert(inv.requests.size()==2);
}
void atomic_and_context(bool reverse) {
  Harness h(reverse);h.send(0,REQUEST_TYPE_READ,0x7000);h.until(1);h.run(100);
  h.send(0,REQUEST_TYPE_READ|0x40,0x7000);h.until(2);
  h.send(1,REQUEST_TYPE_WRITE,0x7000,456);h.run(20);assert(h.replies.size()==2);
  h.send(0,REQUEST_TYPE_WRITE,0x7000,123);h.until(4);h.run(100);
  assert(h.replies[2].thread==0 && h.replies[3].thread==1);
  assert(h.requests[2].mmu_command==MMU_WRITE_DWORD);assert(h.word(0x7000)==456);
  Harness c(reverse);c.send(0,REQUEST_TYPE_READ,0x8000);c.until(1);c.run(100);
  c.send(0,REQUEST_TYPE_WRITE,0x8000,777);c.until(2);
  c.send(0,REQUEST_TYPE_WRITE,0x200,0,255,4);c.until(3);
  c.send(0,REQUEST_TYPE_WRITE,0x8000,888);c.until(4);c.run(100);
  assert(c.requests[3].mmu_command==MMU_WRITE_DWORD); // certificate revoked
}
int main() {
  setenv("AJIT_MODULAR_ASYNC","1",1);
  for(bool reverse:{false,true}) {
    exact_phase_timing(reverse);offsets_and_recheck(reverse);stores_and_forwarding(reverse);
    full_and_barrier(reverse);faults_and_invalidation(reverse);atomic_and_context(reverse);
  }
  native_mode=true;
  exact_phase_timing(false);offsets_and_recheck(false);stores_and_forwarding(false);full_and_barrier(false);
  faults_and_invalidation(false);atomic_and_context(false);
  std::puts("PASS: phase1 sends/phase0 receives, exact idle miss/read hit/store ACK timing, native SiTAR parallel scheduler, atomic ownership, context revocation, offsets, critical response, refill hits, queued recheck, forwarding, full queue, STBAR, precise faults, NF/uncached termination, invalidation, both branch orders");
}
