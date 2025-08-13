//-------------------------------------------------------------------------
// Describes the ajit thread 
//-------------------------------------------------------------------------
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>
#include "Opcodes.h"
#include "ajit_ng.h"
#include "AjitThreadS.h"
#include "CoreS.h"
#include "Traps.h"
#include "Flags.h"
#include "Ancillary.h"
#include "Ajit_Hardware_Configuration.h"
#include "RegisterFile.h"
#include "ThreadInterfaceS.h"
#include "Decode.h"
#include "ExecuteS.h"
//#include "monitorLogger.h"
//#include "Pipes.h"
#include "StatsKeeping.h"
#include "ThreadLogging.h"
//#include "ThreadHWserverInterface.h"
#include "rlut.h"
#include "ImplementationDependent.h"
#include "AjitThreadUtils.h"


int global_verbose_flag = 0;
int use_instruction_buffer = 0;


void increment_instruction_count(ThreadState* s) 
{ 
	s->num_instructions_executed++; 
}

uint64_t get_instruction_count(ThreadState* s) 
{ 
	return s->num_instructions_executed; 
}

uint64_t getCycleEstimate (ThreadState* s)
{
	uint64_t ret_val = s->num_instructions_executed;
	
	//Till we get a new MMU and cache structures
	
	//ret_val += (s->mmu_state->Num_Mmu_translated_accesses - s->mmu_state->Num_Mmu_TLB_hits)*
	//			TLB_MISS_PENALTY;
	//ret_val += s->dcache->number_of_misses * CACHE_MISS_PENALTY;
	//ret_val += s->icache->number_of_misses * CACHE_MISS_PENALTY;
	
	ret_val += s->num_fp_sp_sqroots_executed * FP_SP_SQROOT_PENALTY;
	ret_val += s->num_fp_dp_sqroots_executed * FP_DP_SQROOT_PENALTY;

	ret_val += s->num_fp_sp_divs_executed * FP_SP_DIV_PENALTY;
	ret_val += s->num_fp_dp_divs_executed * FP_DP_DIV_PENALTY;

	ret_val += s->num_iu_divs_executed * IU_DIV_PENALTY;

	ret_val += s->num_traps * TRAP_PENALTY;
	ret_val += s->branch_predictor.mispredicts * CTI_MISPREDICT_PENALTY;

	return(ret_val);
}

void  	printThreadStatistics (ThreadState* s)
{
	fprintf(stderr,"\nStatistics for Thread %d (Core %d):\n", s->thread_id, s->core_id);

	fprintf(stderr,"    number-of-instructions-executed = %" PRIu64 "\n", 
						s->num_instructions_executed);
	fprintf(stderr,"    number-of-traps  = %u\n", 
						s->num_traps);
	fprintf(stderr,"    number-of-cti  = %u, mispredicts=%u\n", 
						s->branch_predictor.branch_count,
						s->branch_predictor.mispredicts);
	fprintf(stderr,"    cycle-count estimate = %lu\n",getCycleEstimate(s));
	if(s->i_buffer)
		reportInstructionDataBufferStatistics(s->i_buffer);	


	dumpBranchPredictorStatus(&(s->branch_predictor));
	dumpReturnAddressStackStatus(&(s->return_address_stack));
}


uint32_t getAsrValue (ThreadState* thread_state, int asr_id)
{
	uint32_t res = 0;
	if((asr_id == 31) || (asr_id == 30))
	{
		uint64_t cycle_count = getCycleEstimate (thread_state);			
		if(asr_id == 31)
		{
			cycle_count = cycle_count >> 32;
		}
		res = cycle_count & 0xffffffff;
	}
	else
		res = thread_state->status_reg.asr[asr_id] ;    

	return(res);
}

//For time-keeping during simulation:
#include <time.h>
time_t simulation_time;

