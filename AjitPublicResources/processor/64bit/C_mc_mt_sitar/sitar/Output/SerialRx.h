	
	#ifndef SERIALRX_H
	#define SERIALRX_H
	//======================================
	//file SerialRx.h                                                  
	//Describes module SerialRx                                      
	//Auto-generated from input file "memorytop.sitar" on 2026-4-16 at time 12:26:36   
	 //(This design unit is parameterized. Generating code into a .h file only) 
	
	//======================================
	#include<iostream>
	#include<iomanip>

	//sitar core classes
	#include"sitar_module.h"
	#include"sitar_time.h"
	//user included files
	 
//----code block from file memorytop.sitar, line:945 ----
extern "C" {
		#include "bridge_module_helpers.h"
	}
	
//----end code block-------


	namespace sitar{

	template<bool trace=false>
	class SerialRx:public module
	{

		public:
			//constructor
			SerialRx();
	 
			void runBehavior(const time& current_time);
			inline void _incrementPointer(unsigned int ptr_number){_pointer[ptr_number]++; _reexecute=true;}
			void _resetBehavior();

			
			//Auto-generated variables for recording state in behavior block
			private:
			static const unsigned int _num_pointers=14; //total pointers used by this module
			static const unsigned int _num_timers=1;  	//total timers used by this module
			static const unsigned int _num_if_flags=8; //total if_flags used
			
			//State holders 
			//the +1 for array size is to avoid zero-sized arrays
			unsigned int 	_pointer[_num_pointers+1];		//pointers for sequences
			time 	_timer[_num_timers+1];			//timers for wait statements
			bool		_if_flag[_num_if_flags+1];		//if_flags
			unsigned int    _pointer_last_value[_num_pointers+1];	//last value taken by each sequence pointer

	
			//other declarations
			public:
			 
inport<64> request;
outport<32> response;
outport<8> irq_level;
inport<8> control_word;
outport<8> rx_full_status;
inport<8> console_rx_data;
inport<8> console_rx_valid;
outport<8> console_rx_ack;
//----code block from file memorytop.sitar, line:950 ----
uint32_t control_register; uint32_t rx_register; uint32_t data_out; uint32_t addr; uint32_t data_in;
//----end code block-------

//----code block from file memorytop.sitar, line:951 ----
bool done_pull; bool done_push; bool rwbar; bool response_pending; bool data_reg_selected;
//----end code block-------

//----code block from file memorytop.sitar, line:952 ----
uint8_t stage; uint8_t resp_stage; uint8_t byte_mask; uint8_t irq_value; uint8_t reg_offset; uint8_t control_value; uint8_t input_byte; uint8_t input_valid; uint8_t ack_value; uint8_t rx_full_value;
//----end code block-------


	};
	}
	//==========================================


	
	namespace sitar{
	//Constructor
	template<bool trace>
	SerialRx<trace>::SerialRx()
	{
		using std::cout;
		_type="SerialRx";
	
		
		//Initialize variables used by behavior block
		for( int i=0;i<int(_num_pointers);i++) _pointer[i]=0;
		for( int i=0;i<int(_num_timers  );i++)   _timer[i]=0;
		for( int i=0;i<int(_num_if_flags);i++) _if_flag[i]=0;
		_terminated=0;
		_reexecute=0;
	
		 
//---Initializing inport request---
request.setInstanceId("request");
addInport(&request,"request");

//---Initializing outport response---
response.setInstanceId("response");
addOutport(&response,"response");

//---Initializing outport irq_level---
irq_level.setInstanceId("irq_level");
addOutport(&irq_level,"irq_level");

//---Initializing inport control_word---
control_word.setInstanceId("control_word");
addInport(&control_word,"control_word");

//---Initializing outport rx_full_status---
rx_full_status.setInstanceId("rx_full_status");
addOutport(&rx_full_status,"rx_full_status");

//---Initializing inport console_rx_data---
console_rx_data.setInstanceId("console_rx_data");
addInport(&console_rx_data,"console_rx_data");

//---Initializing inport console_rx_valid---
console_rx_valid.setInstanceId("console_rx_valid");
addInport(&console_rx_valid,"console_rx_valid");

//---Initializing outport console_rx_ack---
console_rx_ack.setInstanceId("console_rx_ack");
addOutport(&console_rx_ack,"console_rx_ack");

_pointer_last_value[6]=1;
_pointer_last_value[7]=1;
_pointer_last_value[5]=2;
_pointer_last_value[9]=1;
_pointer_last_value[10]=1;
_pointer_last_value[8]=2;
_pointer_last_value[4]=4;
_pointer_last_value[3]=3;
_pointer_last_value[2]=3;
_pointer_last_value[13]=1;
_pointer_last_value[12]=3;
_pointer_last_value[11]=3;
_pointer_last_value[1]=4;
_pointer_last_value[0]=2;
	}





	

	//runBehavior 
	template<bool trace>
	void  SerialRx<trace>::runBehavior(const time& current_time)
	{
		(void)current_time; //shut up compiler warning if variable is unused
		using std::cout;
		using std::endl;
		
		if(_terminated==1) return;

				
		
			_reexecute=1;
			//any statement that moves the pointers will set the _reexecute flag
			//execute module behavior till convergence:
			for(int _sitar_iteration=1; (_sitar_iteration<=SITAR_ITERATION_LIMIT and _reexecute==1);_sitar_iteration++)
		
		{
			//Execute behavior block till convergence
			_reexecute=0;
			
			//Execute behavior block. Any statement that updates 
			//pointers will set the _reexecute flag
			 
switch(_pointer[0])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:955 ----

	control_register = 0;
	rx_register = 0;
	irq_value = 0;
	stage = 0;
	resp_stage = 0;
	response_pending = false;
	
//----end code block-------

 _incrementPointer(0);
}

case 1 :
{
//do-while statement , line:963
 int _dowhile_iteration;
for(_dowhile_iteration=1; _dowhile_iteration<=SITAR_ITERATION_LIMIT; _dowhile_iteration++)
{
//execute the sequence  
switch(_pointer[1])
{

case 0:
{

//if statement , line:964
if(((((((current_time.phase()))==(0))))))
_if_flag[0]=true;
else
_if_flag[0]=false;
 _incrementPointer(1);
}

case 1:
{

if(_if_flag[0]==true)
{

switch(_pointer[2])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:965 ----
{
				uint8_t next_u8 = 0;
				if(signal_pull_u8(&control_word, &next_u8)) {
					control_value = next_u8;
				}
				if(signal_pull_u8(&console_rx_data, &next_u8)) {
					input_byte = next_u8;
				}
				if(signal_pull_u8(&console_rx_valid, &next_u8)) {
					input_valid = next_u8;
				}
			}
			control_register = setBit32(control_register, 0, getBit32(control_value, 0));
			control_register = setBit32(control_register, 1, getBit32(control_value, 1));
			control_register = setBit32(control_register, 2, getBit32(control_value, 2));
			ack_value = 0;
			if(getBit32(control_register, 1) == 0) {
				rx_register = 0;
				control_register = setBit32(control_register, 4, 0);
				irq_value = 0;
			} else if((input_valid != 0) && (getBit32(control_register, 4) == 0)) {
				rx_register = setSlice32(0, 7, 0, input_byte);
				control_register = setBit32(control_register, 4, 1);
				ack_value = 1;
				if(getBit32(control_register, 2) != 0) {
					irq_value = 12;
				}
			}
			
//----end code block-------

 _incrementPointer(2);
}

case 1:
{

//if statement , line:994
if((((!((response_pending))))))
_if_flag[1]=true;
else
_if_flag[1]=false;
 _incrementPointer(2);
}

case 2:
{

if(_if_flag[1]==true)
{

switch(_pointer[3])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:995 ----
done_pull = peripheral_pull_request_step(&stage,
					&rwbar,
					&byte_mask,
					&addr,
					&data_in,
					&request);
//----end code block-------

 _incrementPointer(3);
}

case 1:
{

//if statement , line:1001
if((((((done_pull))))))
_if_flag[2]=true;
else
_if_flag[2]=false;
 _incrementPointer(3);
}

case 2:
{

if(_if_flag[2]==true)
{

switch(_pointer[4])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:1002 ----
reg_offset = (uint8_t) (addr & 0xffu); data_reg_selected = (reg_offset == 0x20u);
//----end code block-------

 _incrementPointer(4);
}

case 1:
{

//if statement , line:1003
if((((((rwbar))))))
_if_flag[3]=true;
else
_if_flag[3]=false;
 _incrementPointer(4);
}

case 2:
{

if(_if_flag[3]==true)
{

switch(_pointer[5])
{

case 0:
{

//if statement , line:1004
if((((((data_reg_selected))))))
_if_flag[4]=true;
else
_if_flag[4]=false;
 _incrementPointer(5);
}

case 1:
{

if(_if_flag[4]==true)
{

switch(_pointer[6])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:1005 ----
if(byte_mask & 0x8u) {
								data_out = setSlice32(0, 31, 24, getSlice32(rx_register, 7, 0));
							} else if(byte_mask & 0x4u) {
								data_out = setSlice32(0, 23, 16, getSlice32(rx_register, 7, 0));
							} else if(byte_mask & 0x2u) {
								data_out = setSlice32(0, 15, 8, getSlice32(rx_register, 7, 0));
							} else {
								data_out = setSlice32(0, 7, 0, getSlice32(rx_register, 7, 0));
							}
//----end code block-------

 _incrementPointer(6);
}

case 1: break;
}

}

else
{

switch(_pointer[7])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:1015 ----
data_out = control_register;
//----end code block-------

 _incrementPointer(7);
}

case 1: break;
}

}

if((_if_flag[4]==true && _pointer[6]>=_pointer_last_value[6]) || (_if_flag[4]==false&& _pointer[7]>= _pointer_last_value[7]))

{
 //if-statement has terminated
 _incrementPointer(5);
 _pointer[6]=0;

 _pointer[7]=0;

}

 else 
 //if-statement has converged
 break;
}

case 2: break;
}

}

else
{

switch(_pointer[8])
{

case 0:
{

//if statement , line:1018
if((((((data_reg_selected))))))
_if_flag[5]=true;
else
_if_flag[5]=false;
 _incrementPointer(8);
}

case 1:
{

if(_if_flag[5]==true)
{

switch(_pointer[9])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:1019 ----
data_out = rx_register;
//----end code block-------

 _incrementPointer(9);
}

case 1: break;
}

}

else
{

switch(_pointer[10])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:1021 ----
control_register = bridge_insert_using_byte_mask32(control_register, data_in, byte_mask);
							data_out = control_register;
//----end code block-------

 _incrementPointer(10);
}

case 1: break;
}

}

if((_if_flag[5]==true && _pointer[9]>=_pointer_last_value[9]) || (_if_flag[5]==false&& _pointer[10]>= _pointer_last_value[10]))

{
 //if-statement has terminated
 _incrementPointer(8);
 _pointer[9]=0;

 _pointer[10]=0;

}

 else 
 //if-statement has converged
 break;
}

case 2: break;
}

}

if((_if_flag[3]==true && _pointer[5]>=_pointer_last_value[5]) || (_if_flag[3]==false&& _pointer[8]>= _pointer_last_value[8]))

{
 //if-statement has terminated
 _incrementPointer(4);
 _pointer[5]=0;

 _pointer[8]=0;

}

 else 
 //if-statement has converged
 break;
}

