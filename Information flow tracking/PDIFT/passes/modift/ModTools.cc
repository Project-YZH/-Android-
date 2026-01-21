/*
 *  yosys -- Yosys Open SYnthesis Suite
 *
 *  Copyright (C) 2012  Claire Xenia Wolf <claire@yosyshq.com>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
#include "kernel/rtlil.h"
#include "kernel/sigtools.h"
#include "kernel/celltypes.h"
#include "kernel/macc.h"
#include <stdio.h>

YOSYS_NAMESPACE_BEGIN



struct DesignMap
{
	RTLIL::Module *module;
	SigMap assign_map;
	SigMap values_map;
	SigPool stop_signals;
	SigSet<RTLIL::Cell*> sig2driver_out;
	SigSet<RTLIL::Cell*> sig2driver_in;
	std::set<RTLIL::SigSpec> sig_in;
	std::set<RTLIL::SigSpec> sig_out;
	std::set<RTLIL::Cell*> busy;
	std::vector<SigMap> stack;
	RTLIL::State defaultval;

    

	DesignMap(RTLIL::Module *module, RTLIL::State defaultval = RTLIL::State::Sm) : module(module), assign_map(module), defaultval(defaultval)
	{
		CellTypes ct;
		ct.setup_internals();
		ct.setup_stdcells();

		for (auto &it : module->cells_) {
			if (!ct.cell_known(it.second->type))
				continue;
			for (auto &it2 : it.second->connections())
			{
				if (ct.cell_output(it.second->type, it2.first))
					{
						sig2driver_out.insert(assign_map(it2.second), it.second);
						sig_out.insert(it2.second);
					}
				else if(ct.cell_input(it.second->type, it2.first))
					{
						sig2driver_in.insert(assign_map(it2.second), it.second);
						sig_in.insert(it2.second);
					}
			}
		}
	}
	
	//iterate through all modules and add logic to end of execution unit
	/*
	RTLIL::Wire *add_mod_end_flag(RTLIL::Module *module, RTLIL::Cell *cell, RTLIL::SigSpec sig, RTLIL::SigSpec set)
	{
		
		RTLIL::SigSpec out_old;
		RTLIL::SigSpec out_new = sig;
		RTLIL::SigSpec compare;
		RTLIL::Wire *tag;// mod exed or not
		module->addDff(NEW_ID,set,out_new,out_old);
		module->addEq(NEW_ID,out_new,out_old,tag);
		module->addNot(NEW_ID,compare,tag);

		return tag;
	}
	*/

	//iterate through all modules, collect units and signals of start and end of modulesk [using celltype structure]
	void edge_of_mod(RTLIL::Module *module)
	{
		std::set<RTLIL::Cell*> mod_end_cell;
		std::set<RTLIL::Cell*> mod_start_cell;
		std::map<RTLIL::Module *, RTLIL::SigSpec> mod_end_sig;
		std::map<RTLIL::Module *, RTLIL::SigSpec> mod_start_sig;
		
		for(auto &it : module->cells_)
		for(auto &sig : it.second->connections_)
		{

			if((sig_in.find(sig.second)!= sig_in.end()) && (sig_out.find(sig.second)!= sig_out.end()))
				continue;
			if((sig_in.find(sig.second)!= sig_in.end()) && (sig_out.find(sig.second) == sig_out.end()))
			{	
				mod_start_cell.insert(it.second);
				mod_start_sig.insert(pair<RTLIL::Module *,RTLIL::SigSpec>(module,sig.second));
			}
			if((sig_out.find(sig.second) != sig_out.end()) && (sig_in.find(sig.second) == sig_in.end()))
			{
				mod_start_cell.insert(it.second);
				mod_start_sig.insert(pair<RTLIL::Module *,RTLIL::SigSpec>(module,sig.second));
			}	
		}




	}
	

	void clear()
	{
		values_map.clear();
		stop_signals.clear();
	}

	void push()
	{
		stack.push_back(values_map);
	}

	void pop()
	{
		values_map.swap(stack.back());
		stack.pop_back();
	}

	void set(RTLIL::SigSpec sig, RTLIL::Const value)
	{
		assign_map.apply(sig);
#ifndef NDEBUG
		RTLIL::SigSpec current_val = values_map(sig);
		for (int i = 0; i < GetSize(current_val); i++)
			log_assert(current_val[i].wire != NULL || current_val[i] == value.bits[i]);
#endif
		values_map.add(sig, RTLIL::SigSpec(value));
	}

	void stop(RTLIL::SigSpec sig)
	{
		assign_map.apply(sig);
		stop_signals.add(sig);
	}

	/*
    const RTLIL::IdString bit_taint = ID(taint4bit);
	bool set_taints(RTLIL::SigSpec sig, string str)
	{	
		if(sig.width_ != str.size() && str.size() != 1)
			return false;

		if(str.size() == 1)
			for(int i = 0; i < GetSize(sig); i++)
				if(atoi(str.c_str()))
					sig.bits_[i].set_bool_attribute(bit_taint, true);
				else
					sig.bits_[i].set_bool_attribute(bit_taint, false);
		else
			for(int i = 0; i < GetSize(sig); i++)
				if(atoi(str.substr(i,1).c_str()))
					sig.bits_[i].set_bool_attribute(bit_taint, true);
				else
					sig.bits_[i].set_bool_attribute(bit_taint, false);
		
	}
    */


   	//finding the edge of Module from scrtach, without any help from other structure.
	const RTLIL::IdString start_of_mod = ID(start_of_mod);
	const RTLIL::IdString end_of_mod = ID(end_of_mod);

	void find_edge_of_MoD(  RTLIL::Module *module)
	{
    	SigMap SigMap(module);
		for(auto cell : module->cells_)
		for(auto sig : cell.second->connections_)
		for(auto sigsig : module->connections_)
		{	
			
			string cell_name = cell.first.c_str();
			RTLIL::SigSpec Sig = SigMap(sig.second);
			string Sigin = log_signal(sigsig.second);
			string Sigout = log_signal(sigsig.first);
		
			if(sig.second != sigsig.first && sig.second != sigsig.second)
				continue;
			if((sig.second == sigsig.first) && sigsig.second.is_wire())
				cell.second->set_bool_attribute(end_of_mod, true);
			if((sig.second == sigsig.second) && sigsig.second.is_wire())
				cell.second->set_bool_attribute(start_of_mod, true);
		}

		printf("===============================");
		for(auto wire : module->wires_)
			string a = wire.first.c_str();
	}


	int index;
	int index_1;
	string cell_exe;
	string mod_exe;
	string path_cell;
	string path_mod;
	vector<string> path_cell_all;
	vector<string> path_mod_all;
	const RTLIL::IdString mod_name = ID(mod_name);
	const RTLIL::IdString mod_type = ID(mod_type);
	

	bool traceCell(RTLIL::Cell *cell, RTLIL::SigSpec &undef)
	{
		if (cell->type == ID($lcu))
		{
			RTLIL::SigSpec sig_p = cell->getPort(ID::P);
			RTLIL::SigSpec sig_g = cell->getPort(ID::G);
			RTLIL::SigSpec sig_ci = cell->getPort(ID::CI);
			RTLIL::SigSpec sig_co = values_map(assign_map(cell->getPort(ID::CO)));

			if (sig_co.is_fully_const())
				return true;

			if (!traceSig(sig_p, undef, cell))
				return false;

			if (!traceSig(sig_g, undef, cell))
				return false;

			if (!traceSig(sig_ci, undef, cell))
				return false;

			if (sig_p.is_fully_def() && sig_g.is_fully_def() && sig_ci.is_fully_def())
			{
				RTLIL::Const coval(RTLIL::Sx, GetSize(sig_co));
				bool carry = sig_ci.as_bool();

				for (int i = 0; i < GetSize(coval); i++) {
					carry = (sig_g[i] == State::S1) || (sig_p[i] == RTLIL::S1 && carry);
					coval.bits[i] = carry ? State::S1 : State::S0;
				}

				set(sig_co, coval);
			}
			else
				set(sig_co, RTLIL::Const(RTLIL::Sx, GetSize(sig_co)));

			return true;
		}

		RTLIL::SigSpec sig_a, sig_b, sig_s, sig_y;

		log_assert(cell->hasPort(ID::Y));
		sig_y = values_map(assign_map(cell->getPort(ID::Y)));
		if (sig_y.is_fully_const())
			return true;

		if (cell->hasPort(ID::S)) {
			sig_s = cell->getPort(ID::S);
		}

		if (cell->hasPort(ID::A))
			sig_a = cell->getPort(ID::A);

		if (cell->hasPort(ID::B))
			sig_b = cell->getPort(ID::B);

		if (cell->type.in(ID($mux), ID($pmux), ID($_MUX_), ID($_NMUX_)))
		{
			std::vector<RTLIL::SigSpec> y_candidates;
			int count_set_s_bits = 0;

			if (!traceSig(sig_s, undef, cell))
				return false;

			for (int i = 0; i < sig_s.size(); i++)
			{
				RTLIL::State s_bit = sig_s.extract(i, 1).as_const().bits.at(0);
				RTLIL::SigSpec b_slice = sig_b.extract(sig_y.size()*i, sig_y.size());

				if (s_bit == RTLIL::State::Sx || s_bit == RTLIL::State::S1)
					y_candidates.push_back(b_slice);

				if (s_bit == RTLIL::State::S1)
					count_set_s_bits++;
			}

			if (count_set_s_bits == 0)
				y_candidates.push_back(sig_a);

			std::vector<RTLIL::Const> y_values;

			log_assert(y_candidates.size() > 0);
			for (auto &yc : y_candidates) {
				if (!traceSig(yc, undef, cell))
					return false;
				if (cell->type == ID($_NMUX_))
					y_values.push_back(RTLIL::const_not(yc.as_const(), Const(), false, false, GetSize(yc)));
				else
					y_values.push_back(yc.as_const());
			}

			if (y_values.size() > 1)
			{
				std::vector<RTLIL::State> master_bits = y_values.at(0).bits;

				for (size_t i = 1; i < y_values.size(); i++) {
					std::vector<RTLIL::State> &slave_bits = y_values.at(i).bits;
					log_assert(master_bits.size() == slave_bits.size());
					for (size_t j = 0; j < master_bits.size(); j++)
						if (master_bits[j] != slave_bits[j])
							master_bits[j] = RTLIL::State::Sx;
				}

				set(sig_y, RTLIL::Const(master_bits));
			}
			else
				set(sig_y, y_values.front());
		}
		else if (cell->type == ID($bmux))
		{
			if (!traceSig(sig_s, undef, cell))
				return false;

			if (sig_s.is_fully_def()) {
				int sel = sig_s.as_int();
				int width = GetSize(sig_y);
				SigSpec res = sig_a.extract(sel * width, width);
				if (!traceSig(res, undef, cell))
					return false;
				set(sig_y, res.as_const());
			} else {
				if (!traceSig(sig_a, undef, cell))
					return false;
				set(sig_y, const_bmux(sig_a.as_const(), sig_s.as_const()));
			}
		}
		else if (cell->type == ID($demux))
		{
			if (!traceSig(sig_a, undef, cell))
				return false;
			if (sig_a.is_fully_zero()) {
				set(sig_y, Const(0, GetSize(sig_y)));
			} else {
				if (!traceSig(sig_s, undef, cell))
					return false;
				set(sig_y, const_demux(sig_a.as_const(), sig_s.as_const()));
			}
		}
		else if (cell->type == ID($fa))
		{
			RTLIL::SigSpec sig_c = cell->getPort(ID::C);
			RTLIL::SigSpec sig_x = cell->getPort(ID::X);
			int width = GetSize(sig_c);

			if (!traceSig(sig_a, undef, cell))
				return false;

			if (!traceSig(sig_b, undef, cell))
				return false;

			if (!traceSig(sig_c, undef, cell))
				return false;

			RTLIL::Const t1 = const_xor(sig_a.as_const(), sig_b.as_const(), false, false, width);
			RTLIL::Const val_y = const_xor(t1, sig_c.as_const(), false, false, width);

			RTLIL::Const t2 = const_and(sig_a.as_const(), sig_b.as_const(), false, false, width);
			RTLIL::Const t3 = const_and(sig_c.as_const(), t1, false, false, width);
			RTLIL::Const val_x = const_or(t2, t3, false, false, width);

			for (int i = 0; i < GetSize(val_y); i++)
				if (val_y.bits[i] == RTLIL::Sx)
					val_x.bits[i] = RTLIL::Sx;

			set(sig_y, val_y);
			set(sig_x, val_x);
		}
		else if (cell->type == ID($alu))
		{
			bool signed_a = cell->parameters.count(ID::A_SIGNED) > 0 && cell->parameters[ID::A_SIGNED].as_bool();
			bool signed_b = cell->parameters.count(ID::B_SIGNED) > 0 && cell->parameters[ID::B_SIGNED].as_bool();

			RTLIL::SigSpec sig_ci = cell->getPort(ID::CI);
			RTLIL::SigSpec sig_bi = cell->getPort(ID::BI);

			if (!traceSig(sig_a, undef, cell))
				return false;

			if (!traceSig(sig_b, undef, cell))
				return false;

			if (!traceSig(sig_ci, undef, cell))
				return false;

			if (!traceSig(sig_bi, undef, cell))
				return false;

			RTLIL::SigSpec sig_x = cell->getPort(ID::X);
			RTLIL::SigSpec sig_co = cell->getPort(ID::CO);

			bool any_input_undef = !(sig_a.is_fully_def() && sig_b.is_fully_def() && sig_ci.is_fully_def() && sig_bi.is_fully_def());
			sig_a.extend_u0(GetSize(sig_y), signed_a);
			sig_b.extend_u0(GetSize(sig_y), signed_b);

			bool carry = sig_ci[0] == State::S1;
			bool b_inv = sig_bi[0] == State::S1;

			for (int i = 0; i < GetSize(sig_y); i++)
			{
				RTLIL::SigSpec x_inputs = { sig_a[i], sig_b[i], sig_bi[0] };

				if (!x_inputs.is_fully_def()) {
					set(sig_x[i], RTLIL::Sx);
				} else {
					bool bit_a = sig_a[i] == State::S1;
					bool bit_b = (sig_b[i] == State::S1) != b_inv;
					bool bit_x = bit_a != bit_b;
					set(sig_x[i], bit_x ? State::S1 : State::S0);
				}

				if (any_input_undef) {
					set(sig_y[i], RTLIL::Sx);
					set(sig_co[i], RTLIL::Sx);
				} else {
					bool bit_a = sig_a[i] == State::S1;
					bool bit_b = (sig_b[i] == State::S1) != b_inv;
					bool bit_y = (bit_a != bit_b) != carry;
					carry = (bit_a && bit_b) || (bit_a && carry) || (bit_b && carry);
					set(sig_y[i], bit_y ? State::S1 : State::S0);
					set(sig_co[i], carry ? State::S1 : State::S0);
				}
			}
		}
		else if (cell->type == ID($macc))
		{
			Macc macc;
			macc.from_cell(cell);

			if (!traceSig(macc.bit_ports, undef, cell))
				return false;

			for (auto &port : macc.ports) {
				if (!traceSig(port.in_a, undef, cell))
					return false;
				if (!traceSig(port.in_b, undef, cell))
					return false;
			}

			RTLIL::Const result(0, GetSize(cell->getPort(ID::Y)));
			if (!macc.eval(result))
				log_abort();

			set(cell->getPort(ID::Y), result);
		}
		else
		{
			RTLIL::SigSpec sig_c, sig_d;

			if (cell->type.in(ID($_AOI3_), ID($_OAI3_), ID($_AOI4_), ID($_OAI4_))) {
				if (cell->hasPort(ID::C))
					sig_c = cell->getPort(ID::C);
				if (cell->hasPort(ID::D))
					sig_d = cell->getPort(ID::D);
			}

			if (sig_a.size() > 0 && !traceSig(sig_a, undef, cell))
				return false;
			if (sig_b.size() > 0 && !traceSig(sig_b, undef, cell))
				return false;
			if (sig_c.size() > 0 && !traceSig(sig_c, undef, cell))
				return false;
			if (sig_d.size() > 0 && !traceSig(sig_d, undef, cell))
				return false;

			bool eval_err = false;
			RTLIL::Const eval_ret = CellTypes::eval(cell, sig_a.as_const(), sig_b.as_const(), sig_c.as_const(), sig_d.as_const(), &eval_err);

			if (eval_err)
				return false;

			set(sig_y, eval_ret);
		}

		return true;
	}


