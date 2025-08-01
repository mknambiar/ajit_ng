#ifndef __EXECUTE_H__
#define __EXECUTE_H__

#include "RegisterFile.h"

uint8_t isNaN32(float op);
uint8_t isNaN64(double op);
uint8_t check_sNaN32(float op);
uint8_t check_sNaN64(double op);
uint32_t executeInstruction_split_1( 
				ThreadState *s, 
				Opcode opcode, 
				uint32_t operand2_0, uint32_t operand2_1, 
				uint32_t operand1_0, uint32_t operand1_1,
				uint32_t *result_h, uint32_t *result_l, 
				uint8_t *flags, 
				uint8_t rs1, uint8_t rd, uint8_t asi, 
				uint8_t imm_flag,
				uint32_t data1, uint32_t data0, uint8_t vector_data_type,
				dcache_out *dc_out, icache_out *ic_out);
				
uint32_t executeInstruction_split_2( 
				ThreadState *s, 
				Opcode opcode, 
				uint32_t operand2_0, uint32_t operand2_1, 
				uint32_t operand1_0, uint32_t operand1_1,
				uint32_t *result_h, uint32_t *result_l, 
				uint8_t *flags, 
				uint8_t rs1, uint8_t rd, uint8_t asi, 
				uint8_t imm_flag,
				uint32_t data1, uint32_t data0, uint8_t vector_data_type,
				dcache_out *dc_out, icache_out *ic_out);

uint32_t executeFPInstruction(ThreadState* s, Opcode opcode, uint32_t operand2_3, uint32_t operand2_2, uint32_t operand2_1,
		uint32_t operand2_0, uint32_t operand1_3, uint32_t operand1_2, uint32_t operand1_1, uint32_t operand1_0, uint32_t *result_h2,
		uint32_t *result_h1, uint32_t *result_l2, uint32_t *result_l1, uint32_t trap_vector, uint8_t *flags, uint8_t *ft, StatusRegisters *ptr);

void executeTrap(ThreadState* thread_state,
			uint32_t *trap_vector, uint32_t *psr, uint32_t *tbr, 
			uint8_t *interrupt_level, ThreadMode *p_mode,
			uint8_t ticc_trap_type, uint32_t *pc, uint32_t *npc);



uint32_t executeLoad_split_12(Opcode op, uint32_t operand1, uint32_t operand2, 
				uint32_t *result_h, uint32_t *result_l,
				StatusRegisters *status_reg, uint32_t trap_vector, 
				uint8_t asi, uint8_t rd, uint8_t *flags, ThreadState *state, dcache_out dc_out);
uint32_t executeStore_split_12( Opcode op, uint32_t operand1, uint32_t operand2, uint32_t *result_h, uint32_t *result_l, uint32_t data0,	uint32_t data1, StatusRegisters *status_reg, uint32_t trap_vector, uint8_t asi, uint8_t rd, ThreadState *state, dcache_out *dc_out);
uint32_t executeLdstub_split_12(Opcode op, 
				uint32_t operand1, uint32_t operand2, uint32_t *result, 
				StatusRegisters *status_reg, StateUpdateFlags* reg_update_flags,
				uint32_t trap_vector, uint8_t asi, uint8_t *flags,
				ThreadState* state, dcache_out *dc_out);
				
uint32_t executeSwap_split_12( Opcode op, 
			uint32_t operand1, uint32_t operand2, uint32_t *result, 
			StatusRegisters *status_reg, StateUpdateFlags* reg_update_flags, 
			uint32_t trap_vector, uint8_t asi, uint8_t imm_flag,
			uint32_t temp, uint8_t *flags,
			ThreadState* state, dcache_out *dc_out);
			
uint32_t executeCswap_split_12( Opcode op, 
				uint32_t operand1, 
				uint32_t operand2,
				uint32_t *result, 
				uint32_t operand3,
				uint32_t trap_vector, uint8_t asi, uint8_t imm_flag,
				ThreadState* state,
				StatusRegisters *status_reg, 
				StateUpdateFlags*  reg_update_flags,
				uint8_t *flags, dcache_out *dc_out);
				
