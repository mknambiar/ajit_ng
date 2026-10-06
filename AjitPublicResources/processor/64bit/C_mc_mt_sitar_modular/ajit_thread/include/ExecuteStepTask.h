#ifndef AJIT_EXECUTE_STEP_TASK_H
#define AJIT_EXECUTE_STEP_TASK_H

#include "../../shim/deep_fetch_step.h"
#include "AjitThread.h"
extern "C" {
#include "Opcodes.h"
}

StepTaskT<int> executeInstruction(CoroutineOwner* owner,
                                  ThreadState* state_ptr,
                                  int trace,
                                  uint8_t i,
                                  uint8_t asi,
                                  uint8_t rd,
                                  uint8_t rs1,
                                  uint8_t vector_data_type,
                                  Opcode opcode,
                                  uint32_t operand2_0,
                                  uint32_t operand2_1,
                                  uint32_t operand1_0,
                                  uint32_t operand1_1,
                                  uint32_t data1,
                                  uint32_t data0,
                                  uint32_t* result_h,
                                  uint32_t* result_l,
                                  uint8_t* flags);

#endif
