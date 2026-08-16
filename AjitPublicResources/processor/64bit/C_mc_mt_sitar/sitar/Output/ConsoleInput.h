	
	#ifndef CONSOLEINPUT_H
	#define CONSOLEINPUT_H
	//======================================
	//file ConsoleInput.h                                                  
	//Describes module ConsoleInput                                      
	//Auto-generated from input file "memorytop.sitar" on 2026-7-19 at time 22:6:27   
	 //(This design unit is parameterized. Generating code into a .h file only) 
	
	//======================================
	#include<iostream>
	#include<iomanip>

	//sitar core classes
	#include"sitar_module.h"
	#include"sitar_time.h"
	//user included files
	 
//----code block from file memorytop.sitar, line:1124 ----
extern "C" {
		#include <stdlib.h>
		#include "ajit_memory_shim.h"
		#include "bridge_module_helpers.h"
	}
	
//----end code block-------


	namespace sitar{

	template<bool trace=false>
	class ConsoleInput:public module
	{

		public:
			//constructor
			ConsoleInput();
	 
			void runBehavior(const time& current_time);
			inline void _incrementPointer(unsigned int ptr_number){_pointer[ptr_number]++; _reexecute=true;}
			void _resetBehavior();

			
			//Auto-generated variables for recording state in behavior block
			private:
			static const unsigned int _num_pointers=4; //total pointers used by this module
			static const unsigned int _num_timers=1;  	//total timers used by this module
			static const unsigned int _num_if_flags=1; //total if_flags used
			
			//State holders 
			//the +1 for array size is to avoid zero-sized arrays
			unsigned int 	_pointer[_num_pointers+1];		//pointers for sequences
			time 	_timer[_num_timers+1];			//timers for wait statements
			bool		_if_flag[_num_if_flags+1];		//if_flags
			unsigned int    _pointer_last_value[_num_pointers+1];	//last value taken by each sequence pointer

	
			//other declarations
			public:
			 
outport<8> console_rx_data;
outport<8> console_rx_valid;
inport<8> console_rx_ack;
inport<8> control_word;
//----code block from file memorytop.sitar, line:1131 ----
uint8_t rx_data; uint8_t rx_valid; uint8_t rx_ack; uint8_t control_value; uint8_t input_paced; uint8_t input_prompt_paced; uint8_t first_byte_sent;
//----end code block-------

//----code block from file memorytop.sitar, line:1132 ----
uint64_t output_count; uint64_t prompt_count; uint64_t last_release_output_count; uint64_t last_release_prompt_count; uint64_t last_seen_prompt_count; uint32_t prompt_release_delay; uint32_t prompt_delay_budget;
//----end code block-------


	};
	}
	//==========================================


	
	namespace sitar{
	//Constructor
	template<bool trace>
	ConsoleInput<trace>::ConsoleInput()
	{
		using std::cout;
		_type="ConsoleInput";
	
		
		//Initialize variables used by behavior block
		for( int i=0;i<int(_num_pointers);i++) _pointer[i]=0;
		for( int i=0;i<int(_num_timers  );i++)   _timer[i]=0;
		for( int i=0;i<int(_num_if_flags);i++) _if_flag[i]=0;
		_terminated=0;
		_reexecute=0;
	
		 
//---Initializing outport console_rx_data---
console_rx_data.setInstanceId("console_rx_data");
addOutport(&console_rx_data,"console_rx_data");

//---Initializing outport console_rx_valid---
console_rx_valid.setInstanceId("console_rx_valid");
addOutport(&console_rx_valid,"console_rx_valid");

//---Initializing inport console_rx_ack---
console_rx_ack.setInstanceId("console_rx_ack");
addInport(&console_rx_ack,"console_rx_ack");

//---Initializing inport control_word---
control_word.setInstanceId("control_word");
addInport(&control_word,"control_word");

_pointer_last_value[2]=2;
_pointer_last_value[3]=1;
_pointer_last_value[1]=4;
_pointer_last_value[0]=2;
	}





	

	//runBehavior 
	template<bool trace>
	void  ConsoleInput<trace>::runBehavior(const time& current_time)
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
//----code block from file memorytop.sitar, line:1135 ----

	rx_data = 0;
	rx_valid = 0;
	rx_ack = 0;
	control_value = 0;
	input_paced = 0;
	input_prompt_paced = 0;
	first_byte_sent = 0;
	output_count = 0;
	prompt_count = 0;
	last_release_output_count = 0;
	last_release_prompt_count = 0;
	last_seen_prompt_count = 0;
	prompt_release_delay = 4096;
	prompt_delay_budget = 0;
	
//----end code block-------

 _incrementPointer(0);
}