void 	clearStateUpdateFlags( ThreadState *s);
//void 	generateLogMessage( ThreadState *s);
uint8_t isBranchInstruction(Opcode op, InstructionType type);
uint8_t isFloatCompareInstruction(Opcode op);

uint32_t completeFPExecution (ThreadState* s,
					Opcode opcode, 
					uint32_t operand2_3, 
					uint32_t operand2_2, 
					uint32_t operand2_1, 
			     		uint32_t operand2_0, 
					uint32_t operand1_3, 
					uint32_t operand1_2, 
					uint32_t operand1_1, 
					uint32_t operand1_0,
					uint8_t dest_reg, 
					StatusRegisters *ptr,	
					uint32_t trap_vector);
void writeBackResult( RegisterFile* rf,
					uint8_t dest_reg, 
					uint32_t result_h, 
					uint32_t result_l, 
					uint8_t flags, 
					uint8_t cwp, 
					uint32_t pc, 
					StateUpdateFlags* reg_update_flags);



void clearStateUpdateFlags( ThreadState *s)
{
	StateUpdateFlags *flags = &(s->reg_update_flags);

	flags->psr_updated=0;   
	flags->wim_updated=0;   
	flags->tbr_updated=0;   
	flags->y_updated=0;     
	flags->gpr_updated=0;   

	flags->fpreg_updated=0;
	flags->fpreg_double_word_write=0;
	flags->fpreg_id=0;
	flags->fpreg_val_high=0;
	flags->fpreg_val_low=0;
	flags->fsr_updated=0;
	flags->ftt_updated=0;
	flags->fcc_updated=0;
	flags->fsr_val=0; 

	flags->store_active = 0;
	flags->double_word_write=0;
	flags->reg_id=0;       
	flags->reg_val_high=0; 
	flags->reg_val_low=0;  
	flags->pc=0;
}

void sitar_init_ajit_thread (ThreadState *s, uint32_t core_id,
			uint32_t thread_id, uint32_t isa_mode, int bp_table_size, 
			uint8_t report_traps, uint32_t init_pc)
{
	s->core_id = core_id;
	s->thread_id = thread_id;
	s->isa_mode = isa_mode;
	s->register_file  =  makeRegisterFile ();


	// instruction buffer
	if(use_instruction_buffer)
	{
		s->i_buffer = (InstructionDataBuffer*) malloc (sizeof(InstructionDataBuffer));
		initInstructionDataBuffer(s->i_buffer,
				s->core_id,
				s->thread_id,
				1, 
				INSTRUCTION_BUFFER_N_ENTRIES,
				(INSTRUCTION_BUFFER_N_ENTRIES/INSTRUCTION_BUFFER_ASSOCIATIVITY));
	}
	else
	{
		s->i_buffer = NULL;
	}



	// these maye get populated
	// by someone else.

	initBranchPredictorState(&(s->branch_predictor), bp_table_size);
	resetThreadState (s);

	initReturnAddressStack(&(s->return_address_stack));

	s->reset_called_once = 0;

	//input signals to the cpu hardwired to 
	//a certain value in the Ajit processor:
	s->bp_fpu_present = isFpuPresent(s->core_id, s->thread_id);
	s->bp_div_present = isDivPresent(s->core_id, s->thread_id);
	s->bp_fpu_exception = 0;
	s->bp_fpu_cc=0;
	s->bp_cp_present = 0;
	s->bp_cp_exception = 0;
	s->bp_cp_cc=0;

	s->pb_error = 0;
	s->pb_block_ldst_word = 0;
	s->pb_block_ldst_byte = 0;

	s->report_traps = report_traps;
	s->init_pc = init_pc;

}