case 3:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:1025 ----
response_pending = true; resp_stage = 0; stage = 0;
//----end code block-------

 _incrementPointer(4);
}

case 4: break;
}

}

else
{

}

if((_if_flag[2]==true && _pointer[4]>=_pointer_last_value[4]) || (_if_flag[2]==false))

{
 //if-statement has terminated
 _incrementPointer(3);
 _pointer[4]=0;

}

 else 
 //if-statement has converged
 break;
}

case 3: break;
}

}

else
{

}

if((_if_flag[1]==true && _pointer[3]>=_pointer_last_value[3]) || (_if_flag[1]==false))

{
 //if-statement has terminated
 _incrementPointer(2);
 _pointer[3]=0;

}

 else 
 //if-statement has converged
 break;
}

case 3: break;
}

}

else
{

switch(_pointer[11])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:1029 ----
if(response_pending && (reg_offset == 0x20u) && rwbar) {
				control_register = setBit32(control_register, 4, 0);
				irq_value = 0;
			}
			rx_full_value = getBit32(control_register, 4) ? 1u : 0u;
			signal_push_u8(&irq_level, irq_value);
			signal_push_u8(&rx_full_status, rx_full_value);
			signal_push_u8(&console_rx_ack, ack_value);
//----end code block-------

 _incrementPointer(11);
}