case 1 :
{
//do-while statement , line:1151
 int _dowhile_iteration;
for(_dowhile_iteration=1; _dowhile_iteration<=SITAR_ITERATION_LIMIT; _dowhile_iteration++)
{
//execute the sequence  
switch(_pointer[1])
{

case 0:
{

//if statement , line:1152
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
//----code block from file memorytop.sitar, line:1153 ----
{
				const char* paced_env = getenv("AJIT_CONSOLE_INPUT_PACED");
				const char* prompt_delay_env = getenv("AJIT_CONSOLE_PROMPT_DELAY");
				output_count = ajit_shim_console_output_count();
				prompt_count = ajit_shim_console_prompt_count();
				input_paced = ((paced_env != NULL) && (paced_env[0] != 0) && (paced_env[0] != '0')) ? 1 : 0;
				input_prompt_paced = ((paced_env != NULL) && (strcmp(paced_env, "prompt") == 0)) ? 1 : 0;
				if(prompt_delay_env != NULL && prompt_delay_env[0] != 0) {
					char* endptr = NULL;
					unsigned long parsed = strtoul(prompt_delay_env, &endptr, 0);
					if(endptr != prompt_delay_env) {
						prompt_release_delay = (uint32_t) parsed;
					}
				}
				if(prompt_count != last_seen_prompt_count) {
					last_seen_prompt_count = prompt_count;
					prompt_delay_budget = input_prompt_paced ? prompt_release_delay : 0;
				} else if(prompt_delay_budget > 0) {
					prompt_delay_budget--;
				}
			}
//----end code block-------

 _incrementPointer(2);
}

case 1:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:1174 ----
rx_ack = 0;
			{
				uint8_t next_u8 = 0;
				if(signal_pull_u8(&console_rx_ack, &next_u8)) {
					rx_ack = next_u8;
				}
				if(signal_pull_u8(&control_word, &next_u8)) {
					control_value = next_u8;
				}
			}
			if((rx_valid != 0) && (rx_ack != 0)) {
				rx_valid = 0;
			}
			if((rx_valid == 0) && ((control_value & 0x2u) != 0)) {
				if((input_paced == 0) ||
				   (input_prompt_paced &&
				    (prompt_count > last_release_prompt_count) &&
				    (prompt_delay_budget == 0)) ||
				   ((input_prompt_paced == 0) &&
				    (((first_byte_sent == 0) && (output_count > 0)) ||
				     ((first_byte_sent != 0) && (output_count > last_release_output_count))))) {
					uint8_t next_byte = 0;
					if(ajit_shim_console_try_read(&next_byte)) {
						rx_data = next_byte;
						rx_valid = 1;
						first_byte_sent = 1;
						last_release_output_count = output_count;
						last_release_prompt_count = prompt_count;
					}
				}
			}
			
//----end code block-------

 _incrementPointer(2);
}

case 2: break;
}

}

else
{

switch(_pointer[3])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:1207 ----
signal_push_u8(&console_rx_data, rx_data);
			signal_push_u8(&console_rx_valid, rx_valid);
//----end code block-------

 _incrementPointer(3);
}

case 1: break;
}

}

if((_if_flag[0]==true && _pointer[2]>=_pointer_last_value[2]) || (_if_flag[0]==false&& _pointer[3]>= _pointer_last_value[3]))

{
 //if-statement has terminated
 _incrementPointer(1);
 _pointer[2]=0;

 _pointer[3]=0;

}

 else 
 //if-statement has converged
 break;
}

case 2:
{

//wait-for -time statement , line:1210
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
 		std::cerr<<"\nERROR:Iteration limit exceeded for do-while loop on line:1151 in file memorytop.sitar";
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
			std::cerr<<"\nERROR:Iteration limit exceeded in behavior block of module ConsoleInput.";
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
	void  ConsoleInput<trace>::_resetBehavior()
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