void resetThreadState(ThreadState *s)
{
	StatusRegisters *status_reg = &(s->status_reg);

	s->instruction = 0;		
	s->addr_space = 0;		
	s->mode =  _RESET_MODE_;	
	s->trap_vector = 0;		
	s->interrupt_level = 0;		
	s->ticc_trap_type = 0;		
	s->store_barrier_pending = 0;	

	status_reg->psr =  0x10C0 ;
	//PSR fields:
	//	current window pointer CWP (bits 4:0 )	= 0000
	//	enable traps		ET (bit  5   )	= 0
	//	previous supervisor	PS (bit  6   )	= 1
	//	supervisor		S  (bit  7   )	= 1
	//	proc interrupt level   PIL (bits 11:8)	= 0000
	//	enable FPU              EF (bit 12   )	= 1
	//	---all others are zero-------
	status_reg->wim =  0 ;
	status_reg->tbr =  0 ;
	status_reg->y   =  0 ;
	status_reg->pc  =  0 ;
	status_reg->npc =  0 ;
	status_reg->iq  =  0 ;
	status_reg->fsr =  0 ;
	status_reg->fq  =  0 ;
	status_reg->csr =  0 ;
	status_reg->cq  =  0 ;

	uint8_t i = 0;
	for(; i < 32; i++)
	{
		//
		// initialize status registers to 0.
		// 
		status_reg->asr[i] = 0x0;
	}

	//
	// asr 29, maintains the cpu-id
	//
	status_reg->asr[29] = 0x50520000 | (s->core_id << 8) | s->thread_id;

	//
	// asr 28, maintains the half-precision floating
	// point format's exponent width.  This number has
	// a default value, but this value can be overridden
	// by the programmer.
	//
	status_reg->asr[28] = DEFAULT_HALF_PRECISION_EXPONENT_WIDTH;

	s->num_ifetches = 0;
	s->num_instructions_executed = 0;
	s->num_traps = 0;

	s->num_fp_sp_sqroots_executed = 0;
	s->num_fp_dp_sqroots_executed = 0;

	s->num_fp_sp_divs_executed = 0;
	s->num_fp_dp_divs_executed = 0;

	s->num_iu_divs_executed = 0;


	resetRegisterFile(s->register_file);

	clearStateUpdateFlags(s);
	resetBranchPredictorState(&(s->branch_predictor));


	s->THREAD_HW_SERVER_ENABLED =0;
	s->THREAD_GDB_FLAG=0;
	s->THREAD_DOVAL_FLAG=0;



}


//if logging is enabled,
//generate a signature
//and send it out to the Logger pipes
//Lets comment it out for now

/* void generateLogMessage( ThreadState *s)
{

	static int64_t log_counter = 0;

	uint32_t reg_write_log=0;
	uint32_t fp_reg_write_log=0;
	uint32_t store_log=0;
	uint32_t pc_log=0;

	StatusRegisters *r = &(s->status_reg);
	StateUpdateFlags *f = &(s->reg_update_flags);

	//Generate the signatures
	reg_write_log = assembleRegisterWriteSignature(
			f->psr_updated, r->psr, // uint8_t psr_updated, uint32_t psr_value,
			f->wim_updated, r->wim, //uint8_t wim_updated, uint32_t wim_value,
			f->tbr_updated, r->tbr, //uint8_t tbr_updated, uint32_t tbr_value,
			f->y_updated,   r->y,	//uint8_t y_updated, uint32_t y_value,
			f->gpr_updated,		//uint8_t gpr_updated,
			f->double_word_write, 	//uint8_t double_word_write,
			f->reg_id, 
			f->reg_val_high, 
			f->reg_val_low);

	 fp_reg_write_log = assembleFpRegisterWriteSignature(
                                        f->fpreg_updated,
                                        0,
                                        (f->fsr_updated | f->ftt_updated | f->fcc_updated),
                                        0,
                                        f->fsr_updated,
                                        f->ftt_updated,
                                        f->fcc_updated,
                                        f->fpreg_double_word_write,
                                        f->fpreg_id,
                                        f->fsr_val,
                                        f->fpreg_val_high,
                                        f->fpreg_val_low);

	store_log = assembleStoreSignature( 
		f->store_active, 	
		f->store_asi,		
		f->store_byte_mask,	
		f->store_double_word,
		f->store_addr,
		f->store_word_high,
		f->store_word_low);

	pc_log = f->pc;

	
	if(global_verbose_flag)
	{
		fprintf(stderr,"Info:psr_update: pc=0x%x psr=0x%x\n", f->pc, r->psr);
		if(f->gpr_updated)
		{
			fprintf(stderr,"Info: iu-reg updated: pc=0x%x, psr=0x%x, "
					"reg-id=0x%x, double=%d, reg-vals 0x%x 0x%x\n",
					f->pc, r->psr, f->reg_id, f->double_word_write, 
					f->reg_val_high, f->reg_val_low);
		}
	}

	//if(f->pc) pc_log = f->pc; //pc was updated inside an instruction
	//else      pc_log = r->pc; //pc to be logged is same as the current pc

	//send to logger pipe
	writeToLoggerPipe(s, reg_write_log);
	writeToLoggerPipe(s, fp_reg_write_log);
	writeToLoggerPipe(s, store_log);
	writeToLoggerPipe(s, pc_log);

	log_counter++;
} */

	


