	
	#ifndef SCRATCHPAD_H
	#define SCRATCHPAD_H
	//======================================
	//file ScratchPad.h                                                  
	//Describes module ScratchPad                                      
	//Auto-generated from input file "memorytop.sitar" on 2026-7-19 at time 22:6:27   
	 //(This design unit is parameterized. Generating code into a .h file only) 
	
	//======================================
	#include<iostream>
	#include<iomanip>

	//sitar core classes
	#include"sitar_module.h"
	#include"sitar_time.h"
	//user included files
	 
//----code block from file memorytop.sitar, line:622 ----
extern "C" {
		#include "bridge_module_helpers.h"
		#include "Ancillary.h"
	}
	
//----end code block-------


	namespace sitar{

	template<bool trace=false>
	class ScratchPad:public module
	{

		public:
			//constructor
			ScratchPad();
	 
			void runBehavior(const time& current_time);
			inline void _incrementPointer(unsigned int ptr_number){_pointer[ptr_number]++; _reexecute=true;}
			void _resetBehavior();

			
			//Auto-generated variables for recording state in behavior block
			private:
			static const unsigned int _num_pointers=8; //total pointers used by this module
			static const unsigned int _num_timers=2;  	//total timers used by this module
			static const unsigned int _num_if_flags=3; //total if_flags used
			
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
//----code block from file memorytop.sitar, line:628 ----
uint32_t scratch_pad_memory[32]; uint32_t data_out; uint32_t addr; uint32_t data_in;
//----end code block-------

//----code block from file memorytop.sitar, line:629 ----
bool done_pull; bool done_push; bool rwbar;
//----end code block-------

//----code block from file memorytop.sitar, line:630 ----
uint8_t stage; uint8_t resp_stage; uint8_t byte_mask;
//----end code block-------


	};
	}
	//==========================================


	
	namespace sitar{
	//Constructor
	template<bool trace>
	ScratchPad<trace>::ScratchPad()
	{
		using std::cout;
		_type="ScratchPad";
	
		
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

_pointer_last_value[2]=2;
_pointer_last_value[4]=1;
_pointer_last_value[5]=1;
_pointer_last_value[7]=2;
_pointer_last_value[6]=4;
_pointer_last_value[3]=5;
_pointer_last_value[1]=4;
_pointer_last_value[0]=2;
	}





	

	//runBehavior 
	template<bool trace>
	void  ScratchPad<trace>::runBehavior(const time& current_time)
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
//----code block from file memorytop.sitar, line:633 ----

	for (int i = 0; i < 32; ++i) {
		scratch_pad_memory[i] = 0;
	}
	stage = 0;
	
//----end code block-------

 _incrementPointer(0);
}

case 1 :
{
//do-while statement , line:639
 int _dowhile_iteration;
for(_dowhile_iteration=1; _dowhile_iteration<=SITAR_ITERATION_LIMIT; _dowhile_iteration++)
{
//execute the sequence  
switch(_pointer[1])
{

case 0:
{

//wait-until statement , line:640
if(((((((current_time.phase()))==(0))))))
 _incrementPointer(1);
else
 break;
 }
case 1:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:641 ----
done_pull = peripheral_pull_request_step(&stage,
			&rwbar,
			&byte_mask,
			&addr,
			&data_in,
			&request);
//----end code block-------

 _incrementPointer(1);
}

case 2:
{

//if statement , line:647
if((((!((done_pull))))))
_if_flag[0]=true;
else
_if_flag[0]=false;
 _incrementPointer(1);
}

case 3:
{

if(_if_flag[0]==true)
{

switch(_pointer[2])
{

case 0:
{

//wait statement , line:648
_timer[0] = sitar::time(current_time)+sitar::time(0,1);
 _incrementPointer(2);
}

case 1:
{
if(current_time>=_timer[0])
 _incrementPointer(2);
else
 break; 
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

//if statement , line:650
if((((((rwbar))))))
_if_flag[1]=true;
else
_if_flag[1]=false;
 _incrementPointer(3);
}

case 1:
{

if(_if_flag[1]==true)
{

switch(_pointer[4])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:651 ----
data_out = scratch_pad_memory[(addr & 0x7f) >> 2];
//----end code block-------

 _incrementPointer(4);
}

case 1: break;
}

}

else
{

switch(_pointer[5])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:653 ----
scratch_pad_memory[(addr & 0x7f) >> 2] = bridge_insert_using_byte_mask32(scratch_pad_memory[(addr & 0x7f) >> 2], data_in, byte_mask);
				data_out = scratch_pad_memory[(addr & 0x7f) >> 2];
//----end code block-------

 _incrementPointer(5);
}

case 1: break;
}

}

if((_if_flag[1]==true && _pointer[4]>=_pointer_last_value[4]) || (_if_flag[1]==false&& _pointer[5]>= _pointer_last_value[5]))

{
 //if-statement has terminated
 _incrementPointer(3);
 _pointer[4]=0;

 _pointer[5]=0;

}

 else 
 //if-statement has converged
 break;
}

case 2:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:656 ----
resp_stage = 0;
//----end code block-------

 _incrementPointer(3);
}

