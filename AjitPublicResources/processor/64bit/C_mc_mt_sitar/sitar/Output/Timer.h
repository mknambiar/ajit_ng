	
	#ifndef TIMER_H
	#define TIMER_H
	//======================================
	//file Timer.h                                                  
	//Describes module Timer                                      
	//Auto-generated from input file "memorytop.sitar" on 2026-7-19 at time 22:6:27   
	 //(This design unit is parameterized. Generating code into a .h file only) 
	
	//======================================
	#include<iostream>
	#include<iomanip>

	//sitar core classes
	#include"sitar_module.h"
	#include"sitar_time.h"
	//user included files
	 
//----code block from file memorytop.sitar, line:673 ----
extern "C" {
		#include "bridge_module_helpers.h"
	}
	#include <cstdlib>
	#include <cstdio>
	
//----end code block-------


	namespace sitar{

	template<bool trace=false>
	class Timer:public module
	{

		public:
			//constructor
			Timer();
	 
			void runBehavior(const time& current_time);
			inline void _incrementPointer(unsigned int ptr_number){_pointer[ptr_number]++; _reexecute=true;}
			void _resetBehavior();

			
			//Auto-generated variables for recording state in behavior block
			private:
			static const unsigned int _num_pointers=11; //total pointers used by this module
			static const unsigned int _num_timers=1;  	//total timers used by this module
			static const unsigned int _num_if_flags=7; //total if_flags used
			
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
//----code block from file memorytop.sitar, line:680 ----
uint32_t control_register; uint32_t timer_count; uint32_t timer_max_count; uint32_t data_out; uint32_t addr; uint32_t data_in; uint32_t next_value;
//----end code block-------

//----code block from file memorytop.sitar, line:681 ----
uint32_t timer_tick_div; uint32_t timer_tick_phase;
//----end code block-------

//----code block from file memorytop.sitar, line:682 ----
bool done_pull; bool done_push; bool rwbar; bool response_pending;
//----end code block-------

//----code block from file memorytop.sitar, line:683 ----
uint8_t stage; uint8_t resp_stage; uint8_t byte_mask; uint8_t irq_value; uint8_t timer_state;
//----end code block-------


	};
	}
	//==========================================


	
	namespace sitar{
	//Constructor
	template<bool trace>
	Timer<trace>::Timer()
	{
		using std::cout;
		_type="Timer";
	
		
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

_pointer_last_value[3]=1;
_pointer_last_value[6]=1;
_pointer_last_value[7]=1;
_pointer_last_value[5]=3;
_pointer_last_value[4]=3;
_pointer_last_value[2]=4;
_pointer_last_value[10]=1;
_pointer_last_value[9]=3;
_pointer_last_value[8]=3;
_pointer_last_value[1]=4;
_pointer_last_value[0]=2;
	}





	

	//runBehavior 
	template<bool trace>
	void  Timer<trace>::runBehavior(const time& current_time)
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
//----code block from file memorytop.sitar, line:685 ----

	control_register = 0;
	timer_count = 0;
	timer_max_count = 0;
	timer_tick_div = 10000000u;
	timer_tick_phase = 0;
	irq_value = 0;
	timer_state = 0;
	stage = 0;
	resp_stage = 0;
	response_pending = false;
	if(const char* tick_div_env = std::getenv("AJIT_TIMER_TICK_DIV")) {
		unsigned long parsed = std::strtoul(tick_div_env, NULL, 0);
		if(parsed != 0ul) {
			timer_tick_div = (uint32_t) parsed;
		}
	}
	
//----end code block-------

 _incrementPointer(0);
}