//Modify the funciton based on the eval(cell)

	bool traceSig(RTLIL::SigSpec &sig,RTLIL::SigSpec &undef, RTLIL::Cell *busy_cell = NULL)
	{
		
		//string SigName = sig.is_wire()? sig.as_wire()->name.c_str() : "Unkown wire";
		//string signame = sig[0].wire ? module->wires_.at(sig[0].wire)
		/*for(auto WireMap : module->wires_)
			if(WireMap.)*/
		
		//string signame = log_signal(sig);
		/*&for(int i = 0; i < GetSize(sig); i++)
			if(sig[i].wire)
				signame = signame + sig[i].wire->name.c_str();
			else 
				printf("the %d of sig is not a wire", i);
		*/
		assign_map.apply(sig);
		string sig_name = log_signal(sig);
		values_map.apply(sig);
		string sig_value = log_signal(sig);

		

		/*if(sig.is_wire())
			{
			RTLIL::Wire *wire = (&sig)->as_wire();
			printf("sig is a wire and the name of sig is %s", wire->name.c_str());
			}
			else
			printf("sig is not a wire");*/

		if (sig.is_fully_const()){
			
			//RTLIL::Wire *sig2wire= sig.as_wire(); sig is failed in as_wire();
			//string sig2name = sig->name.c_str();
			/*path = cell_exe_driver  + "<=" + "(port_name)" + "|"; 
			wire.set_string_attribute(ID(cell_driver),path);*/

			index = busy_cell->name.str().find_last_of('$');
			string cell_name = busy_cell->name.str().substr(index+1);
			string cell_type = busy_cell->type.str();
			index = sig_name.find_last_of('\\');
			sig_name = sig_name.substr(index+1);

			path_cell = cell_exe + "<-" + cell_name + "[" + cell_type + "]" + "<=" + sig_name ;
			path_cell_all.push_back(path_cell);

			string module_name = busy_cell->get_string_attribute(mod_name);
			string module_type = busy_cell->get_string_attribute(mod_type);

			index = mod_exe.find_last_of("-");
			index_1 = mod_exe.find_last_of("[");
			
			
			if(index > 0 && !module_name.compare(mod_exe.substr(index+1,index_1-index-1)))
			{
				path_mod = mod_exe + "<=" + sig_name;	
			}
			else
			{
				path_mod = mod_exe + "<-" + module_name + "[" +module_type + "]" + "<=" + sig_name;

			}
			path_mod_all.push_back(path_mod);
			
			
			
			return true;
		}

		if (stop_signals.check_any(sig)) {
			undef = stop_signals.extract(sig);
			return false;
		}

		if (busy_cell) {
			if (busy.count(busy_cell) > 0) {
				undef = sig;
				return false;
			}
			busy.insert(busy_cell);
            
			index = busy_cell->name.str().find_last_of('$');
			string cell_name = busy_cell->name.str().substr(index+1);
			string cell_type = busy_cell->type.str();
			cell_exe = cell_exe + "<-"  + cell_name + "[" + cell_type + "]";


			string module_name = busy_cell->get_string_attribute(mod_name);
			string module_type = busy_cell->get_string_attribute(mod_type);
			
			index = mod_exe.find_last_of("-");
			index_1 = mod_exe.find_last_of("[");
			
			if(index < 0 ||  module_name.compare(mod_exe.substr(index+1,index_1-index-1)))
				mod_exe = mod_exe + "<-" + module_name + "[" + module_type + "]";
			
			/*if(!wire.has_attribute(ID(cell_busy)))
				cell_exe_busy = "exe path cell(busy way) : [" + busy_cell->type.str() + "]";	
			else */
				
					
            	/*wire.set_string_attribute(ID(cell_busy),cell_exe_busy);
            
			
				/*
				if(wire.has_attribute(ID(mod_busy)))
					mod_exe_busy = wire.get_string_attribute(ID(mod_busy)) + "->" + busy_cell->get_string_attribute(ID(mod)); 
				else 
					mod_exe_busy = "exe path mod(busy_way) : ";
				wire.set_string_attribute(ID(mod_busy),mod_exe_busy);
				*/   
		}

		std::set<RTLIL::Cell*> driver_cells;
		sig2driver_out.find(sig, driver_cells);
		for (auto cell : driver_cells) {
			/*if(!wire.has_attribute(ID(cell_driver)))
				cell_exe_driver = "exe path cell(driver way) :" + cell->type.str();	
			else 
				cell_exe_driver = wire.get_string_attribute(ID(cell_driver)) + "<-" + cell->type.str();
					
            	wire.set_string_attribute(ID(cell_driver),cell_exe_driver);*/
            
			if (!traceCell(cell, undef)) {
				if (busy_cell)
					busy.erase(busy_cell);
				return false;
			}

			/*if(wire.has_attribute(ID(cell_driver)))
				cell_exe_driver = wire.get_string_attribute(ID(cell_driver)) + "->" + cell->name.str();
			else 
				cell_exe_driver = "exe path cell(driver way) :";
            wire.set_string_attribute(ID(cell_driver),cell_exe_driver);	*/
		}

		if (busy_cell)
			busy.erase(busy_cell);
			index = cell_exe.find_last_of('<');
			cell_exe = cell_exe.substr(0,index);
			index = mod_exe.find_last_of('-');
			index_1 = mod_exe.find_last_of('[');
			if(index >0 && !mod_exe.substr(index+1,index_1-index-1).compare(busy_cell->get_string_attribute(mod_name)))
				mod_exe = mod_exe.substr(0,index-1);
			

		values_map.apply(sig);
		if (sig.is_fully_const())
			return true;

		if (defaultval != RTLIL::State::Sm) {
			for (auto &bit : sig)
				if (bit.wire) bit = defaultval;
			return true;
		}

		for (auto &c : sig.chunks())
			if (c.wire != NULL)
				undef.append(c);
		return false;
	}

	bool showtrace(void){

		printf("======================the exe path on the cell level=====================\n");
		for(auto it : path_cell_all)
		{
			std::cout << it << "\n";
		}
		printf("======================the exe path on the mod level=====================\n");
		for(auto it: path_mod_all)
		{
			std::cout << it << "\n";
		}
		return true;
	}
	

};




YOSYS_NAMESPACE_END