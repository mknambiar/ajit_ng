#include <string.h>
#include "sitar_inport.h"
#include "sitar_outport.h"
#include <cstdlib>
#include <cstdio>
#include <map>

namespace {
// Opt-in observation only: do not peek, consume, or inject extra net tokens.
void observe_signal(sitar::object* port, bool push, bool ok, uint8_t value) {
    static const bool enabled = [] {
        const char* p = std::getenv("AJIT_IRQ_DIAGNOSTICS");
        return p && p[0] && p[0] != '0';
    }();
    if (!enabled) return;
    const auto& name = port->instanceId();
    if (name != "control_word" && name != "irq_level" &&
        name != "timer_irq_level" && name != "serial_rx_irq_level" &&
        name.find("thread_irl") != 0 && name != "console_rx_valid" &&
        name != "console_rx_ack" && name != "rx_full_status") return;
    struct Counts {
        uint64_t calls=0, success=0, changes=0;
        unsigned last=256;
    };
    // Each port belongs to one statically scheduled module/worker.
    static thread_local std::map<sitar::object*, Counts> counts;
    auto& c = counts[port];
    ++c.calls;
    if (ok) ++c.success;
    bool changed = ok && c.last != value;
    if (changed) { c.last=value; ++c.changes; }
    const bool change_sample = changed && (c.changes <= 16 || !(c.changes & (c.changes-1)));
    const bool periodic = !(c.calls & (c.calls-1));
    if (change_sample || periodic)
        std::fprintf(stderr, "IRQ-NET port=%s op=%s calls=%llu success=%llu changes=%llu last=%u ok=%u\n",
            port->hierarchicalId().c_str(), push ? "push" : "pull",
            (unsigned long long)c.calls, (unsigned long long)c.success,
            (unsigned long long)c.changes, c.last, ok ? 1u : 0u);
}
}

namespace sitar {
    extern "C" {
        bool pullbool(void *obj, bool *value, bool sync) {
            token<1> tval;
            unsigned char bval;
            bool rc;

            if (sync == false)
                rc = static_cast<inport<1>*>(obj)->pull(tval);
            else 
                while (!(rc = static_cast<inport<1>*>(obj)->pull(tval)));
            bval = tval.data()[0];
            *value = bval?true:false;
            return rc;
        }

        bool pushbool(void *obj, bool value, bool sync) {
            token<1> tval;
            unsigned char bval = value?1:0;
            bool rc;
            
            tval.data()[0] = bval;
            
            if (sync == false)
                rc = static_cast<outport<1>*>(obj)->push(tval);
            else
                while (!(rc = static_cast<outport<1>*>(obj)->push(tval)));
            
            return rc;
        }
        
        bool pullchar(void *obj, uint8_t *value, bool sync) {
            token<sizeof(uint8_t)*8> tval;
            bool rc;

            if (sync == false)
                rc = static_cast<inport<sizeof(uint8_t)*8>*>(obj)->pull(tval);
            else
                while (!(rc = static_cast<inport<sizeof(uint8_t)*8>*>(obj)->pull(tval)));
            
            *value = tval.data()[0];
            observe_signal(static_cast<inport<8>*>(obj), false, rc, *value);
            return rc;
        }

        bool pushchar(void *obj, uint8_t value, bool sync) {
            token<sizeof(uint8_t)*8> tval;
            bool rc;
            
            tval.data()[0] = value;

            if (sync == false)
                rc = static_cast<outport<sizeof(uint8_t)*8>*>(obj)->push(tval);
            else
                while (!(rc = static_cast<outport<sizeof(uint8_t)*8>*>(obj)->push(tval)));

            observe_signal(static_cast<outport<8>*>(obj), true, rc, value);
            
            return rc;
        }

        bool pullword(void *obj, uint32_t *value, bool sync) {
            token<sizeof(uint32_t)*8> tval;
            bool rc;
            
            if (sync == false)
                rc = static_cast<inport<sizeof(uint32_t)*8>*>(obj)->pull(tval);
            else
                while (!(rc = static_cast<inport<sizeof(uint32_t)*8>*>(obj)->pull(tval)));
            
            memcpy(value, tval.data(), sizeof(uint32_t));
            return rc;

        }

        bool pushword(void *obj,uint32_t value, bool sync) {
            token<sizeof(uint32_t)*8> tval;
            bool rc;

            memcpy(tval.data(), &value,  sizeof(uint32_t));
             
            if (sync == false)
                rc = static_cast<outport<sizeof(uint32_t)*8>*>(obj)->push(tval);
            else   
                while (!(rc = static_cast<outport<sizeof(uint32_t)*8>*>(obj)->push(tval)));
            
            return rc;

        }

        bool pulldword(void *obj, uint64_t *value, bool sync) {
            token<sizeof(uint64_t)*8> tval;
            bool rc;

            if (sync == false)
                rc = static_cast<inport<sizeof(uint64_t)*8>*>(obj)->pull(tval);
            else
                while (!(rc = static_cast<inport<sizeof(uint64_t)*8>*>(obj)->pull(tval)));

            memcpy(value, tval.data(), sizeof(uint64_t));
            return rc;
        }

        bool pushdword(void *obj,uint64_t value, bool sync) {
            token<sizeof(uint64_t)*8> tval;
            bool rc;

            memcpy(tval.data(), &value,  sizeof(uint64_t));
             
            if (sync == false)
                rc = static_cast<outport<sizeof(uint64_t)*8>*>(obj)->push(tval);
            else   
                while (!(rc = static_cast<outport<sizeof(uint64_t)*8>*>(obj)->push(tval)));
            
            return rc;

        }
    }
}
