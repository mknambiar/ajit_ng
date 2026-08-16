	
	#ifndef TOP_H
	#define TOP_H
	//======================================
	//file Top.h                                                  
	//Describes module Top                                      
	//Auto-generated from input file "memorytop.sitar" on 2026-7-19 at time 22:6:27   
	 //(This design unit is parameterized. Generating code into a .h file only) 
	
	//======================================
	#include<iostream>
	#include<iomanip>

	//sitar core classes
	#include"sitar_module.h"
	#include"sitar_time.h"
	//user included files
	 
#include"Bridge.h"
#include"L2Cache.h"
#include"Memory.h"
#include"ScratchPad.h"
#include"Timer.h"
#include"InterruptController.h"
#include"SerialTx.h"
#include"SerialRx.h"
#include"ConsoleInput.h"
#include"ConsoleOutput.h"
#include"TestBench.h"

	namespace sitar{

	template<int numT=4>
	class Top:public module
	{

		public:
			//constructor
			Top();
	
			//other declarations
			public:
			 
Bridge<32, 5, numT, true> B;
L2Cache<true> L2;
Memory<32, 5, true> M;
ScratchPad<true> SP;
Timer<true> TIM;
InterruptController<numT, true> IRC;
SerialTx<true> STX;
SerialRx<true> SRX;
ConsoleInput<true> CIN;
ConsoleOutput<true> COUT;
TestBench T[numT];
net<1> active[numT];
token<1> active_buffer[numT][1];
net<1> write[numT];
token<1> write_buffer[numT][1];
net<32> addr[numT];
token<32> addr_buffer[numT][1];
net<64> data_to_bridge[numT];
token<64> data_to_bridge_buffer[numT][1];
net<64> data_from_bridge[numT];
token<64> data_from_bridge_buffer[numT][1];
net<8> bm[numT];
token<8> bm_buffer[numT][1];
net<8> irl[numT];
token<8> irl_buffer[numT][1];
net<8> coh_fill_kind[numT];
token<8> coh_fill_kind_buffer[numT][1];
net<32> coh_fill_pa_line[numT];
token<32> coh_fill_pa_line_buffer[numT][1];
net<32> coh_fill_va_line[numT];
token<32> coh_fill_va_line_buffer[numT][1];
net<32> coh_icache_inval[numT];
token<32> coh_icache_inval_buffer[numT][64];
net<32> coh_dcache_inval[numT];
token<32> coh_dcache_inval_buffer[numT][64];
net<1> bridge_to_mem_active;
token<1> bridge_to_mem_active_buffer[1];
net<1> bridge_to_mem_write;
token<1> bridge_to_mem_write_buffer[1];
net<32> bridge_to_mem_addr;
token<32> bridge_to_mem_addr_buffer[1];
net<64> bridge_to_mem_data;
token<64> bridge_to_mem_data_buffer[1];
net<8> bridge_to_mem_bm;
token<8> bridge_to_mem_bm_buffer[1];
net<64> mem_to_bridge_data;
token<64> mem_to_bridge_data_buffer[1];
net<1> l2_to_mem_active;
token<1> l2_to_mem_active_buffer[1];
net<1> l2_to_mem_write;
token<1> l2_to_mem_write_buffer[1];
net<32> l2_to_mem_addr;
token<32> l2_to_mem_addr_buffer[1];
net<64> l2_to_mem_data;
token<64> l2_to_mem_data_buffer[1];
net<8> l2_to_mem_bm;
token<8> l2_to_mem_bm_buffer[1];
net<64> mem_to_l2_data;
token<64> mem_to_l2_data_buffer[1];
net<64> bridge_to_sp_request;
token<64> bridge_to_sp_request_buffer[1];
net<32> sp_to_bridge_response;
token<32> sp_to_bridge_response_buffer[1];
net<64> bridge_to_tim_request;
token<64> bridge_to_tim_request_buffer[1];
net<32> tim_to_bridge_response;
token<32> tim_to_bridge_response_buffer[1];
net<64> bridge_to_irc_request;
token<64> bridge_to_irc_request_buffer[1];
net<32> irc_to_bridge_response;
token<32> irc_to_bridge_response_buffer[1];
net<64> bridge_to_stx_request;
token<64> bridge_to_stx_request_buffer[1];
net<32> stx_to_bridge_response;
token<32> stx_to_bridge_response_buffer[1];
net<64> bridge_to_srx_request;
token<64> bridge_to_srx_request_buffer[1];
net<32> srx_to_bridge_response;
token<32> srx_to_bridge_response_buffer[1];
net<8> timer_irq;
token<8> timer_irq_buffer[1];
net<8> serial_rx_irq;
token<8> serial_rx_irq_buffer[1];
net<8> serial_control_word;
token<8> serial_control_word_buffer[1];
net<8> serial_rx_full_status;
token<8> serial_rx_full_status_buffer[1];
net<8> serial_tx_byte;
token<8> serial_tx_byte_buffer[1];
net<8> serial_tx_valid;
token<8> serial_tx_valid_buffer[1];
net<8> console_tx_ack;
token<8> console_tx_ack_buffer[1];
net<8> console_rx_byte;
token<8> console_rx_byte_buffer[1];
net<8> console_rx_valid;
token<8> console_rx_valid_buffer[1];
net<8> serial_rx_ack;
token<8> serial_rx_ack_buffer[1];

	};
	}
	//==========================================


	
	namespace sitar{
	//Constructor
	template<int numT>
	Top<numT>::Top()
	{
		using std::cout;
		_type="Top";
	
		 
//---Initializing submodule B---
B.setInstanceId("B");
addSubmodule(&B,"B");

//---Initializing submodule L2---
L2.setInstanceId("L2");
addSubmodule(&L2,"L2");

//---Initializing submodule M---
M.setInstanceId("M");
addSubmodule(&M,"M");

//---Initializing submodule SP---
SP.setInstanceId("SP");
addSubmodule(&SP,"SP");

//---Initializing submodule TIM---
TIM.setInstanceId("TIM");
addSubmodule(&TIM,"TIM");

//---Initializing submodule IRC---
IRC.setInstanceId("IRC");
addSubmodule(&IRC,"IRC");

//---Initializing submodule STX---
STX.setInstanceId("STX");
addSubmodule(&STX,"STX");

//---Initializing submodule SRX---
SRX.setInstanceId("SRX");
addSubmodule(&SRX,"SRX");

//---Initializing submodule CIN---
CIN.setInstanceId("CIN");
addSubmodule(&CIN,"CIN");

//---Initializing submodule COUT---
COUT.setInstanceId("COUT");
addSubmodule(&COUT,"COUT");

//----Initializing module-array TestBench------
for(int i=0;i<(numT);i++)
{

T[i].setInstanceId("T["+sitar::toString(i)+"]");
addSubmodule(&T[i],"T["+sitar::toString(i)+"]");

}

//----Initializing net-array active------
for(int i=0;i<(numT);i++)
{

std::string nname="active["+sitar::toString(i)+"]";
active[i].setInstanceId(nname);
active[i].setBuffer(active_buffer[i],1);
addNet(&active[i],"active["+sitar::toString(i)+"]");

}

//----Initializing net-array write------
for(int i=0;i<(numT);i++)
{

std::string nname="write["+sitar::toString(i)+"]";
write[i].setInstanceId(nname);
write[i].setBuffer(write_buffer[i],1);
addNet(&write[i],"write["+sitar::toString(i)+"]");

}

//----Initializing net-array addr------
for(int i=0;i<(numT);i++)
{

std::string nname="addr["+sitar::toString(i)+"]";
addr[i].setInstanceId(nname);
addr[i].setBuffer(addr_buffer[i],1);
addNet(&addr[i],"addr["+sitar::toString(i)+"]");

}

//----Initializing net-array data_to_bridge------
for(int i=0;i<(numT);i++)
{

std::string nname="data_to_bridge["+sitar::toString(i)+"]";
data_to_bridge[i].setInstanceId(nname);
data_to_bridge[i].setBuffer(data_to_bridge_buffer[i],1);
addNet(&data_to_bridge[i],"data_to_bridge["+sitar::toString(i)+"]");

}

//----Initializing net-array data_from_bridge------
for(int i=0;i<(numT);i++)
{

std::string nname="data_from_bridge["+sitar::toString(i)+"]";
data_from_bridge[i].setInstanceId(nname);
data_from_bridge[i].setBuffer(data_from_bridge_buffer[i],1);
addNet(&data_from_bridge[i],"data_from_bridge["+sitar::toString(i)+"]");

}

//----Initializing net-array bm------
for(int i=0;i<(numT);i++)
{

std::string nname="bm["+sitar::toString(i)+"]";
bm[i].setInstanceId(nname);
bm[i].setBuffer(bm_buffer[i],1);
addNet(&bm[i],"bm["+sitar::toString(i)+"]");

}

//----Initializing net-array irl------
for(int i=0;i<(numT);i++)
{

std::string nname="irl["+sitar::toString(i)+"]";
irl[i].setInstanceId(nname);
irl[i].setBuffer(irl_buffer[i],1);
addNet(&irl[i],"irl["+sitar::toString(i)+"]");

}

//----Initializing net-array coh_fill_kind------
for(int i=0;i<(numT);i++)
{

std::string nname="coh_fill_kind["+sitar::toString(i)+"]";
coh_fill_kind[i].setInstanceId(nname);
coh_fill_kind[i].setBuffer(coh_fill_kind_buffer[i],1);
addNet(&coh_fill_kind[i],"coh_fill_kind["+sitar::toString(i)+"]");

}

//----Initializing net-array coh_fill_pa_line------
for(int i=0;i<(numT);i++)
{

std::string nname="coh_fill_pa_line["+sitar::toString(i)+"]";
coh_fill_pa_line[i].setInstanceId(nname);
coh_fill_pa_line[i].setBuffer(coh_fill_pa_line_buffer[i],1);
addNet(&coh_fill_pa_line[i],"coh_fill_pa_line["+sitar::toString(i)+"]");

}

//----Initializing net-array coh_fill_va_line------
for(int i=0;i<(numT);i++)
{

std::string nname="coh_fill_va_line["+sitar::toString(i)+"]";
coh_fill_va_line[i].setInstanceId(nname);
coh_fill_va_line[i].setBuffer(coh_fill_va_line_buffer[i],1);
addNet(&coh_fill_va_line[i],"coh_fill_va_line["+sitar::toString(i)+"]");

}

//----Initializing net-array coh_icache_inval------
for(int i=0;i<(numT);i++)
{

std::string nname="coh_icache_inval["+sitar::toString(i)+"]";
coh_icache_inval[i].setInstanceId(nname);
coh_icache_inval[i].setBuffer(coh_icache_inval_buffer[i],64);
addNet(&coh_icache_inval[i],"coh_icache_inval["+sitar::toString(i)+"]");

}

//----Initializing net-array coh_dcache_inval------
for(int i=0;i<(numT);i++)
{

std::string nname="coh_dcache_inval["+sitar::toString(i)+"]";
coh_dcache_inval[i].setInstanceId(nname);
coh_dcache_inval[i].setBuffer(coh_dcache_inval_buffer[i],64);
addNet(&coh_dcache_inval[i],"coh_dcache_inval["+sitar::toString(i)+"]");

}

//---Initializing net bridge_to_mem_active---
bridge_to_mem_active.setInstanceId("bridge_to_mem_active");
bridge_to_mem_active.setBuffer(bridge_to_mem_active_buffer,1);
addNet(&bridge_to_mem_active,"bridge_to_mem_active");

//---Initializing net bridge_to_mem_write---
bridge_to_mem_write.setInstanceId("bridge_to_mem_write");
bridge_to_mem_write.setBuffer(bridge_to_mem_write_buffer,1);
addNet(&bridge_to_mem_write,"bridge_to_mem_write");

//---Initializing net bridge_to_mem_addr---
bridge_to_mem_addr.setInstanceId("bridge_to_mem_addr");
bridge_to_mem_addr.setBuffer(bridge_to_mem_addr_buffer,1);
addNet(&bridge_to_mem_addr,"bridge_to_mem_addr");

//---Initializing net bridge_to_mem_data---
bridge_to_mem_data.setInstanceId("bridge_to_mem_data");
bridge_to_mem_data.setBuffer(bridge_to_mem_data_buffer,1);
addNet(&bridge_to_mem_data,"bridge_to_mem_data");

//---Initializing net bridge_to_mem_bm---
bridge_to_mem_bm.setInstanceId("bridge_to_mem_bm");
bridge_to_mem_bm.setBuffer(bridge_to_mem_bm_buffer,1);
addNet(&bridge_to_mem_bm,"bridge_to_mem_bm");

//---Initializing net mem_to_bridge_data---
mem_to_bridge_data.setInstanceId("mem_to_bridge_data");
mem_to_bridge_data.setBuffer(mem_to_bridge_data_buffer,1);
addNet(&mem_to_bridge_data,"mem_to_bridge_data");

//---Initializing net l2_to_mem_active---
l2_to_mem_active.setInstanceId("l2_to_mem_active");
l2_to_mem_active.setBuffer(l2_to_mem_active_buffer,1);
addNet(&l2_to_mem_active,"l2_to_mem_active");

//---Initializing net l2_to_mem_write---
l2_to_mem_write.setInstanceId("l2_to_mem_write");
l2_to_mem_write.setBuffer(l2_to_mem_write_buffer,1);
addNet(&l2_to_mem_write,"l2_to_mem_write");

//---Initializing net l2_to_mem_addr---
l2_to_mem_addr.setInstanceId("l2_to_mem_addr");
l2_to_mem_addr.setBuffer(l2_to_mem_addr_buffer,1);
addNet(&l2_to_mem_addr,"l2_to_mem_addr");

//---Initializing net l2_to_mem_data---
l2_to_mem_data.setInstanceId("l2_to_mem_data");
l2_to_mem_data.setBuffer(l2_to_mem_data_buffer,1);
addNet(&l2_to_mem_data,"l2_to_mem_data");

//---Initializing net l2_to_mem_bm---
l2_to_mem_bm.setInstanceId("l2_to_mem_bm");
l2_to_mem_bm.setBuffer(l2_to_mem_bm_buffer,1);
addNet(&l2_to_mem_bm,"l2_to_mem_bm");

//---Initializing net mem_to_l2_data---
mem_to_l2_data.setInstanceId("mem_to_l2_data");
mem_to_l2_data.setBuffer(mem_to_l2_data_buffer,1);
addNet(&mem_to_l2_data,"mem_to_l2_data");

//---Initializing net bridge_to_sp_request---
bridge_to_sp_request.setInstanceId("bridge_to_sp_request");
bridge_to_sp_request.setBuffer(bridge_to_sp_request_buffer,1);
addNet(&bridge_to_sp_request,"bridge_to_sp_request");

//---Initializing net sp_to_bridge_response---
sp_to_bridge_response.setInstanceId("sp_to_bridge_response");
sp_to_bridge_response.setBuffer(sp_to_bridge_response_buffer,1);
addNet(&sp_to_bridge_response,"sp_to_bridge_response");

//---Initializing net bridge_to_tim_request---
bridge_to_tim_request.setInstanceId("bridge_to_tim_request");
bridge_to_tim_request.setBuffer(bridge_to_tim_request_buffer,1);
addNet(&bridge_to_tim_request,"bridge_to_tim_request");

//---Initializing net tim_to_bridge_response---
tim_to_bridge_response.setInstanceId("tim_to_bridge_response");
tim_to_bridge_response.setBuffer(tim_to_bridge_response_buffer,1);
addNet(&tim_to_bridge_response,"tim_to_bridge_response");

//---Initializing net bridge_to_irc_request---
bridge_to_irc_request.setInstanceId("bridge_to_irc_request");
bridge_to_irc_request.setBuffer(bridge_to_irc_request_buffer,1);
addNet(&bridge_to_irc_request,"bridge_to_irc_request");

//---Initializing net irc_to_bridge_response---
irc_to_bridge_response.setInstanceId("irc_to_bridge_response");
irc_to_bridge_response.setBuffer(irc_to_bridge_response_buffer,1);
addNet(&irc_to_bridge_response,"irc_to_bridge_response");

//---Initializing net bridge_to_stx_request---
bridge_to_stx_request.setInstanceId("bridge_to_stx_request");
bridge_to_stx_request.setBuffer(bridge_to_stx_request_buffer,1);
addNet(&bridge_to_stx_request,"bridge_to_stx_request");

//---Initializing net stx_to_bridge_response---
stx_to_bridge_response.setInstanceId("stx_to_bridge_response");
stx_to_bridge_response.setBuffer(stx_to_bridge_response_buffer,1);
addNet(&stx_to_bridge_response,"stx_to_bridge_response");

//---Initializing net bridge_to_srx_request---
bridge_to_srx_request.setInstanceId("bridge_to_srx_request");
bridge_to_srx_request.setBuffer(bridge_to_srx_request_buffer,1);
addNet(&bridge_to_srx_request,"bridge_to_srx_request");

//---Initializing net srx_to_bridge_response---
srx_to_bridge_response.setInstanceId("srx_to_bridge_response");
srx_to_bridge_response.setBuffer(srx_to_bridge_response_buffer,1);
addNet(&srx_to_bridge_response,"srx_to_bridge_response");

//---Initializing net timer_irq---
timer_irq.setInstanceId("timer_irq");
timer_irq.setBuffer(timer_irq_buffer,1);
addNet(&timer_irq,"timer_irq");

//---Initializing net serial_rx_irq---
serial_rx_irq.setInstanceId("serial_rx_irq");
serial_rx_irq.setBuffer(serial_rx_irq_buffer,1);
addNet(&serial_rx_irq,"serial_rx_irq");

//---Initializing net serial_control_word---
serial_control_word.setInstanceId("serial_control_word");
serial_control_word.setBuffer(serial_control_word_buffer,1);
addNet(&serial_control_word,"serial_control_word");

//---Initializing net serial_rx_full_status---
serial_rx_full_status.setInstanceId("serial_rx_full_status");
serial_rx_full_status.setBuffer(serial_rx_full_status_buffer,1);
addNet(&serial_rx_full_status,"serial_rx_full_status");

//---Initializing net serial_tx_byte---
serial_tx_byte.setInstanceId("serial_tx_byte");
serial_tx_byte.setBuffer(serial_tx_byte_buffer,1);
addNet(&serial_tx_byte,"serial_tx_byte");

//---Initializing net serial_tx_valid---
serial_tx_valid.setInstanceId("serial_tx_valid");
serial_tx_valid.setBuffer(serial_tx_valid_buffer,1);
addNet(&serial_tx_valid,"serial_tx_valid");

//---Initializing net console_tx_ack---
console_tx_ack.setInstanceId("console_tx_ack");
console_tx_ack.setBuffer(console_tx_ack_buffer,1);
addNet(&console_tx_ack,"console_tx_ack");

//---Initializing net console_rx_byte---
console_rx_byte.setInstanceId("console_rx_byte");
console_rx_byte.setBuffer(console_rx_byte_buffer,1);
addNet(&console_rx_byte,"console_rx_byte");

//---Initializing net console_rx_valid---
console_rx_valid.setInstanceId("console_rx_valid");
console_rx_valid.setBuffer(console_rx_valid_buffer,1);
addNet(&console_rx_valid,"console_rx_valid");

//---Initializing net serial_rx_ack---
serial_rx_ack.setInstanceId("serial_rx_ack");
serial_rx_ack.setBuffer(serial_rx_ack_buffer,1);
addNet(&serial_rx_ack,"serial_rx_ack");

//---connecting ports to nets----
for(int i=(0);i<=((numT - 1));i++)
{


T[i].active.setNet(&active[i]);
T[i].write.setNet(&write[i]);
T[i].addr_out.setNet(&addr[i]);
T[i].data_out.setNet(&data_to_bridge[i]);
T[i].byte_mask.setNet(&bm[i]);
T[i].irq_level.setNet(&irl[i]);
T[i].data_in.setNet(&data_from_bridge[i]);
T[i].coh_fill_kind.setNet(&coh_fill_kind[i]);
T[i].coh_fill_pa_line.setNet(&coh_fill_pa_line[i]);
T[i].coh_fill_va_line.setNet(&coh_fill_va_line[i]);
T[i].coh_icache_inval.setNet(&coh_icache_inval[i]);
T[i].coh_dcache_inval.setNet(&coh_dcache_inval[i]);
B.active[i].setNet(&active[i]);
B.write[i].setNet(&write[i]);
B.addr_in[i].setNet(&addr[i]);
B.data_in[i].setNet(&data_to_bridge[i]);
B.byte_mask[i].setNet(&bm[i]);
B.data_out[i].setNet(&data_from_bridge[i]);
B.coh_fill_kind[i].setNet(&coh_fill_kind[i]);
B.coh_fill_pa_line[i].setNet(&coh_fill_pa_line[i]);
B.coh_fill_va_line[i].setNet(&coh_fill_va_line[i]);
B.coh_icache_inval[i].setNet(&coh_icache_inval[i]);
B.coh_dcache_inval[i].setNet(&coh_dcache_inval[i]);
IRC.thread_irl[i].setNet(&irl[i]);

};

B.mem_active.setNet(&bridge_to_mem_active);
B.mem_write.setNet(&bridge_to_mem_write);
B.mem_addr_out.setNet(&bridge_to_mem_addr);
B.mem_data_out.setNet(&bridge_to_mem_data);
B.mem_byte_mask.setNet(&bridge_to_mem_bm);
B.mem_data_in.setNet(&mem_to_bridge_data);
B.sp_request.setNet(&bridge_to_sp_request);
B.sp_response.setNet(&sp_to_bridge_response);
B.timer_request.setNet(&bridge_to_tim_request);
B.timer_response.setNet(&tim_to_bridge_response);
B.irc_request.setNet(&bridge_to_irc_request);
B.irc_response.setNet(&irc_to_bridge_response);
B.serial_tx_request.setNet(&bridge_to_stx_request);
B.serial_tx_response.setNet(&stx_to_bridge_response);
B.serial_rx_request.setNet(&bridge_to_srx_request);
B.serial_rx_response.setNet(&srx_to_bridge_response);
L2.active.setNet(&bridge_to_mem_active);
L2.write.setNet(&bridge_to_mem_write);
L2.addr_in.setNet(&bridge_to_mem_addr);
L2.data_in.setNet(&bridge_to_mem_data);
L2.byte_mask.setNet(&bridge_to_mem_bm);
L2.data_out.setNet(&mem_to_bridge_data);
L2.mem_active.setNet(&l2_to_mem_active);
L2.mem_write.setNet(&l2_to_mem_write);
L2.mem_addr_out.setNet(&l2_to_mem_addr);
L2.mem_data_out.setNet(&l2_to_mem_data);
L2.mem_byte_mask.setNet(&l2_to_mem_bm);
L2.mem_data_in.setNet(&mem_to_l2_data);
M.active.setNet(&l2_to_mem_active);
M.write.setNet(&l2_to_mem_write);
M.addr_in.setNet(&l2_to_mem_addr);
M.data_in.setNet(&l2_to_mem_data);
M.byte_mask.setNet(&l2_to_mem_bm);
M.data_out.setNet(&mem_to_l2_data);
SP.request.setNet(&bridge_to_sp_request);
SP.response.setNet(&sp_to_bridge_response);
TIM.request.setNet(&bridge_to_tim_request);
TIM.response.setNet(&tim_to_bridge_response);
TIM.irq_level.setNet(&timer_irq);
IRC.request.setNet(&bridge_to_irc_request);
IRC.response.setNet(&irc_to_bridge_response);
IRC.timer_irq_level.setNet(&timer_irq);
IRC.serial_rx_irq_level.setNet(&serial_rx_irq);
STX.request.setNet(&bridge_to_stx_request);
STX.response.setNet(&stx_to_bridge_response);
STX.control_word.setNet(&serial_control_word);
STX.rx_full_status.setNet(&serial_rx_full_status);
STX.console_tx_data.setNet(&serial_tx_byte);
STX.console_tx_valid.setNet(&serial_tx_valid);
STX.console_tx_ack.setNet(&console_tx_ack);
SRX.request.setNet(&bridge_to_srx_request);
SRX.response.setNet(&srx_to_bridge_response);
SRX.irq_level.setNet(&serial_rx_irq);
SRX.control_word.setNet(&serial_control_word);
SRX.rx_full_status.setNet(&serial_rx_full_status);
SRX.console_rx_data.setNet(&console_rx_byte);
SRX.console_rx_valid.setNet(&console_rx_valid);
SRX.console_rx_ack.setNet(&serial_rx_ack);
CIN.console_rx_data.setNet(&console_rx_byte);
CIN.console_rx_valid.setNet(&console_rx_valid);
CIN.console_rx_ack.setNet(&serial_rx_ack);
CIN.control_word.setNet(&serial_control_word);
COUT.console_tx_data.setNet(&serial_tx_byte);
COUT.console_tx_valid.setNet(&serial_tx_valid);
COUT.console_tx_ack.setNet(&console_tx_ack);
	}





	
	}

			
	#endif
