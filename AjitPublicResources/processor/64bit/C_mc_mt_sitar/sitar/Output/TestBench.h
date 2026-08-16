	
	#ifndef TESTBENCH_H
	#define TESTBENCH_H
	//======================================
	//file TestBench.h                                                  
	//Describes module TestBench                                      
	//Auto-generated from input file "cop.sitar" on 2026-7-19 at time 22:6:27   
	
	//(This design unit is not parameterized. Generating code into .h and .cpp files) 
	//======================================
	#include<iostream>
	#include<iomanip>

	//sitar core classes
	#include"sitar_module.h"
	#include"sitar_time.h"
	//user included files
	 
//----code block from file cop.sitar, line:16 ----
#include <inttypes.h>
//----end code block-------

//----code block from file cop.sitar, line:17 ----
extern "C" {
		extern bool extract_bracket_number(const char *src, int *out_num);
		extern void ajit_memory_shim_set_ports(int id,
						      void* act_port,
						      void* wr_port,
						      void* addr_port,
						      void* data_in_port,
						      void* data_out_port,
						      void* bm_port,
					      void* irq_port,
					      void* coh_fill_kind_port,
					      void* coh_fill_pa_line_port,
					      void* coh_fill_va_line_port,
					      void* coh_icache_inval_port,
					      void* coh_dcache_inval_port);
	}
	
//----end code block-------

//----code block from file cop.sitar, line:34 ----
#include "ajit_tb_driver.h"
//----end code block-------


	namespace sitar{

	
	class TestBench:public module
	{

		public:
			//constructor
			TestBench();
	 
			void runBehavior(const time& current_time);
			inline void _incrementPointer(unsigned int ptr_number){_pointer[ptr_number]++; _reexecute=true;}
			void _resetBehavior();

			
			//Auto-generated variables for recording state in behavior block
			private:
			static const unsigned int _num_pointers=2; //total pointers used by this module
			static const unsigned int _num_timers=1;  	//total timers used by this module
			static const unsigned int _num_if_flags=0; //total if_flags used
			
			//State holders 
			//the +1 for array size is to avoid zero-sized arrays
			unsigned int 	_pointer[_num_pointers+1];		//pointers for sequences
			time 	_timer[_num_timers+1];			//timers for wait statements
			bool		_if_flag[_num_if_flags+1];		//if_flags
			unsigned int    _pointer_last_value[_num_pointers+1];	//last value taken by each sequence pointer

	
			//other declarations
			public:
			 
inport<64> data_in;
inport<8> irq_level;
outport<1> active;
outport<1> write;
outport<32> addr_out;
outport<64> data_out;
outport<8> byte_mask;
outport<8> coh_fill_kind;
outport<32> coh_fill_pa_line;
outport<32> coh_fill_va_line;
inport<32> coh_icache_inval;
inport<32> coh_dcache_inval;
//----code block from file cop.sitar, line:36 ----
AjitTbDriver* module;
//----end code block-------

//----code block from file cop.sitar, line:37 ----
int instance_number;
//----end code block-------

//----code block from file cop.sitar, line:38 ----
bool extract_success, state = false;
//----end code block-------

//----code block from file cop.sitar, line:39 ----
std::string lf;
//----end code block-------


	};
	}
	//==========================================


	 

			
	#endif
