	
	#ifndef INTERRUPTCONTROLLER_H
	#define INTERRUPTCONTROLLER_H
	//======================================
	//file InterruptController.h                                                  
	//Describes module InterruptController                                      
	//Auto-generated from input file "memorytop.sitar" on 2026-7-19 at time 22:6:27   
	 //(This design unit is parameterized. Generating code into a .h file only) 
	
	//======================================
	#include<iostream>
	#include<iomanip>

	//sitar core classes
	#include"sitar_module.h"
	#include"sitar_time.h"
	//user included files
	 
//----code block from file memorytop.sitar, line:773 ----
extern "C" {
		#include "bridge_module_helpers.h"
	}
	
//----end code block-------


	namespace sitar{

	template<int numT=4,bool trace=false>
	class InterruptController:public module
	{

		public:
			//constructor
			InterruptController();
	 
			void runBehavior(const time& current_time);
			inline void _incrementPointer(unsigned int ptr_number){_pointer[ptr_number]++; _reexecute=true;}
			void _resetBehavior();

			
			//Auto-generated variables for recording state in behavior block
			private:
			static const unsigned int _num_pointers=10; //total pointers used by this module
			static const unsigned int _num_timers=1;  	//total timers used by this module
			static const unsigned int _num_if_flags=6; //total if_flags used
			
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
inport<8> timer_irq_level;
inport<8> serial_rx_irq_level;
outport<8> thread_irl[numT];
//----code block from file memorytop.sitar, line:778 ----
uint32_t control_register[numT]; uint32_t data_out; uint32_t addr; uint32_t data_in;
//----end code block-------

//----code block from file memorytop.sitar, line:779 ----
bool done_pull; bool done_push; bool rwbar; bool response_pending;
//----end code block-------

//----code block from file memorytop.sitar, line:780 ----
uint8_t stage; uint8_t resp_stage; uint8_t byte_mask; uint8_t timer_irq; uint8_t serial_irq; uint8_t thread_level[numT]; uint32_t reg_index; uint16_t irq_enable_mask;
//----end code block-------


	};
	}
	//==========================================


	
	namespace sitar{
	//Constructor
	template<int numT,bool trace>
	InterruptController<numT,trace>::InterruptController()
	{
		using std::cout;
		_type="InterruptController";
	
		
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

//---Initializing inport timer_irq_level---
timer_irq_level.setInstanceId("timer_irq_level");
addInport(&timer_irq_level,"timer_irq_level");

//---Initializing inport serial_rx_irq_level---
serial_rx_irq_level.setInstanceId("serial_rx_irq_level");
addInport(&serial_rx_irq_level,"serial_rx_irq_level");

//----Initializing outport-array thread_irl------
for(int i=0;i<(numT);i++)
{

std::string pname="thread_irl["+sitar::toString(i)+"]";
thread_irl[i].setInstanceId(pname);
addOutport(&thread_irl[i],"thread_irl["+sitar::toString(i)+"]");

}

_pointer_last_value[5]=1;
_pointer_last_value[6]=1;
_pointer_last_value[4]=4;
_pointer_last_value[3]=3;
_pointer_last_value[2]=3;
_pointer_last_value[9]=1;
_pointer_last_value[8]=3;
_pointer_last_value[7]=3;
_pointer_last_value[1]=4;
_pointer_last_value[0]=2;
	}





	

	//runBehavior 
	template<int numT,bool trace>
	void  InterruptController<numT,trace>::runBehavior(const time& current_time)
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
//----code block from file memorytop.sitar, line:782 ----

	for (int i = 0; i < numT; ++i) {
		control_register[i] = 0;
		thread_level[i] = 0;
	}
	stage = 0;
	resp_stage = 0;
	response_pending = false;
	timer_irq = 0;
	serial_irq = 0;
	
//----end code block-------

 _incrementPointer(0);
}

