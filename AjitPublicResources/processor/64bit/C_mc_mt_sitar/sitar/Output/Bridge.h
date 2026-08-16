	
	#ifndef BRIDGE_H
	#define BRIDGE_H
	//======================================
	//file Bridge.h                                                  
	//Describes module Bridge                                      
	//Auto-generated from input file "memorytop.sitar" on 2026-7-19 at time 22:6:27   
	 //(This design unit is parameterized. Generating code into a .h file only) 
	
	//======================================
	#include<iostream>
	#include<iomanip>

	//sitar core classes
	#include"sitar_module.h"
	#include"sitar_time.h"
	//user included files
	 
//----code block from file memorytop.sitar, line:202 ----
#include "string.h"
//----end code block-------

//----code block from file memorytop.sitar, line:203 ----
extern "C" {
		#include "bridge_module_helpers.h"
		#include "Ancillary.h"
	}
	
//----end code block-------


	namespace sitar{

	template<int size=32,int DELAY=0,int PORTS=4,bool trace=false>
	class Bridge:public module
	{

		public:
			//constructor
			Bridge();
	 
			void runBehavior(const time& current_time);
			inline void _incrementPointer(unsigned int ptr_number){_pointer[ptr_number]++; _reexecute=true;}
			void _resetBehavior();

			
			//Auto-generated variables for recording state in behavior block
			private:
			static const unsigned int _num_pointers=35; //total pointers used by this module
			static const unsigned int _num_timers=10;  	//total timers used by this module
			static const unsigned int _num_if_flags=18; //total if_flags used
			
			//State holders 
			//the +1 for array size is to avoid zero-sized arrays
			unsigned int 	_pointer[_num_pointers+1];		//pointers for sequences
			time 	_timer[_num_timers+1];			//timers for wait statements
			bool		_if_flag[_num_if_flags+1];		//if_flags
			unsigned int    _pointer_last_value[_num_pointers+1];	//last value taken by each sequence pointer

	
			//other declarations
			public:
			 
inport<1> active[PORTS];
inport<1> write[PORTS];
inport<32> addr_in[PORTS];
inport<64> data_in[PORTS];
inport<8> byte_mask[PORTS];
outport<64> data_out[PORTS];
inport<8> coh_fill_kind[PORTS];
inport<32> coh_fill_pa_line[PORTS];
inport<32> coh_fill_va_line[PORTS];
outport<32> coh_icache_inval[PORTS];
outport<32> coh_dcache_inval[PORTS];
outport<1> mem_active;
outport<1> mem_write;
outport<32> mem_addr_out;
outport<64> mem_data_out;
outport<8> mem_byte_mask;
inport<64> mem_data_in;
outport<64> sp_request;
inport<32> sp_response;
outport<64> timer_request;
inport<32> timer_response;
outport<64> irc_request;
inport<32> irc_response;
outport<64> serial_tx_request;
inport<32> serial_tx_response;
outport<64> serial_rx_request;
inport<32> serial_rx_response;
//----code block from file memorytop.sitar, line:209 ----
bool done_pull; bool done_push; bool selected_valid; bool port_ready;
//----end code block-------

//----code block from file memorytop.sitar, line:210 ----
bool write_state[PORTS]; bool ready_state[PORTS]; bool write_val;
//----end code block-------

//----code block from file memorytop.sitar, line:211 ----
uint8_t bmask_state[PORTS]; uint8_t req_stage[PORTS]; uint8_t issue_stage; uint8_t resp_stage;
//----end code block-------

//----code block from file memorytop.sitar, line:212 ----
uint8_t coh_fill_stage[PORTS]; uint8_t coh_emit_core; uint8_t coh_emit_kind;
//----end code block-------

//----code block from file memorytop.sitar, line:213 ----
uint8_t coh_kind_state[PORTS]; uint32_t coh_pa_state[PORTS]; uint32_t coh_va_state[PORTS];
//----end code block-------

//----code block from file memorytop.sitar, line:214 ----
uint32_t addr_state[PORTS]; uint32_t req_addr; uint32_t low_resp32; uint32_t high_resp32; uint32_t periph_wdata;
//----end code block-------

//----code block from file memorytop.sitar, line:215 ----
uint64_t data_state[PORTS]; uint64_t req_data; uint64_t resp_data;
//----end code block-------

//----code block from file memorytop.sitar, line:216 ----
uint8_t rr_start = 0; uint8_t probe_count; uint8_t current_port; uint8_t selected_port; uint8_t low_bm; uint8_t high_bm; uint8_t periph_bm; uint8_t target_kind;
//----end code block-------


	};
	}
	//==========================================


	
	namespace sitar{
	//Constructor
	template<int size,int DELAY,int PORTS,bool trace>
	Bridge<size,DELAY,PORTS,trace>::Bridge()
	{
		using std::cout;
		_type="Bridge";
	
		
		//Initialize variables used by behavior block
		for( int i=0;i<int(_num_pointers);i++) _pointer[i]=0;
		for( int i=0;i<int(_num_timers  );i++)   _timer[i]=0;
		for( int i=0;i<int(_num_if_flags);i++) _if_flag[i]=0;
		_terminated=0;
		_reexecute=0;
	
		 
//----Initializing inport-array active------
for(int i=0;i<(PORTS);i++)
{

std::string pname="active["+sitar::toString(i)+"]";
active[i].setInstanceId(pname);
addInport(&active[i],"active["+sitar::toString(i)+"]");

}

//----Initializing inport-array write------
for(int i=0;i<(PORTS);i++)
{

std::string pname="write["+sitar::toString(i)+"]";
write[i].setInstanceId(pname);
addInport(&write[i],"write["+sitar::toString(i)+"]");

}

//----Initializing inport-array addr_in------
for(int i=0;i<(PORTS);i++)
{

std::string pname="addr_in["+sitar::toString(i)+"]";
addr_in[i].setInstanceId(pname);
addInport(&addr_in[i],"addr_in["+sitar::toString(i)+"]");

}

//----Initializing inport-array data_in------
for(int i=0;i<(PORTS);i++)
{

std::string pname="data_in["+sitar::toString(i)+"]";
data_in[i].setInstanceId(pname);
addInport(&data_in[i],"data_in["+sitar::toString(i)+"]");

}

//----Initializing inport-array byte_mask------
for(int i=0;i<(PORTS);i++)
{

std::string pname="byte_mask["+sitar::toString(i)+"]";
byte_mask[i].setInstanceId(pname);
addInport(&byte_mask[i],"byte_mask["+sitar::toString(i)+"]");

}

//----Initializing outport-array data_out------
for(int i=0;i<(PORTS);i++)
{

std::string pname="data_out["+sitar::toString(i)+"]";
data_out[i].setInstanceId(pname);
addOutport(&data_out[i],"data_out["+sitar::toString(i)+"]");

}

//----Initializing inport-array coh_fill_kind------
for(int i=0;i<(PORTS);i++)
{

std::string pname="coh_fill_kind["+sitar::toString(i)+"]";
coh_fill_kind[i].setInstanceId(pname);
addInport(&coh_fill_kind[i],"coh_fill_kind["+sitar::toString(i)+"]");

}

//----Initializing inport-array coh_fill_pa_line------
for(int i=0;i<(PORTS);i++)
{

std::string pname="coh_fill_pa_line["+sitar::toString(i)+"]";
coh_fill_pa_line[i].setInstanceId(pname);
addInport(&coh_fill_pa_line[i],"coh_fill_pa_line["+sitar::toString(i)+"]");

}

//----Initializing inport-array coh_fill_va_line------
for(int i=0;i<(PORTS);i++)
{

std::string pname="coh_fill_va_line["+sitar::toString(i)+"]";
coh_fill_va_line[i].setInstanceId(pname);
addInport(&coh_fill_va_line[i],"coh_fill_va_line["+sitar::toString(i)+"]");

}

//----Initializing outport-array coh_icache_inval------
for(int i=0;i<(PORTS);i++)
{

std::string pname="coh_icache_inval["+sitar::toString(i)+"]";
coh_icache_inval[i].setInstanceId(pname);
addOutport(&coh_icache_inval[i],"coh_icache_inval["+sitar::toString(i)+"]");

}

//----Initializing outport-array coh_dcache_inval------
for(int i=0;i<(PORTS);i++)
{

std::string pname="coh_dcache_inval["+sitar::toString(i)+"]";
coh_dcache_inval[i].setInstanceId(pname);
addOutport(&coh_dcache_inval[i],"coh_dcache_inval["+sitar::toString(i)+"]");

}

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

//---Initializing outport sp_request---
sp_request.setInstanceId("sp_request");
addOutport(&sp_request,"sp_request");

//---Initializing inport sp_response---
sp_response.setInstanceId("sp_response");
addInport(&sp_response,"sp_response");

//---Initializing outport timer_request---
timer_request.setInstanceId("timer_request");
addOutport(&timer_request,"timer_request");

//---Initializing inport timer_response---
timer_response.setInstanceId("timer_response");
addInport(&timer_response,"timer_response");

//---Initializing outport irc_request---
irc_request.setInstanceId("irc_request");
addOutport(&irc_request,"irc_request");

//---Initializing inport irc_response---
irc_response.setInstanceId("irc_response");
addInport(&irc_response,"irc_response");

//---Initializing outport serial_tx_request---
serial_tx_request.setInstanceId("serial_tx_request");
addOutport(&serial_tx_request,"serial_tx_request");

//---Initializing inport serial_tx_response---
serial_tx_response.setInstanceId("serial_tx_response");
addInport(&serial_tx_response,"serial_tx_response");

//---Initializing outport serial_rx_request---
serial_rx_request.setInstanceId("serial_rx_request");
addOutport(&serial_rx_request,"serial_rx_request");

//---Initializing inport serial_rx_response---
serial_rx_response.setInstanceId("serial_rx_response");
addInport(&serial_rx_response,"serial_rx_response");

_pointer_last_value[4]=1;
_pointer_last_value[3]=3;
_pointer_last_value[5]=1;
_pointer_last_value[6]=1;
_pointer_last_value[2]=6;
_pointer_last_value[7]=2;
_pointer_last_value[12]=2;
_pointer_last_value[11]=4;
_pointer_last_value[10]=1;
_pointer_last_value[15]=2;
_pointer_last_value[14]=4;
_pointer_last_value[13]=1;
_pointer_last_value[17]=2;
_pointer_last_value[16]=4;
_pointer_last_value[18]=1;
_pointer_last_value[21]=2;
_pointer_last_value[20]=4;
_pointer_last_value[19]=2;
_pointer_last_value[9]=8;
_pointer_last_value[25]=2;
_pointer_last_value[24]=4;
_pointer_last_value[27]=2;
_pointer_last_value[26]=4;
_pointer_last_value[23]=4;
_pointer_last_value[30]=2;
_pointer_last_value[29]=4;
_pointer_last_value[32]=2;
_pointer_last_value[31]=4;
_pointer_last_value[28]=4;
_pointer_last_value[22]=2;
_pointer_last_value[34]=2;
_pointer_last_value[33]=4;
_pointer_last_value[8]=6;
_pointer_last_value[1]=6;
_pointer_last_value[0]=2;
	}





	

	//runBehavior 
	template<int size,int DELAY,int PORTS,bool trace>
	void  Bridge<size,DELAY,PORTS,trace>::runBehavior(const time& current_time)
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
//----code block from file memorytop.sitar, line:219 ----

	for (int i = 0; i < PORTS; ++i) {
		req_stage[i] = 0;
		ready_state[i] = false;
		write_state[i] = false;
		addr_state[i] = 0;
		data_state[i] = 0;
		bmask_state[i] = 0xff;
		coh_fill_stage[i] = 0;
		coh_kind_state[i] = 0;
		coh_pa_state[i] = 0;
		coh_va_state[i] = 0;
	}
	
//----end code block-------

 _incrementPointer(0);
}