uint8_t isBranchInstruction(Opcode op, InstructionType type)
{
	return ((op == _CALL_) || (op == _RETT_) || (op == _JMPL_) || (type == _CC_INS_) || (type == _TICC_INS_)); 
}

uint8_t isFloatCompareInstruction(Opcode op)
{
	return ((op == _FCMPs_) || (op == _FCMPd_) || (op == _FCMPq_) || (op == _FCMPEs_) || (op == _FCMPEd_) || (op == _FCMPEq_));
}

// Forward declare the wrapper functions
extern bool pullchar(void *obj, uint8_t *value, bool sync);
extern bool pushchar(void *obj, uint8_t value, bool sync);
extern bool pullword(void *obj, uint32_t *value, bool sync);
extern bool pushword(void *obj,uint32_t value, bool sync);
extern bool pulldword(void *obj, uint64_t *value, bool sync);
extern bool pushdword(void *obj,uint64_t value, bool sync);



int fetchInstruction_split_1(ThreadState* s,  
				uint8_t addr_space, uint32_t addr , 
				icache_out *ic_out)
{

	uint8_t acc;
	int is_buffer_hit = 0;
	bool sync = true;
	bool rc;
	
	if(s->i_buffer != NULL)
	{
		is_buffer_hit = lookupInstructionDataBuffer (s->i_buffer, addr & 0xfffffff8, &acc, &ic_out->inst_pair);
		is_buffer_hit = is_buffer_hit && privilegesOk(addr_space & 0x7f, 1, 1, acc);
	}
	
	if(!is_buffer_hit)
	{
        //Starting Sending stuff to I cacheable
        // This should be a function by itself
        rc = pushchar(ic_out->i_asi_port, addr_space, sync); 
        assert(rc == true);		
        rc = pushword(ic_out->i_addr_port, addr & 0xfffffff8, sync); 
        assert(rc == true);		
        rc = pushchar(ic_out->i_request_type_port, REQUEST_TYPE_IFETCH, sync); 
        assert(rc == true);		    
        rc = pushchar(ic_out->i_byte_mask_port, 0xff, sync); 
        assert(rc == true);
		ic_out->push_done = 1;

    }
    
	return is_buffer_hit;
	
}

uint8_t fetchInstruction_split_2(ThreadState* s,  
			 uint32_t addr, uint32_t *inst, uint32_t* mmu_fsr,
			 icache_out *ic_out,
			 icache_in *ic_in)

