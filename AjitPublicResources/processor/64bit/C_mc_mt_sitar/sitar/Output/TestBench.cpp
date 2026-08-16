 
	//====================================================================
	//file TestBench.cpp                                                 
	//Describes module TestBench                                      
	//Auto-generated from input "cop.sitar" on 2026-7-19 at time 22:6:27   
	//====================================================================
	#include"TestBench.h"
	#include<iostream>
	#include<iomanip>
	
	namespace sitar{
	//Constructor
	
	TestBench::TestBench()
	{
		using std::cout;
		_type="TestBench";
	
		
		//Initialize variables used by behavior block
		for( int i=0;i<int(_num_pointers);i++) _pointer[i]=0;
		for( int i=0;i<int(_num_timers  );i++)   _timer[i]=0;
		for( int i=0;i<int(_num_if_flags);i++) _if_flag[i]=0;
		_terminated=0;
		_reexecute=0;
	
		 
//---Initializing inport data_in---
data_in.setInstanceId("data_in");
addInport(&data_in,"data_in");

//---Initializing inport irq_level---
irq_level.setInstanceId("irq_level");
addInport(&irq_level,"irq_level");

//---Initializing outport active---
active.setInstanceId("active");
addOutport(&active,"active");

//---Initializing outport write---
write.setInstanceId("write");
addOutport(&write,"write");

//---Initializing outport addr_out---
addr_out.setInstanceId("addr_out");
addOutport(&addr_out,"addr_out");

//---Initializing outport data_out---
data_out.setInstanceId("data_out");
addOutport(&data_out,"data_out");

//---Initializing outport byte_mask---
byte_mask.setInstanceId("byte_mask");
addOutport(&byte_mask,"byte_mask");

//---Initializing outport coh_fill_kind---
coh_fill_kind.setInstanceId("coh_fill_kind");
addOutport(&coh_fill_kind,"coh_fill_kind");

//---Initializing outport coh_fill_pa_line---
coh_fill_pa_line.setInstanceId("coh_fill_pa_line");
addOutport(&coh_fill_pa_line,"coh_fill_pa_line");

//---Initializing outport coh_fill_va_line---
coh_fill_va_line.setInstanceId("coh_fill_va_line");
addOutport(&coh_fill_va_line,"coh_fill_va_line");

//---Initializing inport coh_icache_inval---
coh_icache_inval.setInstanceId("coh_icache_inval");
addInport(&coh_icache_inval,"coh_icache_inval");

//---Initializing inport coh_dcache_inval---
coh_dcache_inval.setInstanceId("coh_dcache_inval");
addInport(&coh_dcache_inval,"coh_dcache_inval");

_pointer_last_value[1]=3;
_pointer_last_value[0]=1;
	}





	

	//runBehavior 
	
	void  TestBench::runBehavior(const time& current_time)
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

case 0 :
{
//do-while statement , line:43
 int _dowhile_iteration;
for(_dowhile_iteration=1; _dowhile_iteration<=SITAR_ITERATION_LIMIT; _dowhile_iteration++)
{
//execute the sequence  
switch(_pointer[1])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file cop.sitar, line:44 ----

		if (!(state)) {
			module = new AjitTbDriver;
			
			lf = hierarchicalId() + "_log.txt";

			extract_success = extract_bracket_number(lf.c_str(), &instance_number);
			if (!(extract_success)) 
				log << "Instance Id of module not found." << endl;
			else
				log << "My Instance string was " << lf << " and I extracted " << instance_number << " as my id" << endl;
			
			module->id = instance_number;
			module->owner_tid = -1;	
			module->log = log;
			ajit_memory_shim_set_ports(instance_number, &active, &write, &addr_out, &data_in, &data_out, &byte_mask, &irq_level,
				&coh_fill_kind, &coh_fill_pa_line, &coh_fill_va_line, &coh_icache_inval, &coh_dcache_inval);
			state = true;
			
			log << "TB: init shim ports active=" << &active << " write=" << &write << " addr=" << &addr_out << std::endl;
		
		}
		
		
		
		log << "Before running module for cycle " << current_time << endl;
		
		//setLogPrefix(current_time);
		
		module->run(current_time.toUint64());
		
		log << "After running module for cycle " << current_time << endl;
	
//----end code block-------

 _incrementPointer(1);
}

case 1:
{

//wait-for -time statement , line:77
_timer[0] = sitar::time(current_time)+sitar::time(((0)),((1)));
 _incrementPointer(1);
}

case 2:
{
if(current_time>=_timer[0])
 _incrementPointer(1);
else
 break; 
}
case 3: break;
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
 		std::cerr<<"\nERROR:Iteration limit exceeded for do-while loop on line:43 in file cop.sitar";
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
case 1: break;
}

		
		}

		
			//if iteration limit has exceeded, throw error and stop simulation 
			if(_reexecute==1)
			{
			std::cerr<<"\nERROR:Iteration limit exceeded in behavior block of module TestBench.";
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
	
	void  TestBench::_resetBehavior()
	{
		//reset variables used by behavior block
		for( int i=0;i<int(_num_pointers);i++) _pointer[i]=0;
		for( int i=0;i<int(_num_timers  );i++)   _timer[i]=0;
		for( int i=0;i<int(_num_if_flags);i++) _if_flag[i]=0;
		_terminated=0;
		_reexecute=0;
	}


	
	}

	