case 1 :
{
//do-while statement , line:233
 int _dowhile_iteration;
for(_dowhile_iteration=1; _dowhile_iteration<=SITAR_ITERATION_LIMIT; _dowhile_iteration++)
{
//execute the sequence  
switch(_pointer[1])
{

case 0:
{

//wait-until statement , line:234
if(((((((current_time.phase()))==(0))))))
 _incrementPointer(1);
else
 break;
 }
case 1:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:235 ----
for (int coh_i = 0; coh_i < PORTS; ++coh_i) {
			bridge_poll_coherence_fill_step(&coh_fill_stage[coh_i],
				(uint8_t) coh_i,
				&coh_kind_state[coh_i],
				&coh_pa_state[coh_i],
				&coh_va_state[coh_i],
				&coh_fill_kind[coh_i],
				&coh_fill_pa_line[coh_i],
				&coh_fill_va_line[coh_i]);
		}
//----end code block-------

 _incrementPointer(1);
}

case 2:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:246 ----
probe_count = 0; selected_valid = false; selected_port = rr_start;
//----end code block-------

 _incrementPointer(1);
}

case 3 :
{
//do-while statement , line:247
 int _dowhile_iteration;
for(_dowhile_iteration=1; _dowhile_iteration<=SITAR_ITERATION_LIMIT; _dowhile_iteration++)
{
//execute the sequence  
switch(_pointer[2])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:248 ----
current_port = (uint8_t) ((rr_start + probe_count) % PORTS); port_ready = ready_state[current_port];
//----end code block-------

 _incrementPointer(2);
}

case 1:
{

//if statement , line:249
if((((!((port_ready))))))
_if_flag[0]=true;
else
_if_flag[0]=false;
 _incrementPointer(2);
}

case 2:
{

if(_if_flag[0]==true)
{

switch(_pointer[3])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:250 ----
done_pull = bridge_pull_cpu_request_step(&req_stage[current_port],
					&write_state[current_port],
					&addr_state[current_port],
					&data_state[current_port],
					&bmask_state[current_port],
					&active[current_port],
					&write[current_port],
					&addr_in[current_port],
					&data_in[current_port],
					&byte_mask[current_port]);
//----end code block-------

 _incrementPointer(3);
}

case 1:
{

//if statement , line:260
if((((((done_pull))))))
_if_flag[1]=true;
else
_if_flag[1]=false;
 _incrementPointer(3);
}

case 2:
{

if(_if_flag[1]==true)
{

switch(_pointer[4])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:261 ----
ready_state[current_port] = true;
//----end code block-------

 _incrementPointer(4);
}

case 1: break;
}

}

else
{

}

if((_if_flag[1]==true && _pointer[4]>=_pointer_last_value[4]) || (_if_flag[1]==false))

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

if((_if_flag[0]==true && _pointer[3]>=_pointer_last_value[3]) || (_if_flag[0]==false))

{
 //if-statement has terminated
 _incrementPointer(2);
 _pointer[3]=0;

}

 else 
 //if-statement has converged
 break;
}

case 3:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:264 ----
port_ready = ready_state[current_port];
//----end code block-------

 _incrementPointer(2);
}