case 1 :
{
//do-while statement , line:793
 int _dowhile_iteration;
for(_dowhile_iteration=1; _dowhile_iteration<=SITAR_ITERATION_LIMIT; _dowhile_iteration++)
{
//execute the sequence  
switch(_pointer[1])
{

case 0:
{

//if statement , line:794
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
//----code block from file memorytop.sitar, line:795 ----
signal_pull_u8(&timer_irq_level, &timer_irq);
			signal_pull_u8(&serial_rx_irq_level, &serial_irq);
			for (int i = 0; i < numT; ++i) {
				thread_level[i] = 0;
				if(control_register[i] & 0x1) {
					irq_enable_mask = (uint16_t) ((control_register[i] >> 1) & 0x7fffu);
					if(irq_enable_mask == 0) {
						// Legacy single-thread tests only write bit 0 to enable the IRC.
						// Treat that as "all sources enabled" for compatibility.
						irq_enable_mask = 0x7fffu;
					}
					if((timer_irq != 0) && ((irq_enable_mask >> (10 - 1)) & 0x1)) {
						thread_level[i] = timer_irq;
					} else if((serial_irq != 0) && ((irq_enable_mask >> (12 - 1)) & 0x1)) {
						thread_level[i] = serial_irq;
					}
				}
			}
			
//----end code block-------

 _incrementPointer(2);
}

case 1:
{

//if statement , line:814
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
//----code block from file memorytop.sitar, line:815 ----
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

//if statement , line:821
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
//----code block from file memorytop.sitar, line:822 ----
reg_index = ((addr - 0xFFFF3000) >> 2);
					if(reg_index >= (uint32_t) numT) reg_index = 0;
//----end code block-------

 _incrementPointer(4);
}

case 1:
{

//if statement , line:824
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
 
//code_block_statement 
//----code block from file memorytop.sitar, line:825 ----
data_out = control_register[reg_index];
//----end code block-------

 _incrementPointer(5);
}

case 1: break;
}

}

else
{

switch(_pointer[6])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:827 ----
control_register[reg_index] = bridge_insert_using_byte_mask32(control_register[reg_index], data_in, byte_mask);
						data_out = control_register[reg_index];
//----end code block-------

 _incrementPointer(6);
}

case 1: break;
}

}

if((_if_flag[3]==true && _pointer[5]>=_pointer_last_value[5]) || (_if_flag[3]==false&& _pointer[6]>= _pointer_last_value[6]))

{
 //if-statement has terminated
 _incrementPointer(4);
 _pointer[5]=0;

 _pointer[6]=0;

}

 else 
 //if-statement has converged
 break;
}

case 3:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:830 ----
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

switch(_pointer[7])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:834 ----
for (int i = 0; i < numT; ++i) signal_push_u8(&thread_irl[i], thread_level[i]);
//----end code block-------

 _incrementPointer(7);
}

case 1:
{

//if statement , line:835
if((((((response_pending))))))
_if_flag[4]=true;
else
_if_flag[4]=false;
 _incrementPointer(7);
}

case 2:
{

if(_if_flag[4]==true)
{

switch(_pointer[8])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:836 ----
done_push = peripheral_send_response_step(&resp_stage, &response, data_out);
//----end code block-------

 _incrementPointer(8);
}

case 1:
{

//if statement , line:837
if((((((done_push))))))
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
//----code block from file memorytop.sitar, line:838 ----
response_pending = false;
//----end code block-------

 _incrementPointer(9);
}

case 1: break;
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

else
{

}

if((_if_flag[4]==true && _pointer[8]>=_pointer_last_value[8]) || (_if_flag[4]==false))

{
 //if-statement has terminated
 _incrementPointer(7);
 _pointer[8]=0;

}

 else 
 //if-statement has converged
 break;
}

case 3: break;
}

}

if((_if_flag[0]==true && _pointer[2]>=_pointer_last_value[2]) || (_if_flag[0]==false&& _pointer[7]>= _pointer_last_value[7]))

{
 //if-statement has terminated
 _incrementPointer(1);
 _pointer[2]=0;

 _pointer[7]=0;

}

 else 
 //if-statement has converged
 break;
}

case 2:
{

//wait-for -time statement , line:842
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
 		std::cerr<<"\nERROR:Iteration limit exceeded for do-while loop on line:793 in file memorytop.sitar";
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
			std::cerr<<"\nERROR:Iteration limit exceeded in behavior block of module InterruptController.";
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
	template<int numT,bool trace>
	void  InterruptController<numT,trace>::_resetBehavior()
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
