	
	#ifndef L2CACHE_H
	#define L2CACHE_H
	//======================================
	//file L2Cache.h                                                  
	//Describes module L2Cache                                      
	//Auto-generated from input file "memorytop.sitar" on 2026-4-16 at time 12:26:36   
	 //(This design unit is parameterized. Generating code into a .h file only) 
	
	//======================================
	#include<iostream>
	#include<iomanip>

	//sitar core classes
	#include"sitar_module.h"
	#include"sitar_time.h"
	//user included files
	 
//----code block from file memorytop.sitar, line:442 ----
extern "C" {
		#include "bridge_module_helpers.h"
	}
	
//----end code block-------


	namespace sitar{

	template<bool trace=false>
	class L2Cache:public module
	{

		public:
			//constructor
			L2Cache();
	 
			void runBehavior(const time& current_time);
			inline void _incrementPointer(unsigned int ptr_number){_pointer[ptr_number]++; _reexecute=true;}
			void _resetBehavior();

			
			//Auto-generated variables for recording state in behavior block
			private:
			static const unsigned int _num_pointers=10; //total pointers used by this module
			static const unsigned int _num_timers=3;  	//total timers used by this module
			static const unsigned int _num_if_flags=4; //total if_flags used
			
			//State holders 
			//the +1 for array size is to avoid zero-sized arrays
			unsigned int 	_pointer[_num_pointers+1];		//pointers for sequences
			time 	_timer[_num_timers+1];			//timers for wait statements
			bool		_if_flag[_num_if_flags+1];		//if_flags
			unsigned int    _pointer_last_value[_num_pointers+1];	//last value taken by each sequence pointer

	
			//other declarations
			public:
			 
inport<1> active;
inport<1> write;
inport<32> addr_in;
inport<64> data_in;
inport<8> byte_mask;
outport<64> data_out;
outport<1> mem_active;
outport<1> mem_write;
outport<32> mem_addr_out;
outport<64> mem_data_out;
outport<8> mem_byte_mask;
inport<64> mem_data_in;
//----code block from file memorytop.sitar, line:447 ----
uint64_t read_data; bool done_pull; bool done_push; bool write_val;
//----end code block-------

//----code block from file memorytop.sitar, line:448 ----
uint32_t mem_addr;
//----end code block-------

//----code block from file memorytop.sitar, line:449 ----
uint64_t data; uint8_t req_stage; uint8_t l2_stage; uint8_t resp_stage;
//----end code block-------

//----code block from file memorytop.sitar, line:450 ----
uint8_t bmask;
//----end code block-------


	};
	}
	//==========================================


	
	namespace sitar{
	//Constructor
	template<bool trace>
	L2Cache<trace>::L2Cache()
	{
		using std::cout;
		_type="L2Cache";
	
		
		//Initialize variables used by behavior block
		for( int i=0;i<int(_num_pointers);i++) _pointer[i]=0;
		for( int i=0;i<int(_num_timers  );i++)   _timer[i]=0;
		for( int i=0;i<int(_num_if_flags);i++) _if_flag[i]=0;
		_terminated=0;
		_reexecute=0;
	
		 
//---Initializing inport active---
active.setInstanceId("active");
addInport(&active,"active");

//---Initializing inport write---
write.setInstanceId("write");
addInport(&write,"write");

//---Initializing inport addr_in---
addr_in.setInstanceId("addr_in");
addInport(&addr_in,"addr_in");

//---Initializing inport data_in---
data_in.setInstanceId("data_in");
addInport(&data_in,"data_in");

//---Initializing inport byte_mask---
byte_mask.setInstanceId("byte_mask");
addInport(&byte_mask,"byte_mask");

//---Initializing outport data_out---
data_out.setInstanceId("data_out");
addOutport(&data_out,"data_out");

//---Initializing outport mem_active---
mem_active.setInstanceId("mem_active");
addOutport(&mem_active,"mem_active");

//---Initializing outport mem_write---
mem_write.setInstanceId("mem_write");
addOutport(&mem_write,"mem_write");

//---Initializing outport mem_addr_out---
mem_addr_out.setInstanceId("mem_addr_out");
addOutport(&mem_addr_out,"mem_addr_out");

//---Initializing outport mem_data_out---
mem_data_out.setInstanceId("mem_data_out");
addOutport(&mem_data_out,"mem_data_out");

//---Initializing outport mem_byte_mask---
mem_byte_mask.setInstanceId("mem_byte_mask");
addOutport(&mem_byte_mask,"mem_byte_mask");

//---Initializing inport mem_data_in---
mem_data_in.setInstanceId("mem_data_in");
addInport(&mem_data_in,"mem_data_in");

_pointer_last_value[2]=2;
_pointer_last_value[5]=1;
_pointer_last_value[6]=1;
_pointer_last_value[7]=2;
_pointer_last_value[4]=4;
_pointer_last_value[9]=2;
_pointer_last_value[8]=4;
_pointer_last_value[3]=5;
_pointer_last_value[1]=4;
_pointer_last_value[0]=2;
	}





	

	//runBehavior 
	template<bool trace>
	void  L2Cache<trace>::runBehavior(const time& current_time)
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
//----code block from file memorytop.sitar, line:453 ----

	req_stage = 0;
	l2_stage = 0;
	bmask = 0xff;
	
//----end code block-------

 _incrementPointer(0);
}

