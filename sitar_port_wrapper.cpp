#include <string.h>
#include "sitar_inport.h"
#include "sitar_outport.h"

namespace sitar {
    // Explicit instantiation of the template for int and double

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
            token<sizeof(uint8_t)> tval;
            bool rc;

            if (sync == false)
                rc = static_cast<inport<sizeof(uint8_t)>*>(obj)->pull(tval);
            else
                while (!(rc = static_cast<inport<sizeof(uint8_t)>*>(obj)->pull(tval)));
            
            *value = tval.data()[0];
            return rc;
        }

        bool pushchar(void *obj, uint8_t value, bool sync) {
            token<sizeof(uint8_t)> tval;
            bool rc;
            
            tval.data()[0] = value;

            if (sync == false)
                rc = static_cast<outport<sizeof(uint8_t)>*>(obj)->push(tval);
            else
                while (!(rc = static_cast<outport<sizeof(uint8_t)>*>(obj)->push(tval)));
            
            return rc;
        }

        bool pullword(void *obj, uint32_t *value, bool sync) {
            token<sizeof(uint32_t)> tval;
            bool rc;
            
            if (sync == false)
                rc = static_cast<inport<sizeof(uint32_t)>*>(obj)->pull(tval);
            else
                while (!(rc = static_cast<inport<sizeof(uint32_t)>*>(obj)->pull(tval)));
            
            memcpy(value, tval.data(), sizeof(uint32_t));
            return rc;

        }

        bool pushword(void *obj,uint32_t value, bool sync) {
            token<sizeof(uint32_t)> tval;
            bool rc;

            memcpy(tval.data(), &value,  sizeof(uint32_t));
             
            if (sync == false)
                rc = static_cast<outport<sizeof(uint32_t)>*>(obj)->push(tval);
            else   
                while (!(rc = static_cast<outport<sizeof(uint32_t)>*>(obj)->push(tval)));
            
            return rc;

        }
		
        bool pulldword(void *obj, uint64_t *value, bool sync) {
            token<sizeof(uint64_t)> tval;
            bool rc;
            
            if (sync == false)
                rc = static_cast<inport<sizeof(uint64_t)>*>(obj)->pull(tval);
            else
                while (!(rc = static_cast<inport<sizeof(uint64_t)>*>(obj)->pull(tval)));
            
            memcpy(value, tval.data(), sizeof(uint64_t));
            return rc;

        }

        bool pushdword(void *obj,uint64_t value, bool sync) {
            token<sizeof(uint64_t)> tval;
            bool rc;

            memcpy(tval.data(), &value,  sizeof(uint64_t));
             
            if (sync == false)
                rc = static_cast<outport<sizeof(uint64_t)>*>(obj)->push(tval);
            else   
                while (!(rc = static_cast<outport<sizeof(uint64_t)>*>(obj)->push(tval)));
            
            return rc;

        }		
    }

}