case 1:
{

//if statement , line:1037
if((((((response_pending))))))
_if_flag[6]=true;
else
_if_flag[6]=false;
 _incrementPointer(11);
}

case 2:
{

if(_if_flag[6]==true)
{

switch(_pointer[12])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:1038 ----
done_push = peripheral_send_response_step(&resp_stage, &response, data_out);
//----end code block-------

 _incrementPointer(12);
}

case 1:
{

//if statement , line:1039
if((((((done_push))))))
_if_flag[7]=true;
else
_if_flag[7]=false;
 _incrementPointer(12);
}

case 2:
{

if(_if_flag[7]==true)
{

switch(_pointer[13])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:1040 ----
response_pending = false;
//----end code block-------

 _incrementPointer(13);
}

case 1: break;
}

}

else
{

}

if((_if_flag[7]==true && _pointer[13]>=_pointer_last_value[13]) || (_if_flag[7]==false))

{
 //if-statement has terminated
 _incrementPointer(12);
 _pointer[13]=0;

}

 else 
 //if-statement has converged
 break;
}

case 3: break;
}

}

else
{

}

if((_if_flag[6]==true && _pointer[12]>=_pointer_last_value[12]) || (_if_flag[6]==false))

{
 //if-statement has terminated
 _incrementPointer(11);
 _pointer[12]=0;

}

 else 
 //if-statement has converged
 break;
}