case 1 :
{
//do-while statement , line:458
 int _dowhile_iteration;
for(_dowhile_iteration=1; _dowhile_iteration<=SITAR_ITERATION_LIMIT; _dowhile_iteration++)
{
//execute the sequence  
switch(_pointer[1])
{

case 0:
{

//wait-until statement , line:459
if(((((((current_time.phase()))==(0))))))
 _incrementPointer(1);
else
 break;
 }
case 1:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:460 ----
done_pull = memory_pull_request_step(&req_stage,
			&write_val,
			&mem_addr,
			&data,
			&bmask,
			&active,
			&write,
			&addr_in,
			&data_in,
			&byte_mask);
//----end code block-------

 _incrementPointer(1);
}

case 2:
{

//if statement , line:470
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

//wait statement , line:471
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
 
//code_block_statement 
//----code block from file memorytop.sitar, line:473 ----
l2_stage = 0;
//----end code block-------

 _incrementPointer(3);
}

case 1 :
{
//do-while statement , line:474
 int _dowhile_iteration;
for(_dowhile_iteration=1; _dowhile_iteration<=SITAR_ITERATION_LIMIT; _dowhile_iteration++)
{
//execute the sequence  
switch(_pointer[4])
{

case 0:
{

//if statement , line:475
if(((((((current_time.phase()))==(0))))))
_if_flag[1]=true;
else
_if_flag[1]=false;
 _incrementPointer(4);
}

case 1:
{

if(_if_flag[1]==true)
{

switch(_pointer[5])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:476 ----
done_push = l2_cache_access_step(&l2_stage,
						0,
						write_val,
						bmask,
						mem_addr,
						data,
						&read_data,
						&mem_active,
						&mem_write,
						&mem_addr_out,
						&mem_data_out,
						&mem_byte_mask,
						&mem_data_in);
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
//----code block from file memorytop.sitar, line:490 ----
done_push = l2_cache_access_step(&l2_stage,
						1,
						write_val,
						bmask,
						mem_addr,
						data,
						&read_data,
						&mem_active,
						&mem_write,
						&mem_addr_out,
						&mem_data_out,
						&mem_byte_mask,
						&mem_data_in);
//----end code block-------

 _incrementPointer(6);
}

case 1: break;
}

}

if((_if_flag[1]==true && _pointer[5]>=_pointer_last_value[5]) || (_if_flag[1]==false&& _pointer[6]>= _pointer_last_value[6]))

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

case 2:
{

//if statement , line:504
if((((!((done_push))))))
_if_flag[2]=true;
else
_if_flag[2]=false;
 _incrementPointer(4);
}

case 3:
{

if(_if_flag[2]==true)
{

switch(_pointer[7])
{

case 0:
{

//wait statement , line:504
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
 _incrementPointer(4);
 _pointer[7]=0;

}

 else 
 //if-statement has converged
 break;
}

case 4: break;
}

if(_pointer[4]< _pointer_last_value[4])  
break; //sequence has converged  
 else 
 if(_pointer[4]==_pointer_last_value[4] && ((((!((done_push)))))==true))
 {
//re-activate the sequence	
_pointer[4]=0;	
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
 		std::cerr<<"\nERROR:Iteration limit exceeded for do-while loop on line:474 in file memorytop.sitar";
		_pointer[4]=0;                                                               
		_incrementPointer(3);                                                                    
	}                                                                                              
	else if(_pointer[4]<_pointer_last_value[4])                                    
	{                                                                                               
		//sequence just converged;                                                              
		break;                                                                                  
	}                                                                                               
	else if (_pointer[4]==_pointer_last_value[4] && ((((!((done_push)))))==false))  
	{                                                                                               
		//terminate the do-while statement                                                      
		_pointer[4]=0;                                                               
		_incrementPointer(3);                                                                    
	} ;                                                                                              
};                                                                                                   
case 2:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:507 ----
resp_stage = 0;
//----end code block-------

 _incrementPointer(3);
}

case 3 :
{
//do-while statement , line:508
 int _dowhile_iteration;
for(_dowhile_iteration=1; _dowhile_iteration<=SITAR_ITERATION_LIMIT; _dowhile_iteration++)
{
//execute the sequence  
switch(_pointer[8])
{

case 0:
{

//wait-until statement , line:509
if(((((((current_time.phase()))==(1))))))
 _incrementPointer(8);
else
 break;
 }
case 1:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:510 ----
done_push = memory_send_response_step(&resp_stage, &data_out, read_data);
//----end code block-------

 _incrementPointer(8);
}

case 2:
{

//if statement , line:511
if((((!((done_push))))))
_if_flag[3]=true;
else
_if_flag[3]=false;
 _incrementPointer(8);
}

case 3:
{

if(_if_flag[3]==true)
{

switch(_pointer[9])
{

case 0:
{

//wait statement , line:511
_timer[2] = sitar::time(current_time)+sitar::time(0,1);
 _incrementPointer(9);
}

case 1:
{
if(current_time>=_timer[2])
 _incrementPointer(9);
else
 break; 
}
case 2: break;
}

}

else
{

}

if((_if_flag[3]==true && _pointer[9]>=_pointer_last_value[9]) || (_if_flag[3]==false))

{
 //if-statement has terminated
 _incrementPointer(8);
 _pointer[9]=0;

}

 else 
 //if-statement has converged
 break;
}

case 4: break;
}

if(_pointer[8]< _pointer_last_value[8])  
break; //sequence has converged  
 else 
 if(_pointer[8]==_pointer_last_value[8] && ((((!((done_push)))))==true))
 {
//re-activate the sequence	
_pointer[8]=0;	
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
 		std::cerr<<"\nERROR:Iteration limit exceeded for do-while loop on line:508 in file memorytop.sitar";
		_pointer[8]=0;                                                               
		_incrementPointer(3);                                                                    
	}                                                                                              
	else if(_pointer[8]<_pointer_last_value[8])                                    
	{                                                                                               
		//sequence just converged;                                                              
		break;                                                                                  
	}                                                                                               
	else if (_pointer[8]==_pointer_last_value[8] && ((((!((done_push)))))==false))  
	{                                                                                               
		//terminate the do-while statement                                                      
		_pointer[8]=0;                                                               
		_incrementPointer(3);                                                                    
	} ;                                                                                              
};                                                                                                   
case 4:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:514 ----
req_stage = 0;
			bmask = 0xff;
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
 		std::cerr<<"\nERROR:Iteration limit exceeded for do-while loop on line:458 in file memorytop.sitar";
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
			std::cerr<<"\nERROR:Iteration limit exceeded in behavior block of module L2Cache.";
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
	void  L2Cache<trace>::_resetBehavior()
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