case 4:
{

//if statement , line:265
if(((((((!((selected_valid)))))&&((port_ready))))))
_if_flag[2]=true;
else
_if_flag[2]=false;
 _incrementPointer(2);
}

case 5:
{

if(_if_flag[2]==true)
{

switch(_pointer[5])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:266 ----
selected_valid = true; selected_port = current_port;
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
//----code block from file memorytop.sitar, line:268 ----
probe_count = probe_count + 1;
//----end code block-------

 _incrementPointer(6);
}

case 1: break;
}

}

if((_if_flag[2]==true && _pointer[5]>=_pointer_last_value[5]) || (_if_flag[2]==false&& _pointer[6]>= _pointer_last_value[6]))

{
 //if-statement has terminated
 _incrementPointer(2);
 _pointer[5]=0;

 _pointer[6]=0;

}

 else 
 //if-statement has converged
 break;
}

case 6: break;
}

if(_pointer[2]< _pointer_last_value[2])  
break; //sequence has converged  
 else 
 if(_pointer[2]==_pointer_last_value[2] && (((((((((probe_count)<(PORTS)))))&&(((!((selected_valid))))))))==true))
 {
//re-activate the sequence	
_pointer[2]=0;	
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
 		std::cerr<<"\nERROR:Iteration limit exceeded for do-while loop on line:247 in file memorytop.sitar";
		_pointer[2]=0;                                                               
		_incrementPointer(1);                                                                    
	}                                                                                              
	else if(_pointer[2]<_pointer_last_value[2])                                    
	{                                                                                               
		//sequence just converged;                                                              
		break;                                                                                  
	}                                                                                               
	else if (_pointer[2]==_pointer_last_value[2] && (((((((((probe_count)<(PORTS)))))&&(((!((selected_valid))))))))==false))  
	{                                                                                               
		//terminate the do-while statement                                                      
		_pointer[2]=0;                                                               
		_incrementPointer(1);                                                                    
	} ;                                                                                              
};                                                                                                   
case 4:
{

//if statement , line:272
if((((!((selected_valid))))))
_if_flag[3]=true;
else
_if_flag[3]=false;
 _incrementPointer(1);
}