{

    bool sync = true;
    bool rc;
    uint64_t ipair;
	
	ipair = ic_out->inst_pair;
	
	if(!ic_out->push_done)
	{
        
        //readInstructionPair
        //mae_value has already been read
        
        rc = pulldword(ic_in->i_inst_pair_port, &ipair, sync);
        assert(rc == true);        
        rc = pullword(ic_in->i_mmu_fsr_port, mmu_fsr, sync);
        assert(rc == true);        
        
		if(s->i_buffer != NULL)
		{
			if((ic_out->mae  & 0x3) == 0)
			{
				uint8_t cacheable = (ic_out->mae >> 7) & 0x1;
				if(cacheable)
				{
					uint8_t acc = (ic_out->mae >> 4) & 0x7;
					insertIntoInstructionDataBuffer(s->i_buffer,
							addr & 0xfffffff8,
							acc,
							ipair);
				}
			}
		}
	}

	setPageBit((CoreState*) s->parent_core_state,addr);

	if(getBit32(addr,2)) 
		*inst = ipair;
	else 
		*inst = ipair>>32;

	if(global_verbose_flag)
	{
	   fprintf(stderr,"Info:fetchInstruction addr=0x%x instr=0x%x  buf-hit=%d\n", addr, *inst, ic_out->push_done);
	}

	// only bottom bit of mae value is used
	return (0x1 & ic_out->mae);					
					
}					


void writeBackResult(RegisterFile* rf, uint8_t dest_reg, uint32_t result_h, uint32_t result_l, uint8_t flags, uint8_t cwp, uint32_t pc, StateUpdateFlags* reg_update_flags)
{
	uint8_t is_float_instruction = (getBit8(flags, _FLOAT_INSTRUCTION_) == 1);
	uint8_t is_double_result = (getBit8(flags, _DOUBLE_RESULT_) == 1);

	//IU write-back
	if(!is_float_instruction)
	{
		if(!is_double_result)
		{
			writeRegister(pc, rf,  dest_reg, cwp, result_l);
		}
		else
		{
			writeRegister(pc, rf,  setBit8(dest_reg, 0, 0), cwp, result_h);
			writeRegister(pc, rf, setBit8(dest_reg, 0, 1), cwp, result_l);
		}

		//Information to be logged:
		reg_update_flags->gpr_updated=1;
		if(is_double_result) 
		{
			reg_update_flags->double_word_write=1; 
		}
		else
		{
			reg_update_flags->double_word_write=0; 
		}
		reg_update_flags->reg_id=dest_reg;

		reg_update_flags->reg_val_high=result_h;
		reg_update_flags->reg_val_low=result_l;
	}
	//FP write-back
	else
	{
		reg_update_flags->fpreg_updated = 1;
		reg_update_flags->fpreg_val_low = result_l;
		reg_update_flags->fpreg_id = dest_reg;

		if(!is_double_result)
		{
			writeFRegister(rf, dest_reg, result_l); 
		}
		else
		{
			dest_reg = setBit8(dest_reg, 0, 0) ;
			writeFRegister(rf, dest_reg, result_h);
			dest_reg = setBit8(dest_reg, 0, 1) ;
			writeFRegister(rf, dest_reg, result_l);

			reg_update_flags->fpreg_double_word_write = 1;
			reg_update_flags->fpreg_val_high = result_h;
		}
	}
}

