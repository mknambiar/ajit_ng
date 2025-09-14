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
		
        bool pullword_fromdword(void *obj, uint32_t *value, bool sync, uint8_t even_odd) {
            token<sizeof(uint64_t)> tval;
			uint64_t data64;
            bool rc;
            
            if (sync == false)
                rc = static_cast<inport<sizeof(uint64_t)>*>(obj)->pull(tval);
            else
                while (!(rc = static_cast<inport<sizeof(uint64_t)>*>(obj)->pull(tval)));
            
            memcpy(&data64, tval.data(), sizeof(uint64_t));
			
			if(even_odd) 
				*value = data64;
			else
				*value = (data64)>>32;
	
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
        
        bool pullline(void *obj, char line_data[], bool sync) {
            token<sizeof(uint64_t)*8> tval;
            bool rc;
            
            if (sync == false)
                rc = static_cast<inport<sizeof(uint64_t)*8>*>(obj)->pull(tval);
            else
                while (!(rc = static_cast<inport<sizeof(uint64_t)*8>*>(obj)->pull(tval)));
            
            memcpy(line_data, tval.data(), sizeof(uint64_t)*8);
            return rc;

        }

        bool pushline(void *obj, char line_data[], bool sync) {
            token<sizeof(uint64_t)*8> tval;
            bool rc;

            memcpy(tval.data(), line_data,  sizeof(uint64_t));
             
            if (sync == false)
                rc = static_cast<outport<sizeof(uint64_t)*8>*>(obj)->push(tval);
            else   
                while (!(rc = static_cast<outport<sizeof(uint64_t)*8>*>(obj)->push(tval)));
            
            return rc;

        }        

        uint8_t arbitrate(uint8_t *mystatic_last_served, uint8_t *mystatic_fifo_number, uint8_t num_ports, void *obj[]) {
            token<sizeof(uint8_t)> tval;
            uint8_t rval;
            bool found = false;
            bool rc;
            uint8_t port_number = ((*mystatic_fifo_number) + 1) % num_ports;
            uint8_t count = 0;

            if (port_number == *mystatic_last_served)
                    port_number = (port_number + 1) % num_ports; /*Skip the last served guy*/
                    
            while(!(found)) {
                rc = static_cast<inport<sizeof(uint8_t)>*>(obj[port_number])->pull(tval);
            
                if (rc) {
                    found = true;
                    if (count == 0)
                        *mystatic_fifo_number = port_number;
                    *mystatic_last_served = port_number;
                    memcpy(&rval, tval.data(), sizeof(uint8_t));
                    break;
                }
                        
                count++;
                port_number = (port_number + 1) % num_ports;
                if ((port_number == *mystatic_last_served) && (count < num_ports))
                        port_number = (port_number + 1) % num_ports; /*Skip the last served guy*/               
            
            }
        
            return rval; // calls always synchronous
        
        }
    }

}