case 5:
{

if(_if_flag[3]==true)
{

switch(_pointer[7])
{

case 0:
{

//wait statement , line:273
_timer[0] = sitar::time(current_time)+sitar::time(0,1);
 _incrementPointer(7);
}

case 1:
{
if(current_time>=_timer[0])
 _incrementPointer(7);
else
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
 
//code_block_statement 
//----code block from file memorytop.sitar, line:275 ----
write_val = write_state[selected_port];
			req_addr = addr_state[selected_port];
			req_data = data_state[selected_port];
			target_kind = bridge_decode_target(req_addr);
			low_bm = bmask_state[selected_port] & 0x0f;
			high_bm = (uint8_t) ((bmask_state[selected_port] >> 4) & 0x0f);
//----end code block-------

 _incrementPointer(8);
}

case 1:
{

//if statement , line:282
if((((((target_kind)==(BRIDGE_TARGET_MEMORY))))))
_if_flag[4]=true;
else
_if_flag[4]=false;
 _incrementPointer(8);
}

case 2:
{

if(_if_flag[4]==true)
{

switch(_pointer[9])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:283 ----
issue_stage = 0;
//----end code block-------

 _incrementPointer(9);
}

case 1:
{

//if statement , line:284
if((((((write_val))))))
_if_flag[5]=true;
else
_if_flag[5]=false;
 _incrementPointer(9);
}

case 2:
{

if(_if_flag[5]==true)
{

switch(_pointer[10])
{

case 0 :
{
//do-while statement , line:285
 int _dowhile_iteration;
for(_dowhile_iteration=1; _dowhile_iteration<=SITAR_ITERATION_LIMIT; _dowhile_iteration++)
{
//execute the sequence  
switch(_pointer[11])
{

case 0:
{

//wait-until statement , line:286
if(((((((current_time.phase()))==(1))))))
 _incrementPointer(11);
else
 break;
 }
case 1:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:287 ----
done_push = bridge_issue_mem_write_step(&issue_stage,
							&mem_active,
							&mem_write,
							&mem_addr_out,
							&mem_data_out,
							&mem_byte_mask,
							req_addr,
							req_data,
							bmask_state[selected_port]);
//----end code block-------

 _incrementPointer(11);
}

case 2:
{

//if statement , line:296
if((((!((done_push))))))
_if_flag[6]=true;
else
_if_flag[6]=false;
 _incrementPointer(11);
}

case 3:
{

if(_if_flag[6]==true)
{

switch(_pointer[12])
{

case 0:
{

//wait statement , line:296
_timer[1] = sitar::time(current_time)+sitar::time(0,1);
 _incrementPointer(12);
}

case 1:
{
if(current_time>=_timer[1])
 _incrementPointer(12);
else
 break; 
}
case 2: break;
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

case 4: break;
}

if(_pointer[11]< _pointer_last_value[11])  
break; //sequence has converged  
 else 
 if(_pointer[11]==_pointer_last_value[11] && ((((!((done_push)))))==true))
 {
//re-activate the sequence	
_pointer[11]=0;	
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
 		std::cerr<<"\nERROR:Iteration limit exceeded for do-while loop on line:285 in file memorytop.sitar";
		_pointer[11]=0;                                                               
		_incrementPointer(10);                                                                    
	}                                                                                              
	else if(_pointer[11]<_pointer_last_value[11])                                    
	{                                                                                               
		//sequence just converged;                                                              
		break;                                                                                  
	}                                                                                               
	else if (_pointer[11]==_pointer_last_value[11] && ((((!((done_push)))))==false))  
	{                                                                                               
		//terminate the do-while statement                                                      
		_pointer[11]=0;                                                               
		_incrementPointer(10);                                                                    
	} ;                                                                                              
};                                                                                                   
case 1: break;
}

}

else
{

switch(_pointer[13])
{

case 0 :
{
//do-while statement , line:299
 int _dowhile_iteration;
for(_dowhile_iteration=1; _dowhile_iteration<=SITAR_ITERATION_LIMIT; _dowhile_iteration++)
{
//execute the sequence  
switch(_pointer[14])
{

case 0:
{

//wait-until statement , line:300
if(((((((current_time.phase()))==(1))))))
 _incrementPointer(14);
else
 break;
 }
case 1:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:301 ----
done_push = bridge_issue_mem_read_step(&issue_stage,
							&mem_active,
							&mem_write,
							&mem_addr_out,
							req_addr);
//----end code block-------

 _incrementPointer(14);
}

case 2:
{

//if statement , line:306
if((((!((done_push))))))
_if_flag[7]=true;
else
_if_flag[7]=false;
 _incrementPointer(14);
}

case 3:
{

if(_if_flag[7]==true)
{

switch(_pointer[15])
{

case 0:
{

//wait statement , line:306
_timer[2] = sitar::time(current_time)+sitar::time(0,1);
 _incrementPointer(15);
}

case 1:
{
if(current_time>=_timer[2])
 _incrementPointer(15);
else
 break; 
}
case 2: break;
}

}

else
{

}

if((_if_flag[7]==true && _pointer[15]>=_pointer_last_value[15]) || (_if_flag[7]==false))

{
 //if-statement has terminated
 _incrementPointer(14);
 _pointer[15]=0;

}

 else 
 //if-statement has converged
 break;
}

case 4: break;
}

if(_pointer[14]< _pointer_last_value[14])  
break; //sequence has converged  
 else 
 if(_pointer[14]==_pointer_last_value[14] && ((((!((done_push)))))==true))
 {
//re-activate the sequence	
_pointer[14]=0;	
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
 		std::cerr<<"\nERROR:Iteration limit exceeded for do-while loop on line:299 in file memorytop.sitar";
		_pointer[14]=0;                                                               
		_incrementPointer(13);                                                                    
	}                                                                                              
	else if(_pointer[14]<_pointer_last_value[14])                                    
	{                                                                                               
		//sequence just converged;                                                              
		break;                                                                                  
	}                                                                                               
	else if (_pointer[14]==_pointer_last_value[14] && ((((!((done_push)))))==false))  
	{                                                                                               
		//terminate the do-while statement                                                      
		_pointer[14]=0;                                                               
		_incrementPointer(13);                                                                    
	} ;                                                                                              
};                                                                                                   
case 1: break;
}

}

if((_if_flag[5]==true && _pointer[10]>=_pointer_last_value[10]) || (_if_flag[5]==false&& _pointer[13]>= _pointer_last_value[13]))

{
 //if-statement has terminated
 _incrementPointer(9);
 _pointer[10]=0;

 _pointer[13]=0;

}

 else 
 //if-statement has converged
 break;
}

case 3 :
{
//do-while statement , line:310
 int _dowhile_iteration;
for(_dowhile_iteration=1; _dowhile_iteration<=SITAR_ITERATION_LIMIT; _dowhile_iteration++)
{
//execute the sequence  
switch(_pointer[16])
{

case 0:
{

//wait-until statement , line:311
if(((((((current_time.phase()))==(0))))))
 _incrementPointer(16);
else
 break;
 }
case 1:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:312 ----
done_pull = bridge_collect_mem_response(&mem_data_in, &resp_data);
//----end code block-------

 _incrementPointer(16);
}

case 2:
{

//if statement , line:313
if((((!((done_pull))))))
_if_flag[8]=true;
else
_if_flag[8]=false;
 _incrementPointer(16);
}

case 3:
{

if(_if_flag[8]==true)
{

switch(_pointer[17])
{

case 0:
{

//wait statement , line:313
_timer[3] = sitar::time(current_time)+sitar::time(0,1);
 _incrementPointer(17);
}

case 1:
{
if(current_time>=_timer[3])
 _incrementPointer(17);
else
 break; 
}
case 2: break;
}

}

else
{

}

if((_if_flag[8]==true && _pointer[17]>=_pointer_last_value[17]) || (_if_flag[8]==false))

{
 //if-statement has terminated
 _incrementPointer(16);
 _pointer[17]=0;

}

 else 
 //if-statement has converged
 break;
}

case 4: break;
}

if(_pointer[16]< _pointer_last_value[16])  
break; //sequence has converged  
 else 
 if(_pointer[16]==_pointer_last_value[16] && ((((!((done_pull)))))==true))
 {
//re-activate the sequence	
_pointer[16]=0;	
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
 		std::cerr<<"\nERROR:Iteration limit exceeded for do-while loop on line:310 in file memorytop.sitar";
		_pointer[16]=0;                                                               
		_incrementPointer(9);                                                                    
	}                                                                                              
	else if(_pointer[16]<_pointer_last_value[16])                                    
	{                                                                                               
		//sequence just converged;                                                              
		break;                                                                                  
	}                                                                                               
	else if (_pointer[16]==_pointer_last_value[16] && ((((!((done_pull)))))==false))  
	{                                                                                               
		//terminate the do-while statement                                                      
		_pointer[16]=0;                                                               
		_incrementPointer(9);                                                                    
	} ;                                                                                              
};                                                                                                   
case 4:
{

//if statement , line:316
if((((!((write_val))))))
_if_flag[9]=true;
else
_if_flag[9]=false;
 _incrementPointer(9);
}

case 5:
{

if(_if_flag[9]==true)
{

switch(_pointer[18])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:317 ----
bridge_snoop_filter_note_memory_read(selected_port, (req_addr >> 6));
//----end code block-------

 _incrementPointer(18);
}

case 1: break;
}

}

else
{

}

if((_if_flag[9]==true && _pointer[18]>=_pointer_last_value[18]) || (_if_flag[9]==false))

{
 //if-statement has terminated
 _incrementPointer(9);
 _pointer[18]=0;

}

 else 
 //if-statement has converged
 break;
}

case 6:
{

//if statement , line:320
if((((((write_val))))))
_if_flag[10]=true;
else
_if_flag[10]=false;
 _incrementPointer(9);
}

case 7:
{

if(_if_flag[10]==true)
{

switch(_pointer[19])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:321 ----
coh_emit_core = 0; coh_emit_kind = 0;
//----end code block-------

 _incrementPointer(19);
}

case 1 :
{
//do-while statement , line:322
 int _dowhile_iteration;
for(_dowhile_iteration=1; _dowhile_iteration<=SITAR_ITERATION_LIMIT; _dowhile_iteration++)
{
//execute the sequence  
switch(_pointer[20])
{

case 0:
{

//wait-until statement , line:323
if(((((((current_time.phase()))==(1))))))
 _incrementPointer(20);
else
 break;
 }
case 1:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:324 ----
done_push = bridge_emit_coherence_invalidate_step(&coh_emit_core,
							&coh_emit_kind,
							selected_port,
							(req_addr >> 6),
							PORTS,
							&coh_icache_inval[coh_emit_core],
							&coh_dcache_inval[coh_emit_core]);
//----end code block-------

 _incrementPointer(20);
}

case 2:
{

//if statement , line:331
if((((!((done_push))))))
_if_flag[11]=true;
else
_if_flag[11]=false;
 _incrementPointer(20);
}

case 3:
{

if(_if_flag[11]==true)
{

switch(_pointer[21])
{

case 0:
{

//wait statement , line:331
_timer[4] = sitar::time(current_time)+sitar::time(0,1);
 _incrementPointer(21);
}

case 1:
{
if(current_time>=_timer[4])
 _incrementPointer(21);
else
 break; 
}
case 2: break;
}

}

else
{

}

if((_if_flag[11]==true && _pointer[21]>=_pointer_last_value[21]) || (_if_flag[11]==false))

{
 //if-statement has terminated
 _incrementPointer(20);
 _pointer[21]=0;

}

 else 
 //if-statement has converged
 break;
}

case 4: break;
}

if(_pointer[20]< _pointer_last_value[20])  
break; //sequence has converged  
 else 
 if(_pointer[20]==_pointer_last_value[20] && ((((!((done_push)))))==true))
 {
//re-activate the sequence	
_pointer[20]=0;	
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
 		std::cerr<<"\nERROR:Iteration limit exceeded for do-while loop on line:322 in file memorytop.sitar";
		_pointer[20]=0;                                                               
		_incrementPointer(19);                                                                    
	}                                                                                              
	else if(_pointer[20]<_pointer_last_value[20])                                    
	{                                                                                               
		//sequence just converged;                                                              
		break;                                                                                  
	}                                                                                               
	else if (_pointer[20]==_pointer_last_value[20] && ((((!((done_push)))))==false))  
	{                                                                                               
		//terminate the do-while statement                                                      
		_pointer[20]=0;                                                               
		_incrementPointer(19);                                                                    
	} ;                                                                                              
};                                                                                                   
case 2: break;
}

}

else
{

}

if((_if_flag[10]==true && _pointer[19]>=_pointer_last_value[19]) || (_if_flag[10]==false))

{
 //if-statement has terminated
 _incrementPointer(9);
 _pointer[19]=0;

}

 else 
 //if-statement has converged
 break;
}

case 8: break;
}

}

else
{

switch(_pointer[22])
{

case 0:
{

//if statement , line:335
if((((((write_val))))))
_if_flag[12]=true;
else
_if_flag[12]=false;
 _incrementPointer(22);
}

case 1:
{

if(_if_flag[12]==true)
{

switch(_pointer[23])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:336 ----
issue_stage = 0;
					if((req_addr & 0x4) == 0) {
						periph_wdata = (uint32_t) ((req_data >> 32) & 0xffffffffULL);
						periph_bm = high_bm;
					} else {
						periph_wdata = (uint32_t) (req_data & 0xffffffffULL);
						periph_bm = low_bm;
					}
//----end code block-------

 _incrementPointer(23);
}

case 1 :
{
//do-while statement , line:344
 int _dowhile_iteration;
for(_dowhile_iteration=1; _dowhile_iteration<=SITAR_ITERATION_LIMIT; _dowhile_iteration++)
{
//execute the sequence  
switch(_pointer[24])
{

case 0:
{

//wait-until statement , line:345
if(((((((current_time.phase()))==(1))))))
 _incrementPointer(24);
else
 break;
 }
case 1:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:346 ----
done_push = bridge_issue_target_access_step(&issue_stage,
							target_kind,
							&sp_request,
							&timer_request,
							&irc_request,
							&serial_tx_request,
							&serial_rx_request,
							false,
							periph_bm,
							req_addr,
							periph_wdata);
//----end code block-------

 _incrementPointer(24);
}

case 2:
{

//if statement , line:357
if((((!((done_push))))))
_if_flag[13]=true;
else
_if_flag[13]=false;
 _incrementPointer(24);
}

case 3:
{

if(_if_flag[13]==true)
{

switch(_pointer[25])
{

case 0:
{

//wait statement , line:357
_timer[5] = sitar::time(current_time)+sitar::time(0,1);
 _incrementPointer(25);
}

case 1:
{
if(current_time>=_timer[5])
 _incrementPointer(25);
else
 break; 
}
case 2: break;
}

}

else
{

}

if((_if_flag[13]==true && _pointer[25]>=_pointer_last_value[25]) || (_if_flag[13]==false))

{
 //if-statement has terminated
 _incrementPointer(24);
 _pointer[25]=0;

}

 else 
 //if-statement has converged
 break;
}

case 4: break;
}

if(_pointer[24]< _pointer_last_value[24])  
break; //sequence has converged  
 else 
 if(_pointer[24]==_pointer_last_value[24] && ((((!((done_push)))))==true))
 {
//re-activate the sequence	
_pointer[24]=0;	
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
 		std::cerr<<"\nERROR:Iteration limit exceeded for do-while loop on line:344 in file memorytop.sitar";
		_pointer[24]=0;                                                               
		_incrementPointer(23);                                                                    
	}                                                                                              
	else if(_pointer[24]<_pointer_last_value[24])                                    
	{                                                                                               
		//sequence just converged;                                                              
		break;                                                                                  
	}                                                                                               
	else if (_pointer[24]==_pointer_last_value[24] && ((((!((done_push)))))==false))  
	{                                                                                               
		//terminate the do-while statement                                                      
		_pointer[24]=0;                                                               
		_incrementPointer(23);                                                                    
	} ;                                                                                              
};                                                                                                   
case 2 :
{
//do-while statement , line:359
 int _dowhile_iteration;
for(_dowhile_iteration=1; _dowhile_iteration<=SITAR_ITERATION_LIMIT; _dowhile_iteration++)
{
//execute the sequence  
switch(_pointer[26])
{

case 0:
{

//wait-until statement , line:360
if(((((((current_time.phase()))==(0))))))
 _incrementPointer(26);
else
 break;
 }
case 1:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:361 ----
done_pull = bridge_collect_target_response(target_kind,
							&sp_response,
							&timer_response,
							&irc_response,
							&serial_tx_response,
							&serial_rx_response,
							&low_resp32);
//----end code block-------

 _incrementPointer(26);
}

case 2:
{

//if statement , line:368
if((((!((done_pull))))))
_if_flag[14]=true;
else
_if_flag[14]=false;
 _incrementPointer(26);
}

case 3:
{

if(_if_flag[14]==true)
{

switch(_pointer[27])
{

case 0:
{

//wait statement , line:368
_timer[6] = sitar::time(current_time)+sitar::time(0,1);
 _incrementPointer(27);
}

case 1:
{
if(current_time>=_timer[6])
 _incrementPointer(27);
else
 break; 
}
case 2: break;
}

}

else
{

}

if((_if_flag[14]==true && _pointer[27]>=_pointer_last_value[27]) || (_if_flag[14]==false))

{
 //if-statement has terminated
 _incrementPointer(26);
 _pointer[27]=0;

}

 else 
 //if-statement has converged
 break;
}

case 4: break;
}

if(_pointer[26]< _pointer_last_value[26])  
break; //sequence has converged  
 else 
 if(_pointer[26]==_pointer_last_value[26] && ((((!((done_pull)))))==true))
 {
//re-activate the sequence	
_pointer[26]=0;	
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
 		std::cerr<<"\nERROR:Iteration limit exceeded for do-while loop on line:359 in file memorytop.sitar";
		_pointer[26]=0;                                                               
		_incrementPointer(23);                                                                    
	}                                                                                              
	else if(_pointer[26]<_pointer_last_value[26])                                    
	{                                                                                               
		//sequence just converged;                                                              
		break;                                                                                  
	}                                                                                               
	else if (_pointer[26]==_pointer_last_value[26] && ((((!((done_pull)))))==false))  
	{                                                                                               
		//terminate the do-while statement                                                      
		_pointer[26]=0;                                                               
		_incrementPointer(23);                                                                    
	} ;                                                                                              
};                                                                                                   
case 3:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:370 ----
resp_data = req_data;
//----end code block-------

 _incrementPointer(23);
}

case 4: break;
}

}

else
{

switch(_pointer[28])
{

case 0:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:372 ----
issue_stage = 0;
//----end code block-------

 _incrementPointer(28);
}

case 1 :
{
//do-while statement , line:373
 int _dowhile_iteration;
for(_dowhile_iteration=1; _dowhile_iteration<=SITAR_ITERATION_LIMIT; _dowhile_iteration++)
{
//execute the sequence  
switch(_pointer[29])
{

case 0:
{

//wait-until statement , line:374
if(((((((current_time.phase()))==(1))))))
 _incrementPointer(29);
else
 break;
 }
case 1:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:375 ----
done_push = bridge_issue_target_access_step(&issue_stage,
							target_kind,
							&sp_request,
							&timer_request,
							&irc_request,
							&serial_tx_request,
							&serial_rx_request,
							true,
							0x0f,
							req_addr,
							0);
//----end code block-------

 _incrementPointer(29);
}

case 2:
{

//if statement , line:386
if((((!((done_push))))))
_if_flag[15]=true;
else
_if_flag[15]=false;
 _incrementPointer(29);
}

case 3:
{

if(_if_flag[15]==true)
{

switch(_pointer[30])
{

case 0:
{

//wait statement , line:386
_timer[7] = sitar::time(current_time)+sitar::time(0,1);
 _incrementPointer(30);
}

case 1:
{
if(current_time>=_timer[7])
 _incrementPointer(30);
else
 break; 
}
case 2: break;
}

}

else
{

}

if((_if_flag[15]==true && _pointer[30]>=_pointer_last_value[30]) || (_if_flag[15]==false))

{
 //if-statement has terminated
 _incrementPointer(29);
 _pointer[30]=0;

}

 else 
 //if-statement has converged
 break;
}

case 4: break;
}

if(_pointer[29]< _pointer_last_value[29])  
break; //sequence has converged  
 else 
 if(_pointer[29]==_pointer_last_value[29] && ((((!((done_push)))))==true))
 {
//re-activate the sequence	
_pointer[29]=0;	
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
 		std::cerr<<"\nERROR:Iteration limit exceeded for do-while loop on line:373 in file memorytop.sitar";
		_pointer[29]=0;                                                               
		_incrementPointer(28);                                                                    
	}                                                                                              
	else if(_pointer[29]<_pointer_last_value[29])                                    
	{                                                                                               
		//sequence just converged;                                                              
		break;                                                                                  
	}                                                                                               
	else if (_pointer[29]==_pointer_last_value[29] && ((((!((done_push)))))==false))  
	{                                                                                               
		//terminate the do-while statement                                                      
		_pointer[29]=0;                                                               
		_incrementPointer(28);                                                                    
	} ;                                                                                              
};                                                                                                   
case 2 :
{
//do-while statement , line:388
 int _dowhile_iteration;
for(_dowhile_iteration=1; _dowhile_iteration<=SITAR_ITERATION_LIMIT; _dowhile_iteration++)
{
//execute the sequence  
switch(_pointer[31])
{

case 0:
{

//wait-until statement , line:389
if(((((((current_time.phase()))==(0))))))
 _incrementPointer(31);
else
 break;
 }
case 1:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:390 ----
done_pull = bridge_collect_target_response(target_kind,
							&sp_response,
							&timer_response,
							&irc_response,
							&serial_tx_response,
							&serial_rx_response,
							&low_resp32);
//----end code block-------

 _incrementPointer(31);
}

case 2:
{

//if statement , line:397
if((((!((done_pull))))))
_if_flag[16]=true;
else
_if_flag[16]=false;
 _incrementPointer(31);
}

case 3:
{

if(_if_flag[16]==true)
{

switch(_pointer[32])
{

case 0:
{

//wait statement , line:397
_timer[8] = sitar::time(current_time)+sitar::time(0,1);
 _incrementPointer(32);
}

case 1:
{
if(current_time>=_timer[8])
 _incrementPointer(32);
else
 break; 
}
case 2: break;
}

}

else
{

}

if((_if_flag[16]==true && _pointer[32]>=_pointer_last_value[32]) || (_if_flag[16]==false))

{
 //if-statement has terminated
 _incrementPointer(31);
 _pointer[32]=0;

}

 else 
 //if-statement has converged
 break;
}

case 4: break;
}

if(_pointer[31]< _pointer_last_value[31])  
break; //sequence has converged  
 else 
 if(_pointer[31]==_pointer_last_value[31] && ((((!((done_pull)))))==true))
 {
//re-activate the sequence	
_pointer[31]=0;	
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
 		std::cerr<<"\nERROR:Iteration limit exceeded for do-while loop on line:388 in file memorytop.sitar";
		_pointer[31]=0;                                                               
		_incrementPointer(28);                                                                    
	}                                                                                              
	else if(_pointer[31]<_pointer_last_value[31])                                    
	{                                                                                               
		//sequence just converged;                                                              
		break;                                                                                  
	}                                                                                               
	else if (_pointer[31]==_pointer_last_value[31] && ((((!((done_pull)))))==false))  
	{                                                                                               
		//terminate the do-while statement                                                      
		_pointer[31]=0;                                                               
		_incrementPointer(28);                                                                    
	} ;                                                                                              
};                                                                                                   
case 3:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:399 ----
if((req_addr & 0x4) == 0) {
						high_resp32 = low_resp32;
						low_resp32 = 0;
					} else {
						high_resp32 = 0;
					}
					resp_data = (((uint64_t) high_resp32) << 32) | ((uint64_t) low_resp32);
//----end code block-------

 _incrementPointer(28);
}

case 4: break;
}

}