uint32_t completeFPExecution(ThreadState* s,
		Opcode opcode, 
		uint32_t operand2_3, uint32_t operand2_2, 
		uint32_t operand2_1, uint32_t operand2_0, 
		uint32_t operand1_3, uint32_t operand1_2, 
		uint32_t operand1_1, uint32_t operand1_0,
		uint8_t dest_reg, StatusRegisters *ptr, uint32_t trap_vector)
{
	RegisterFile* rf = s->register_file;
	uint32_t tv, result_l2, result_l1, result_h2, result_h1;
	uint8_t texc, aexc, tem, c, tfcc;
	uint8_t flags = 0;
	uint8_t ft = 0;
	// StatusRegisters current_sr = *ptr; 

	tv = executeFPInstruction(s, opcode, operand2_3, operand2_2, operand2_1, operand2_0, operand1_3, operand1_2, operand1_1, operand1_0,
			&result_h2, &result_h1, &result_l2, &result_l1, trap_vector, &flags, &ft, ptr);
	c = getBit8(ft, 0);
	texc = getSlice8(ft, 5, 1);
	tfcc = getSlice8(ft, 7, 6);

	tem = getSlice32((*ptr).fsr, 27, 23);
	aexc = getSlice32((*ptr).fsr, 9, 5);

	uint8_t no_trap = (getBit32(tv, _TRAP_) == 0);
	uint8_t execute_fp_op =  no_trap;

	uint8_t no_fpu_present = (getBpFPUPresent(s) == 0);
	uint8_t unimplemented = execute_fp_op && no_fpu_present;
	if(unimplemented)
	{
		tv = setBit32(tv, _TRAP_, 1) ;
		tv = setBit32(tv, _FP_EXCEPTION_, 1);
		ptr->fsr = setSlice32((*ptr).fsr, 16, 14, _UNIMPLEMENTED_FPOP_) ;

		s->reg_update_flags.ftt_updated = 1;
		s->reg_update_flags.fsr_val = ptr->fsr;

	}

	uint8_t incomplete = (c == 0);
	uint8_t unfinished = execute_fp_op && !no_fpu_present && incomplete;
	if(unfinished)
	{
		tv = setBit32(tv, _TRAP_, 1) ;
		tv = setBit32(tv, _FP_EXCEPTION_, 1) ;
		ptr->fsr = setSlice32((*ptr).fsr, 16, 14, _UNFINISHED_FPOP_) ;
		s->reg_update_flags.ftt_updated = 1;
		s->reg_update_flags.fsr_val = ptr->fsr;

	}

	uint8_t ex = (texc != 0) && (tem != 0);
	uint8_t execute = execute_fp_op && !no_fpu_present && !incomplete;
	uint8_t exception = execute && ex;

	if( (exception)) 
	{
		ptr->fsr = setSlice32((*ptr).fsr, 4, 0, (getBit8(texc, 5) & getBit8(tem, 5))) ;
		tv = setBit32(tv, _TRAP_, 1) ;
		tv = setBit32(tv, _FP_EXCEPTION_, 1) ;
		ptr->fsr = setSlice32((*ptr).fsr, 16, 14, _IEEE_754_EXCEPTION_) ;

		s->reg_update_flags.ftt_updated = 1;
		s->reg_update_flags.fsr_val = ptr->fsr;

	}

	uint8_t no_exception = execute && !ex;
	if( (no_exception))
	{
		ptr->fsr = setSlice32((*ptr).fsr, 4, 0, texc) ;
		ptr->fsr = setSlice32((*ptr).fsr, 9, 5, (aexc | texc)) ;
	}

	uint8_t is_single_result = (getBit8(flags, _SINGLE_RESULT_) == 1);
	uint8_t is_double_result = (getBit8(flags, _DOUBLE_RESULT_) == 1);
	uint8_t is_quad_result = (getBit8(flags, _QUAD_RESULT_) == 1);

	if(!(!no_exception || !is_single_result)) {

		 writeFRegister(rf,dest_reg, result_l1);

		s->reg_update_flags.fpreg_updated = 1;
                s->reg_update_flags.fpreg_val_low = result_l1;
                s->reg_update_flags.fpreg_id = dest_reg;

	}
	if( (is_double_result)) dest_reg = setBit8(dest_reg, 0, 0) ;

	if(!(!no_exception || !is_double_result)) {

		writeFRegister(rf,dest_reg, result_l2);

		s->reg_update_flags.fpreg_updated = 1;
                s->reg_update_flags.fpreg_id = dest_reg;
                s->reg_update_flags.fpreg_val_low = result_l1;
                s->reg_update_flags.fpreg_val_high = result_l2;
                s->reg_update_flags.fpreg_double_word_write = 1;

	}

	if( (is_double_result)) dest_reg = setBit8(dest_reg, 0, 1) ;
	if(!(!no_exception || !is_double_result)) writeFRegister(rf,dest_reg, result_l1);

	if( (is_quad_result)) dest_reg = setSlice8(dest_reg, 1, 0, 0) ;
	if(!(!no_exception || !is_quad_result))	writeFRegister(rf, dest_reg, result_h2);
	if( (is_quad_result)) dest_reg = setBit8(dest_reg, 0, 1) ;
	if(!(!no_exception || !is_quad_result))	writeFRegister(rf, dest_reg, result_h1);
	if( (is_quad_result)) dest_reg = setBit8(dest_reg, 1, 1) ;
	if(!(!no_exception || !is_quad_result))	writeFRegister(rf, dest_reg, result_l1);
	if( (is_quad_result)) dest_reg = setBit8(dest_reg, 0, 0) ;
	if(!(!no_exception || !is_quad_result)) writeFRegister(rf, dest_reg, result_l2);

	uint8_t compare_inst = no_exception && isFloatCompareInstruction(opcode);

	if( (compare_inst)){
		 ptr->fsr = setSlice32((*ptr).fsr, 11, 10, tfcc) ;

		 s->reg_update_flags.fcc_updated = 1;
		 s->reg_update_flags.fsr_val = ptr->fsr;
	}

	if( (no_exception)) ptr->fsr = setSlice32((*ptr).fsr, 16, 14, 0) ;

	return tv;
}