void executeNop();
void executeLogical( Opcode op, uint32_t operand1, uint32_t operand2, 
			uint32_t *result, StatusRegisters *status_reg, StateUpdateFlags*  reg_update_flags,
			uint8_t *flags);

void executeSethi( uint32_t operand1, uint32_t *result, uint8_t *flags);

void executeShift(Opcode op, uint32_t operand1, uint32_t operand2, uint32_t *result, uint8_t *flags);

void executeAdd( Opcode op, uint32_t operand1, uint32_t operand2, 
			uint32_t *result, StatusRegisters *status_reg, 
			StateUpdateFlags* reg_update_flags, uint8_t *flags);

void executeSub( Opcode op, uint32_t operand1, uint32_t operand2, uint32_t *result, 
				StatusRegisters *status_reg, StateUpdateFlags* reg_update_flags, 
				uint8_t *flags);

void executeMul( Opcode op, uint32_t operand1, uint32_t operand2, uint32_t *result, 
			StatusRegisters *status_reg, StateUpdateFlags* reg_update_flags, uint8_t *flags);

uint32_t executeDiv   (Opcode op, uint32_t operand1, uint32_t operand2, uint32_t *result, 
				StatusRegisters *status_reg, StateUpdateFlags* reg_update_flags,
				uint32_t trap_vector, uint8_t *flags, ThreadState* state);

uint32_t executeSave( uint32_t operand1, uint32_t operand2, uint32_t *result, 
			StatusRegisters *status_reg,StateUpdateFlags* reg_update_flags, 
			uint32_t trap_vector, uint8_t *flags);

uint32_t executeRestore( uint32_t operand1, uint32_t operand2, uint32_t *result, 
				StatusRegisters *status_reg, StateUpdateFlags* reg_update_flags,
				uint32_t trap_vector, uint8_t *flags);

uint32_t executeBicc( Opcode op, uint32_t operand1, 
			StatusRegisters *status_reg, uint32_t trap_vector, uint8_t flags);

uint32_t executeBfpcc(ThreadState* s, Opcode op, uint32_t operand1, 
			StatusRegisters *status_reg, uint32_t trap_vector, uint8_t flags);

uint32_t executeBcpcc(ThreadState* s, Opcode op, uint32_t operand1, 
			StatusRegisters *status_reg, uint32_t trap_vector, uint8_t flags);

void executeCall( RegisterFile* rf,
			uint32_t operand1, StatusRegisters *status_reg, StateUpdateFlags* suf);

uint32_t executeJumpAndLink( Opcode op, uint32_t operand1, uint32_t operand2, 
			uint32_t *result, 
			StatusRegisters *status_reg, uint32_t trap_vector, uint8_t *flags);

uint32_t executeRett( Opcode op, uint32_t operand1, uint32_t operand2, uint32_t *result, 
		StatusRegisters *status_reg, StateUpdateFlags* reg_update_flags,
			uint32_t trap_vector, ThreadMode* p_mode);

uint32_t executeTicc( Opcode op, uint32_t operand1, uint32_t operand2, StatusRegisters *status_reg,
				StateUpdateFlags* reg_update_flags, uint32_t trap_vector,
				uint8_t *ticc_trap_type);
uint32_t executeReadStateReg( Opcode op, uint8_t rs1, uint32_t *result, 
					ThreadState* s, 
					uint32_t trap_vector, uint8_t *flags);
uint32_t executeWriteStateReg( Opcode op, uint32_t operand1, uint32_t operand2, uint8_t rd, StatusRegisters *status_reg, 
					StateUpdateFlags* reg_update_flags, uint32_t trap_vector);
void executeStbar_split_12( uint8_t *store_barrier_pending, StateUpdateFlags* reg_update_flags, ThreadState* s, dcache_out *dc);
uint32_t executeUnImplemented( uint32_t trap_vector);

uint32_t executeFlush_split_12(uint32_t flush_addr, uint32_t trap_vector, StateUpdateFlags* reg_update_flags,
				ThreadState* state, dcache_out *dc_out, icache_out *ic_out );