case 1 :
{
//do-while statement , line:703
 int _dowhile_iteration;
for(_dowhile_iteration=1; _dowhile_iteration<=SITAR_ITERATION_LIMIT; _dowhile_iteration++)
{
//execute the sequence  
switch(_pointer[1])
{

case 0:
{

//if statement , line:704
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

//if statement , line:705
if((((((timer_state)==(1))))))
_if_flag[1]=true;
else
_if_flag[1]=false;
 _incrementPointer(2);
}

case 1:
{

if(_if_flag[1]==true)
{

switch(_pointer[3])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:706 ----
timer_tick_phase = timer_tick_phase + 1;
				if(timer_tick_phase >= timer_tick_div) {
					timer_tick_phase = 0;
					timer_count = timer_count + 1;
					if((timer_max_count != 0) && (timer_count >= timer_max_count)) {
						timer_state = 2;
						irq_value = 10;
					}
				}
				
//----end code block-------

 _incrementPointer(3);
}

case 1: break;
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

case 2:
{

//if statement , line:717
if((((!((response_pending))))))
_if_flag[2]=true;
else
_if_flag[2]=false;
 _incrementPointer(2);
}

case 3:
{

if(_if_flag[2]==true)
{

switch(_pointer[4])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:718 ----
done_pull = peripheral_pull_request_step(&stage,
					&rwbar,
					&byte_mask,
					&addr,
					&data_in,
					&request);
//----end code block-------

 _incrementPointer(4);
}

case 1:
{

//if statement , line:724
if((((((done_pull))))))
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

//if statement , line:725
if((((((rwbar))))))
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
//----code block from file memorytop.sitar, line:726 ----
data_out = control_register;
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
//----code block from file memorytop.sitar, line:728 ----
next_value = bridge_insert_using_byte_mask32(control_register, data_in, byte_mask);
						control_register = next_value;
						if((control_register & 0x1) == 0) {
							timer_state = 0;
							irq_value = 0;
							timer_count = 0;
							timer_max_count = 0;
							timer_tick_phase = 0;
						} else if(timer_state == 0) {
							timer_state = 1;
							irq_value = 0;
							timer_count = 0;
							timer_max_count = (control_register >> 1);
							timer_tick_phase = 0;
						} else if(timer_state == 1) {
							timer_max_count = (control_register >> 1);
						}
						data_out = 0;
					
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

case 2:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:748 ----
response_pending = true; resp_stage = 0; stage = 0;
//----end code block-------

 _incrementPointer(5);
}

case 3: break;
}

}

else
{

}

if((_if_flag[3]==true && _pointer[5]>=_pointer_last_value[5]) || (_if_flag[3]==false))

{
 //if-statement has terminated
 _incrementPointer(4);
 _pointer[5]=0;

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

if((_if_flag[2]==true && _pointer[4]>=_pointer_last_value[4]) || (_if_flag[2]==false))

{
 //if-statement has terminated
 _incrementPointer(2);
 _pointer[4]=0;

}

 else 
 //if-statement has converged
 break;
}

case 4: break;
}

}

else
{

switch(_pointer[8])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:752 ----
done_push = signal_push_u8(&irq_level, irq_value);
//----end code block-------

 _incrementPointer(8);
}

case 1:
{

//if statement , line:753
if((((((response_pending))))))
_if_flag[5]=true;
else
_if_flag[5]=false;
 _incrementPointer(8);
}

case 2:
{

if(_if_flag[5]==true)
{

switch(_pointer[9])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:754 ----
done_push = peripheral_send_response_step(&resp_stage, &response, data_out);
//----end code block-------

 _incrementPointer(9);
}

case 1:
{

//if statement , line:755
if((((((done_push))))))
_if_flag[6]=true;
else
_if_flag[6]=false;
 _incrementPointer(9);
}

case 2:
{

if(_if_flag[6]==true)
{

switch(_pointer[10])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:756 ----
response_pending = false;
//----end code block-------

 _incrementPointer(10);
}

case 1: break;
}

}

else
{

}

if((_if_flag[6]==true && _pointer[10]>=_pointer_last_value[10]) || (_if_flag[6]==false))

{
 //if-statement has terminated
 _incrementPointer(9);
 _pointer[10]=0;

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

if((_if_flag[5]==true && _pointer[9]>=_pointer_last_value[9]) || (_if_flag[5]==false))

{
 //if-statement has terminated
 _incrementPointer(8);
 _pointer[9]=0;

}

 else 
 //if-statement has converged
 break;
}

case 3: break;
}

}

if((_if_flag[0]==true && _pointer[2]>=_pointer_last_value[2]) || (_if_flag[0]==false&& _pointer[8]>= _pointer_last_value[8]))

{
 //if-statement has terminated
 _incrementPointer(1);
 _pointer[2]=0;

 _pointer[8]=0;

}

 else 
 //if-statement has converged
 break;
}

case 2:
{

//wait-for -time statement , line:760
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
 		std::cerr<<"\nERROR:Iteration limit exceeded for do-while loop on line:703 in file memorytop.sitar";
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
			std::cerr<<"\nERROR:Iteration limit exceeded in behavior block of module Timer.";
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
	void  Timer<trace>::_resetBehavior()
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