case 3: break;
}

}

if((_if_flag[0]==true && _pointer[2]>=_pointer_last_value[2]) || (_if_flag[0]==false&& _pointer[11]>= _pointer_last_value[11]))

{
 //if-statement has terminated
 _incrementPointer(1);
 _pointer[2]=0;

 _pointer[11]=0;

}

 else 
 //if-statement has converged
 break;
}

case 2:
{

//wait-for -time statement , line:1044
_timer[0] = sitar::time(current_time)+sitar::time(((0)),((1)));
 _incrementPointer(1);
}

case 3:
{
if(current_time>=_timer[0])
 _incrementPointer(1);
else
 break; 
}
case 4: break;
}

if(_pointer[1]< _pointer_last_value[1])  
break; //sequence has converged  
 else 
 if(_pointer[1]==_pointer_last_value[1] && ((((((1)))))==true))
 {
//re-activate the sequence	
_pointer[1]=0;	
_reexecute=1;       	
}				
else break; //sequence has terminated			
 };
	//For loop will finish if									
	//the sequence inside do-while loop converges                                                   
	//OR the expression becomes false at the end of some execution of while loop                    
	//OR  if the iteration limit is exceeded.                                                       
	if (_dowhile_iteration>SITAR_ITERATION_LIMIT)   
	{                                                                                               
		//iteration limit exceeded. Throw error and                                             
		//terminate the do-while statement                                                      
 		std::cerr<<"\nERROR:Iteration limit exceeded for do-while loop on line:963 in file memorytop.sitar";
		_pointer[1]=0;                                                               
		_incrementPointer(0);                                                                    
	}                                                                                              
	else if(_pointer[1]<_pointer_last_value[1])                                    
	{                                                                                               
		//sequence just converged;                                                              
		break;                                                                                  
	}                                                                                               
	else if (_pointer[1]==_pointer_last_value[1] && ((((((1)))))==false))  
	{                                                                                               
		//terminate the do-while statement                                                      
		_pointer[1]=0;                                                               
		_incrementPointer(0);                                                                    
	} ;                                                                                              
};                                                                                                   
case 2: break;
}

		
		}

		
			//if iteration limit has exceeded, throw error and stop simulation 
			if(_reexecute==1)
			{
			std::cerr<<"\nERROR:Iteration limit exceeded in behavior block of module SerialRx.";
			std::cerr<<"\nModule behavior did not converge within a phase .... stopping simulation";
			stop_simulation();
			}

		

		//check if behavior has terminated. 
		if(_pointer[0]==_pointer_last_value[0])
		{
			_terminated=true;
		}
		return;
	}
	
	//resetBehavior 
	template<bool trace>
	void  SerialRx<trace>::_resetBehavior()
	{
		//reset variables used by behavior block
		for( int i=0;i<int(_num_pointers);i++) _pointer[i]=0;
		for( int i=0;i<int(_num_timers  );i++)   _timer[i]=0;
		for( int i=0;i<int(_num_if_flags);i++) _if_flag[i]=0;
		_terminated=0;
		_reexecute=0;
	}


	
	}

			
	#endif
