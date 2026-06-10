	
	#ifndef MEMORY_H
	#define MEMORY_H
	//======================================
	//file Memory.h                                                  
	//Describes module Memory                                      
	//Auto-generated from input file "memorytop.sitar" on 2026-4-16 at time 12:26:36   
	 //(This design unit is parameterized. Generating code into a .h file only) 
	
	//======================================
	#include<iostream>
	#include<iomanip>

	//sitar core classes
	#include"sitar_module.h"
	#include"sitar_time.h"
	//user included files
	 
//----code block from file memorytop.sitar, line:532 ----
#include <stdio.h>
//----end code block-------

//----code block from file memorytop.sitar, line:533 ----
extern "C" {
		#include "memory.h"
		#include "bridge_module_helpers.h"
		uint64_t getDoubleWordInMemory(uint32_t addr);
		void setDoubleWordInMemory(uint32_t addr, uint64_t data, uint8_t bm);
		int initializeMemory(char *); 
		int allocateMemory(unsigned int log_memory_size);
		int dumpMemoryToFile(char* memoryMapFile);
		void setMemoryTraceFile(FILE* fp);
	}
	
//----end code block-------

//----code block from file memorytop.sitar, line:544 ----
#include "string.h"
//----end code block-------


	namespace sitar{

	template<int size=32,int DELAY=0,bool trace=false>
	class Memory:public module
	{

		public:
			//constructor
			Memory();
	 
			void runBehavior(const time& current_time);
			inline void _incrementPointer(unsigned int ptr_number){_pointer[ptr_number]++; _reexecute=true;}
			void _resetBehavior();

			
			//Auto-generated variables for recording state in behavior block
			private:
			static const unsigned int _num_pointers=8; //total pointers used by this module
			static const unsigned int _num_timers=4;  	//total timers used by this module
			static const unsigned int _num_if_flags=3; //total if_flags used
			
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
//----code block from file memorytop.sitar, line:546 ----
uint64_t read_data; bool done_pull; bool done_push; bool write_val;
//----end code block-------

//----code block from file memorytop.sitar, line:547 ----
uint32_t mem_addr;
//----end code block-------

//----code block from file memorytop.sitar, line:548 ----
uint64_t data; uint8_t req_stage; uint8_t resp_stage;
//----end code block-------

//----code block from file memorytop.sitar, line:549 ----
uint8_t bmask;
//----end code block-------

//----code block from file memorytop.sitar, line:550 ----
char ifilename[100],ofilename[100], tfilename[100]; uint8_t requests_handled=0;
//----end code block-------

//----code block from file memorytop.sitar, line:551 ----
unsigned int long mem_size;
//----end code block-------


	};
	}
	//==========================================


	
	namespace sitar{
	//Constructor
	template<int size,int DELAY,bool trace>
	Memory<size,DELAY,trace>::Memory()
	{
		using std::cout;
		_type="Memory";
	
		
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

_pointer_last_value[2]=2;
_pointer_last_value[4]=3;
_pointer_last_value[5]=4;
_pointer_last_value[7]=2;
_pointer_last_value[6]=4;
_pointer_last_value[3]=5;
_pointer_last_value[1]=4;
_pointer_last_value[0]=2;
	}





	

	//runBehavior 
	template<int size,int DELAY,bool trace>
	void  Memory<size,DELAY,trace>::runBehavior(const time& current_time)
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
//----code block from file memorytop.sitar, line:554 ----

	mem_size = size;
	allocateMemory(mem_size);
	strcpy(ifilename, "../test_memmap_input_0.txt");
	strcpy(ofilename, "test_memmap_output_0.txt");
	strcpy(tfilename, "test_memmap_trace_0.txt");
	initializeMemory(ifilename);
	if (trace)
		setMemoryTraceFile(fopen(tfilename, "w"));
	log.turnON();
	req_stage = 0;
	bmask = 0xff;
	
//----end code block-------

 _incrementPointer(0);
}

case 1 :
{
//do-while statement , line:567
 int _dowhile_iteration;
for(_dowhile_iteration=1; _dowhile_iteration<=SITAR_ITERATION_LIMIT; _dowhile_iteration++)
{
//execute the sequence  
switch(_pointer[1])
{

case 0:
{

//wait-until statement , line:568
if(((((((current_time.phase()))==(0))))))
 _incrementPointer(1);
else
 break;
 }
case 1:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:569 ----
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

//if statement , line:579
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

//wait statement , line:580
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

//if statement , line:582
if((((!((write_val))))))
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

//wait-for -time statement , line:583
_timer[1] = sitar::time(current_time)+sitar::time(((DELAY)),((0)));
 _incrementPointer(4);
}

case 1:
{
if(current_time>=_timer[1])
 _incrementPointer(4);
else
 break; 
}
case 2:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:584 ----
read_data = getDoubleWordInMemory(mem_addr);
//----end code block-------

 _incrementPointer(4);
}

case 3: break;
}

}

else
{

switch(_pointer[5])
{

case 0:
{

//wait-for -time statement , line:586
_timer[2] = sitar::time(current_time)+sitar::time(((DELAY)),((0)));
 _incrementPointer(5);
}

case 1:
{
if(current_time>=_timer[2])
 _incrementPointer(5);
else
 break; 
}
case 2:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:587 ----
setDoubleWordInMemory(mem_addr, data, bmask);
//----end code block-------

 _incrementPointer(5);
}

case 3:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:588 ----
read_data = data;
//----end code block-------

 _incrementPointer(5);
}

case 4: break;
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
//----code block from file memorytop.sitar, line:591 ----
resp_stage = 0;
//----end code block-------

 _incrementPointer(3);
}

case 3 :
{
//do-while statement , line:592
 int _dowhile_iteration;
for(_dowhile_iteration=1; _dowhile_iteration<=SITAR_ITERATION_LIMIT; _dowhile_iteration++)
{
//execute the sequence  
switch(_pointer[6])
{

case 0:
{

//wait-until statement , line:593
if(((((((current_time.phase()))==(1))))))
 _incrementPointer(6);
else
 break;
 }
case 1:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:594 ----
done_push = memory_send_response_step(&resp_stage, &data_out, read_data);
//----end code block-------

 _incrementPointer(6);
}

case 2:
{

//if statement , line:595
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

//wait statement , line:595
_timer[3] = sitar::time(current_time)+sitar::time(0,1);
 _incrementPointer(7);
}

case 1:
{
if(current_time>=_timer[3])
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
 		std::cerr<<"\nERROR:Iteration limit exceeded for do-while loop on line:592 in file memorytop.sitar";
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
//----code block from file memorytop.sitar, line:598 ----
requests_handled++;
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
 		std::cerr<<"\nERROR:Iteration limit exceeded for do-while loop on line:567 in file memorytop.sitar";
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
			std::cerr<<"\nERROR:Iteration limit exceeded in behavior block of module Memory.";
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
	template<int size,int DELAY,bool trace>
	void  Memory<size,DELAY,trace>::_resetBehavior()
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