uint32_t wim(ThreadState* s)
{
	return(s->status_reg.wim);
}

uint32_t stack_pointer(ThreadState* s)
{	
	// Stack pointer is register o6 that is r14.
	uint32_t ret_val = readRegister(s->register_file, 14, (s->status_reg.psr & 0x7));
	return(ret_val);
}

uint32_t frame_pointer(ThreadState* s)
{
	// Stack pointer is register i6 that is r30.
	uint32_t ret_val = readRegister(s->register_file, 30, (s->status_reg.psr & 0x7));
	return(ret_val);
}


void initReturnAddressStack(ReturnAddressStack* ras)
{
	int I;
	for(I = 0; I < RAS_DEPTH; I++)
	{
		ras->entries[I] = 0;
	}

	ras->push_count = 0;
	ras->pop_count  = 0;
	ras->mispredicts = 0;
	ras->write_pointer = 0;
	ras->read_pointer  = 0;
	ras->surplus = 0;
}

void pushIntoReturnAddressStack(ReturnAddressStack* ras, uint32_t return_address)
{

	ras->entries[ras->write_pointer] = (return_address | 0x1);
	ras->read_pointer = ras->write_pointer;
	ras->write_pointer = (ras->write_pointer+1) & (RAS_DEPTH-1);
	ras->push_count++;
	ras->surplus++;

	if(global_verbose_flag)
		fprintf(stderr,"RAS: pushed 0x%x [%d]\n", return_address, ras->surplus);

}
uint32_t popFromReturnAddressStack(ReturnAddressStack* ras)
{
	uint32_t ret_val = ras->entries[ras->read_pointer];
	ras->entries[ras->read_pointer] = 0;
	ras->write_pointer = ras->read_pointer;
	ras->read_pointer  = (ras->read_pointer + RAS_DEPTH - 1) & (RAS_DEPTH-1);

	ras->pop_count++;
	ras->surplus--;

	if(global_verbose_flag)
		fprintf(stderr,"RAS: popped 0x%x [%d]\n", ret_val & (~0x1), ras->surplus);
	return(ret_val);
}

uint32_t topOfReturnAddressStack(ReturnAddressStack* ras)
{
	return(ras->entries[ras->read_pointer]);
}

void incrementRasMispredicts(ReturnAddressStack* ras)
{
	ras->mispredicts++;
}