case 3 :
{
//do-while statement , line:657
 int _dowhile_iteration;
for(_dowhile_iteration=1; _dowhile_iteration<=SITAR_ITERATION_LIMIT; _dowhile_iteration++)
{
//execute the sequence  
switch(_pointer[6])
{

case 0:
{

//wait-until statement , line:658
if(((((((current_time.phase()))==(1))))))
 _incrementPointer(6);
else
 break;
 }
case 1:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:659 ----
done_push = peripheral_send_response_step(&resp_stage, &response, data_out);
//----end code block-------

 _incrementPointer(6);
}

case 2:
{

//if statement , line:660
if((((!((done_push))))))
_if_flag[2]=true;
else
_if_flag[2]=false;
 _incrementPointer(6);
}

case 3:
{

if(_if_flag[2]==true)
{

switch(_pointer[7])
{

case 0:
{

//wait statement , line:660
_timer[1] = sitar::time(current_time)+sitar::time(0,1);
 _incrementPointer(7);
}

case 1:
{
if(current_time>=_timer[1])
 _incrementPointer(7);
else
 break; 
}
case 2: break;
}

}

else
{

}

if((_if_flag[2]==true && _pointer[7]>=_pointer_last_value[7]) || (_if_flag[2]==false))

{
 //if-statement has terminated
 _incrementPointer(6);
 _pointer[7]=0;

}

 else 
 //if-statement has converged
 break;
}

case 4: break;
}

if(_pointer[6]< _pointer_last_value[6])  
break; //sequence has converged  
 else 
 if(_pointer[6]==_pointer_last_value[6] && ((((!((done_push)))))==true))
 {
//re-activate the sequence	
_pointer[6]=0;	
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
 		std::cerr<<"\nERROR:Iteration limit exceeded for do-while loop on line:657 in file memorytop.sitar";
		_pointer[6]=0;                                                               
		_incrementPointer(3);                                                                    
	}                                                                                              
	else if(_pointer[6]<_pointer_last_value[6])                                    
	{                                                                                               
		//sequence just converged;                                                              
		break;                                                                                  
	}                                                                                               
	else if (_pointer[6]==_pointer_last_value[6] && ((((!((done_push)))))==false))  
	{                                                                                               
		//terminate the do-while statement                                                      
		_pointer[6]=0;                                                               
		_incrementPointer(3);                                                                    
	} ;                                                                                              
};                                                                                                   
case 4:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:662 ----
stage = 0;
//----end code block-------

 _incrementPointer(3);
}

case 5: break;
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
 		std::cerr<<"\nERROR:Iteration limit exceeded for do-while loop on line:639 in file memorytop.sitar";
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
			std::cerr<<"\nERROR:Iteration limit exceeded in behavior block of module ScratchPad.";
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
	void  ScratchPad<trace>::_resetBehavior()
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
