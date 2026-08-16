	
	#ifndef CONSOLEOUTPUT_H
	#define CONSOLEOUTPUT_H
	//======================================
	//file ConsoleOutput.h                                                  
	//Describes module ConsoleOutput                                      
	//Auto-generated from input file "memorytop.sitar" on 2026-7-19 at time 22:6:27   
	 //(This design unit is parameterized. Generating code into a .h file only) 
	
	//======================================
	#include<iostream>
	#include<iomanip>

	//sitar core classes
	#include"sitar_module.h"
	#include"sitar_time.h"
	//user included files
	 
//----code block from file memorytop.sitar, line:1082 ----
extern "C" {
		#include "ajit_memory_shim.h"
		#include "bridge_module_helpers.h"
	}
	
//----end code block-------


	namespace sitar{

	template<bool trace=false>
	class ConsoleOutput:public module
	{

		public:
			//constructor
			ConsoleOutput();
	 
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
			 
inport<8> console_tx_data;
inport<8> console_tx_valid;
outport<8> console_tx_ack;
//----code block from file memorytop.sitar, line:1088 ----
uint8_t tx_data; uint8_t tx_valid; uint8_t ack_value; bool saw_valid;
//----end code block-------


	};
	}
	//==========================================


	
	namespace sitar{
	//Constructor
	template<bool trace>
	ConsoleOutput<trace>::ConsoleOutput()
	{
		using std::cout;
		_type="ConsoleOutput";
	
		
		//Initialize variables used by behavior block
		for( int i=0;i<int(_num_pointers);i++) _pointer[i]=0;
		for( int i=0;i<int(_num_timers  );i++)   _timer[i]=0;
		for( int i=0;i<int(_num_if_flags);i++) _if_flag[i]=0;
		_terminated=0;
		_reexecute=0;
	
		 
//---Initializing inport console_tx_data---
console_tx_data.setInstanceId("console_tx_data");
addInport(&console_tx_data,"console_tx_data");

//---Initializing inport console_tx_valid---
console_tx_valid.setInstanceId("console_tx_valid");
addInport(&console_tx_valid,"console_tx_valid");

//---Initializing outport console_tx_ack---
console_tx_ack.setInstanceId("console_tx_ack");
addOutport(&console_tx_ack,"console_tx_ack");

_pointer_last_value[2]=1;
_pointer_last_value[3]=1;
_pointer_last_value[1]=4;
_pointer_last_value[0]=2;
	}





	

	//runBehavior 
	template<bool trace>
	void  ConsoleOutput<trace>::runBehavior(const time& current_time)
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
//----code block from file memorytop.sitar, line:1091 ----

	tx_data = 0;
	tx_valid = 0;
	ack_value = 0;
	saw_valid = false;
	
//----end code block-------

 _incrementPointer(0);
}

case 1 :
{
//do-while statement , line:1097
 int _dowhile_iteration;
for(_dowhile_iteration=1; _dowhile_iteration<=SITAR_ITERATION_LIMIT; _dowhile_iteration++)
{
//execute the sequence  
switch(_pointer[1])
{

case 0:
{

//if statement , line:1098
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
//----code block from file memorytop.sitar, line:1099 ----
signal_pull_u8(&console_tx_data, &tx_data);
			signal_pull_u8(&console_tx_valid, &tx_valid);
			if((tx_valid != 0) && !saw_valid) {
				ajit_shim_console_write(tx_data);
				ack_value = 1;
				saw_valid = true;
			} else if(tx_valid == 0) {
				saw_valid = false;
			}
			
//----end code block-------

 _incrementPointer(2);
}

case 1: break;
}

}

else
{

switch(_pointer[3])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:1110 ----
signal_push_u8(&console_tx_ack, ack_value);
			ack_value = 0;
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

//wait-for -time statement , line:1113
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
 		std::cerr<<"\nERROR:Iteration limit exceeded for do-while loop on line:1097 in file memorytop.sitar";
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
			std::cerr<<"\nERROR:Iteration limit exceeded in behavior block of module ConsoleOutput.";
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
	void  ConsoleOutput<trace>::_resetBehavior()
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