////////////////////////////////////////////////////////////////////////////////////
// 64-bit ISA extension related executes.
////////////////////////////////////////////////////////////////////////////////////
void execute64BitLogical( Opcode op, 
			uint32_t operand1_0, uint32_t operand1_1, 
			uint32_t operand2_0, uint32_t operand2_1,
			uint32_t *result_h, uint32_t* result_l,  
			StatusRegisters *status_reg, 
			StateUpdateFlags*  reg_update_flags,
			uint8_t *flags);
void execute64BitShift(Opcode op, uint32_t operand1_0, uint32_t operand1_1, uint32_t operand2,
					uint32_t *result_h, uint32_t* result_l, 
					StatusRegisters *status_reg, StateUpdateFlags* reg_update_flags,
					uint8_t *flags);
void execute64BitAdd  (Opcode op, uint32_t operand1_0, uint32_t operand1_1, 
				uint32_t operand2_0, uint32_t operand2_1,
				uint32_t *result_h, uint32_t* result_l, 
				StatusRegisters *status_reg, StateUpdateFlags *reg_update_flags,
				uint8_t *flags);
void execute64BitSub  (Opcode op, uint32_t operand1_0, uint32_t operand1_1, 
				uint32_t operand2_0, uint32_t operand2_1,
				uint32_t *result_h, uint32_t* result_l, 
				StatusRegisters *status_reg, StateUpdateFlags *reg_update_flags,
				uint8_t *flags);
void execute64BitMul  (Opcode op, uint32_t operand1_0, uint32_t operand1_1, 
					uint32_t operand2_0, uint32_t operand2_1,
					uint32_t *result_h, uint32_t* result_l, 
					StatusRegisters *status_reg, StateUpdateFlags *reg_update_flags,
					uint8_t *flags);
uint32_t execute64BitDiv   (Opcode op, uint32_t operand1_0, uint32_t operand1_1, 
				uint32_t operand2_0, uint32_t operand2_1, 
				uint32_t *result_h,  uint32_t *result_l,
				StatusRegisters *status_reg, StateUpdateFlags* reg_update_flags,
				uint32_t trap_vector, uint8_t *flags,
				ThreadState* state);
void execute64BitReduce8  (Opcode op, 
					uint32_t operand1_0, uint32_t operand1_1, 
					uint32_t operand2, 
					uint32_t* result_l, 
					StatusRegisters *status_reg, StateUpdateFlags* reg_update_flags,
					uint8_t *flags);
void execute64BitReduce16  (Opcode op, 
					uint32_t operand1_0, uint32_t operand1_1, 
					uint32_t operand2, 
					uint32_t* result_l, 
					StatusRegisters *status_reg, StateUpdateFlags* reg_update_flags,
					uint8_t *flags);

void execute64BitZBytePos  (Opcode op, uint32_t operand1_0, uint32_t operand1_1, 
					uint32_t operand2,
					uint32_t* result_l, 
					StatusRegisters *status_reg, StateUpdateFlags* reg_update_flags,
					uint8_t *flags);

// Vector ops.
void execute64BitVectorOp  (Opcode op, 
				uint8_t  data_type,
				uint32_t operand1_0, uint32_t operand1_1, 
				uint32_t operand2_0, uint32_t operand2_1,
				uint32_t *result_h, uint32_t* result_l, 
				StatusRegisters *status_reg, StateUpdateFlags* reg_update_flags,
				uint8_t *flags);


//
// Vector float ops..
void execute64FloatVf (Opcode op, uint32_t operand1_1, uint32_t operand1_0, 
		uint32_t operand2_1, uint32_t operand2_0,
		uint32_t *result_h,  uint32_t *result_l, 
		uint8_t* ft,
		uint8_t *flags);

void execute64FpHalfAddReduce(Opcode op, uint32_t operand2_1, uint32_t operand2_0,
		uint32_t* result_l, uint8_t* ft, uint8_t* flags);

void execute64FpHalfConvert(Opcode op, uint32_t operand2_0,
		uint32_t* result_l, uint8_t* ft, uint8_t* flags);
//
#endif /* EXECUTE_H_ */