void dumpReturnAddressStackStatus(ReturnAddressStack* ras)
{
	fprintf(stderr,"    RAS: push-count=%d, pop-count=%d, mispredicts=%d (surplus=%d).\n", ras->push_count, ras->pop_count, ras->mispredicts, ras->surplus);
}
void initBranchPredictorState(BranchPredictorState* bps, uint32_t tsize)
{
	bps->table_size = tsize;
	bps->pc_tags    = (uint32_t*) malloc(tsize*sizeof(uint32_t));
	bps->nnpc_values = (uint32_t*) malloc(tsize*sizeof(uint32_t));
	bps->saturating_counters = (uint8_t*) malloc(tsize*sizeof(uint8_t));
}

void resetBranchPredictorState(BranchPredictorState* bps)
{
	bps->mispredicts = 0;
	bps->branch_count = 0;

	uint32_t I;
	for(I=0; I < bps->table_size; I++)
	{
		bps->pc_tags[I] = 0;
		bps->saturating_counters[I] = 0;
	}
}

void addBranchPredictEntry (BranchPredictorState* bps, uint8_t br_taken, uint32_t pc, uint32_t nnpc)
{
	uint32_t index = (pc >> 2) % bps->table_size;
		
	bps->pc_tags[index] = pc | 1;
	bps->nnpc_values[index] = nnpc;	

	if(br_taken)
		bps->saturating_counters[index] = 0;
	else 
		bps->saturating_counters[index] = BP_MAX_COUNT_VALUE -1 ;
}

int branchPrediction(BranchPredictorState* bps, uint32_t pc, uint32_t npc, uint32_t* bp_idx, uint32_t* nnpc)
{
	int ret_val = 0;

	*bp_idx = 0;
	*nnpc = (npc + 4);

	uint32_t index = (pc >> 2) % bps->table_size;
	uint32_t pc_tag = bps->pc_tags[index];
	if(pc_tag & 0x1)
	{
		if(pc_tag == (pc | 1))
		{
			uint8_t sat_counter = bps->saturating_counters[index];

			uint8_t rv = drand48() * 4;
			if((rv >= sat_counter) && (sat_counter < BP_MAX_COUNT_VALUE -1 ))
			{
				*nnpc = bps->nnpc_values[index];
				ret_val = 1;
				*bp_idx = index;
			}
		}
	}

	bps->branch_count++;
	return(ret_val);
}

void dumpBranchPredictorStatus(BranchPredictorState* bps)
{
	fprintf(stderr,"Branch-predictor entries.\n");
	uint32_t I;
	for(I = 0; I < bps->table_size; I++)
	{
		if((bps->pc_tags[I] & 0x1) != 0)
		{
			fprintf(stderr, "\t 0x%x 0x%x 0x%x\n",
					(bps->pc_tags[I] & 0xfffffffe),
					bps->nnpc_values[I], bps->saturating_counters[I]);
		}
	}
}

void updateBranchPredictEntry (BranchPredictorState* bps, uint32_t idx, uint8_t br_taken, uint32_t nnpc)
{
	if(br_taken)
	{
		bps->nnpc_values[idx] = nnpc;
		if(bps->saturating_counters[idx] > 0)
			bps->saturating_counters[idx] -= 1;
			
	}
	else 
	{
		if(bps->saturating_counters[idx] < BP_MAX_COUNT_VALUE -1 )
			bps->saturating_counters[idx] += 1;
	}
		
}

void incrementMispredicts(BranchPredictorState* bps)
{
	bps->mispredicts++;
}
uint32_t numberOfMispredicts(BranchPredictorState* bps)
{
	return(bps->mispredicts);
}

uint32_t numberOfBranches(BranchPredictorState* bps)
{
	return(bps->branch_count);
}

//Only one context. We comment this for now
/* 
uint8_t getThreadContext (ThreadState* s)
{
	return(s->mmu_state->MmuContextRegister[s->thread_id]);
}
 */