if((_if_flag[12]==true && _pointer[23]>=_pointer_last_value[23]) || (_if_flag[12]==false&& _pointer[28]>= _pointer_last_value[28]))

{
 //if-statement has terminated
 _incrementPointer(22);
 _pointer[23]=0;

 _pointer[28]=0;

}

 else 
 //if-statement has converged
 break;
}

case 2: break;
}

}

if((_if_flag[4]==true && _pointer[9]>=_pointer_last_value[9]) || (_if_flag[4]==false&& _pointer[22]>= _pointer_last_value[22]))

{
 //if-statement has terminated
 _incrementPointer(8);
 _pointer[9]=0;

 _pointer[22]=0;

}

 else 
 //if-statement has converged
 break;
}

case 3:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:409 ----
resp_stage = 0;
//----end code block-------

 _incrementPointer(8);
}

case 4 :
{
//do-while statement , line:410
 int _dowhile_iteration;
for(_dowhile_iteration=1; _dowhile_iteration<=SITAR_ITERATION_LIMIT; _dowhile_iteration++)
{
//execute the sequence  
switch(_pointer[33])
{

case 0:
{

//wait-until statement , line:411
if(((((((current_time.phase()))==(1))))))
 _incrementPointer(33);
else
 break;
 }
case 1:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:412 ----
done_push = bridge_send_cpu_response_step(&resp_stage, &data_out[selected_port], resp_data);
//----end code block-------

 _incrementPointer(33);
}

case 2:
{

//if statement , line:413
if((((!((done_push))))))
_if_flag[17]=true;
else
_if_flag[17]=false;
 _incrementPointer(33);
}

case 3:
{

if(_if_flag[17]==true)
{

switch(_pointer[34])
{

case 0:
{

//wait statement , line:413
_timer[9] = sitar::time(current_time)+sitar::time(0,1);
 _incrementPointer(34);
}

case 1:
{
if(current_time>=_timer[9])
 _incrementPointer(34);
else
 break; 
}
case 2: break;
}

}

else
{

}

if((_if_flag[17]==true && _pointer[34]>=_pointer_last_value[34]) || (_if_flag[17]==false))

{
 //if-statement has terminated
 _incrementPointer(33);
 _pointer[34]=0;

}

 else 
 //if-statement has converged
 break;
}

case 4: break;
}

if(_pointer[33]< _pointer_last_value[33])  
break; //sequence has converged  
 else 
 if(_pointer[33]==_pointer_last_value[33] && ((((!((done_push)))))==true))
 {
//re-activate the sequence	
_pointer[33]=0;	
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
 		std::cerr<<"\nERROR:Iteration limit exceeded for do-while loop on line:410 in file memorytop.sitar";
		_pointer[33]=0;                                                               
		_incrementPointer(8);                                                                    
	}                                                                                              
	else if(_pointer[33]<_pointer_last_value[33])                                    
	{                                                                                               
		//sequence just converged;                                                              
		break;                                                                                  
	}                                                                                               
	else if (_pointer[33]==_pointer_last_value[33] && ((((!((done_push)))))==false))  
	{                                                                                               
		//terminate the do-while statement                                                      
		_pointer[33]=0;                                                               
		_incrementPointer(8);                                                                    
	} ;                                                                                              
};                                                                                                   
case 5:
{ 
 
//code_block_statement 
//----code block from file memorytop.sitar, line:416 ----
ready_state[selected_port] = false;
			req_stage[selected_port] = 0;
			write_state[selected_port] = false;
			addr_state[selected_port] = 0;
			data_state[selected_port] = 0;
			bmask_state[selected_port] = 0xff;
			rr_start = (uint8_t) ((selected_port + 1) % PORTS);
//----end code block-------

 _incrementPointer(8);
}

case 6: break;
}

}

if((_if_flag[3]==true && _pointer[7]>=_pointer_last_value[7]) || (_if_flag[3]==false&& _pointer[8]>= _pointer_last_value[8]))

{
 //if-statement has terminated
 _incrementPointer(1);
 _pointer[7]=0;

 _pointer[8]=0;

}

 else 
 //if-statement has converged
 break;
}

case 6: break;
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
 		std::cerr<<"\nERROR:Iteration limit exceeded for do-while loop on line:233 in file memorytop.sitar";
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
			std::cerr<<"\nERROR:Iteration limit exceeded in behavior block of module Bridge.";
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
	template<int size,int DELAY,int PORTS,bool trace>
	void  Bridge<size,DELAY,PORTS,trace>::_resetBehavior()
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
