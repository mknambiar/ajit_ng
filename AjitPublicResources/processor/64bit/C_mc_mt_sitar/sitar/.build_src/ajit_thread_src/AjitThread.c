//-------------------------------------------------------------------------
// Describes the ajit thread 
//-------------------------------------------------------------------------
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>

#include "AjitThread.h"
#include "Core.h"
#include "Opcodes.h"
#include "Traps.h"
#include "Flags.h"
#include "Ancillary.h"
#include "Ajit_Hardware_Configuration.h"
#include "RegisterFile.h"
#include "ThreadInterface.h"
#include "Decode.h"
#include "Execute.h"
#include "monitorLogger.h"
#include "Pipes.h"
#include "StatsKeeping.h"
#include "ThreadLogging.h"
#include "ThreadHWserverInterface.h"
#include "Mmu.h"
#include "rlut.h"
#include "ImplementationDependent.h"
#include "AjitThreadUtils.h"

extern int global_verbose_flag;
extern int use_instruction_buffer;

extern int   global_enable_statistic_collection;
extern int   global_stat_collection_trigger_pc;

void increment_instruction_count(ThreadState* s) 
{ 
	s->num_instructions_executed++; 
};

uint64_t get_instruction_count(ThreadState* s) 
{ 
	return s->num_instructions_executed; 
};

uint64_t getCycleEstimate (ThreadState* s)
{
	uint64_t ret_val = s->num_instructions_executed;
	ret_val += (s->mmu_state->Num_Mmu_translated_accesses - s->mmu_state->Num_Mmu_TLB_hits)*
				TLB_MISS_PENALTY;
	ret_val += s->dcache->number_of_misses * CACHE_MISS_PENALTY;
	ret_val += s->icache->number_of_misses * CACHE_MISS_PENALTY;
	
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

void 	clearStateUpdateFlags( ThreadState *s);
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

void init_ajit_thread (ThreadState *s, uint32_t core_id,
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
	s->hw_server	  =  NULL;
	s->monitor_logger =  NULL;

	sprintf(s->mode_pipe_name,"AJIT_to_ENV_processor_mode_%d_%d", core_id, thread_id);
	sprintf(s->error_pipe_name,"AJIT_to_ENV_error_%d_%d", core_id, thread_id);
	sprintf(s->reset_pipe_name,"ENV_to_AJIT_reset_%d_%d", core_id, thread_id);
	sprintf(s->ilvl_pipe_name, "ENV_to_THREAD_irl_%d_%d", core_id, thread_id);
	sprintf(s->logger_pipe_name, "AJIT_to_ENV_logger_%d_%d", core_id, thread_id);

#ifndef SITAR_PIPELESS
	register_port(s->mode_pipe_name, 8, 0);
	register_pipe(s->error_pipe_name, 2, 8, PIPE_FIFO_MODE);
	register_port(s->reset_pipe_name, 8, 1);
	register_port(s->ilvl_pipe_name, 8, 1);
	register_pipe(s->logger_pipe_name, 3, 32, PIPE_FIFO_MODE);
#endif

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
	s->ifetch_pending = 0;
	s->ifetch_addr = 0;
	s->sitar_sim_time = 0;

}

void setThreadSitarSimTime(ThreadState* s, uint64_t sim_time)
{
	s->sitar_sim_time = sim_time;
}

void resetThreadState(ThreadState *s)
{
	StatusRegisters *status_reg = &(s->status_reg);

	s->instruction = 0;		
	s->addr_space = 0;		
	s->last_data_asi = 0;
	s->last_data_asi_valid = 0;
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
	s->ifetch_pending = 0;
	s->ifetch_addr = 0;



}



uint8_t isBranchInstruction(Opcode op, InstructionType type)
{
	return ((op == _CALL_) || (op == _RETT_) || (op == _JMPL_) || (type == _CC_INS_) || (type == _TICC_INS_)); 
}

uint8_t isFloatCompareInstruction(Opcode op)
{
	return ((op == _FCMPs_) || (op == _FCMPd_) || (op == _FCMPq_) || (op == _FCMPEs_) || (op == _FCMPEd_) || (op == _FCMPEq_));
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

	int I;
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
	int I;
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


uint8_t getThreadContext (ThreadState* s)
{
	return(s->mmu_state->MmuContextRegister[s->thread_id]);
}
