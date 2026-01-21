/*
 *	2023.7 sysj1606 sunhd365@gmail.com
 *
 *  ModExe.cc
 *	find the begin/end cell/wire
 *	set the attribtue in cell/wire for future creative work
 */

#include "kernel/utils.h"
#include "kernel/yosys.h"
#include "kernel/register.h"
#include "kernel/celltypes.h"
#include "kernel/sigtools.h"
#include "kernel/log.h"

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <algorithm>
#include <iostream>
#include <vector>


std::map<std::string, int> state_map;
std::string STM_express;
USING_YOSYS_NAMESPACE
std::set<RTLIL::SigSig> get_mod_CheckingList(string attr);
RTLIL::IdString Yosys::mod_flag = ID(mod_exe_flag);
//vector<std::set<RTLIL::SigSig>> mod_policy;


struct ModExePass : public Pass{

    ModExePass() : Pass("ModExe", "Determines whether a module is executed //by mod_end_cell//") {}

    void execute(std::vector<std::string> args, RTLIL::Design *design) override
    {
		//get the STM_express, include bit numbuer of state_label[ loop_num + mod_num + ... ]
		
		log("Enter the State machine expression : \t");
		std::cin >> STM_express;


		//if express have loop, get the loop_num; else loop_num = 0;
		int loop_num = 0;
		if(STM_express.find('*') != string::npos)
		{
			int loop_num_index = STM_express.find('*');
			char loop_num_char = STM_express.at(loop_num_index + 1);
			// if loop_num_char is 16 then only the 1 being process; there need to fix
			int loop = loop_num_char -'0';
			state_map.insert(make_pair("loop", loop));
			if(loop < 4) 
				state_map.insert(make_pair("loop_num", 2));
			else if(4 < loop && loop < 8)
				state_map.insert(make_pair("loop_num", 3));
			else if(9 <= loop && loop < 16)
				state_map.insert(make_pair("loop_num", 4));
			else if(16 <= loop )
				state_map.insert(make_pair("loop_num", 5));
			else 
				printf("the mix loop is 16");
		}

		//get the states number of express, A,B,C means three states, if add D, that's four
		int mod_num;
		if(STM_express.find('E') != string::npos)
			mod_num = 5;
		else if(STM_express.find('D') != string::npos)
			mod_num = 4;
		else if(STM_express.find('C') != string::npos)
			mod_num = 3;
		else if(STM_express.find('B') != string::npos)
			mod_num = 2;
		else{
			mod_num = 1;
		}
		state_map.insert(make_pair("mod_num", mod_num));

		state_map.insert(make_pair("state_num", state_map["loop_num"] + mod_num));

		//Iterate all modules and annoteated the unique idenity of the interest module
		int flag; // wheather instrument a module
		string attr; 
		for(auto &mod_iter : design->modules_)
        {
			log("module is %s, whether flag the exe state: \t", mod_iter.second->name.c_str());
			std::cin >> flag;
			if(flag == 0)
				continue;
			log("enter the %s state: \t", mod_iter.second->name.c_str());
			std::cin >> attr;

 			CellTypes ct;
			ct.setup_internals();
			ct.setup_stdcells();
			SigMap assign_map; //store all cell in map 

			SigSet<RTLIL::Cell*> sig2driver_out;
			SigSet<RTLIL::Cell*> sig2driver_in;
			std::set<RTLIL::SigSpec> sig_in;
			std::set<RTLIL::SigSpec> sig_out;
			
			//std::map<RTLIL::SigSpec, RTLIL::SigSpec> mod_start_sig;
			//RTLIL::IdString sig_name;

			for (auto &it : mod_iter.second->cells_) {
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
			//get the relation between cell and wire, by port

			std::set<RTLIL::Cell*> mod_end_cell;
			std::set<RTLIL::Cell*> mod_start_cell;
			std::map<RTLIL::Module *, RTLIL::SigSpec> mod_end_sig;
			std::map<RTLIL::Module *, RTLIL::SigSpec> mod_start_sig;
			
			std::set<string> sig_in_name;
			std::set<string> sig_out_name;
			string name;

			for(auto &it : sig_in)
			{
				if(it.is_fully_const()) // ? 
					continue;
				else if(it.is_wire())//one to one
					name = log_id(it.as_wire()->name);
				else if(it.chunks().size() >= 1){ 
					if(it.chunks().size() == 1){ //one to more
						name = log_id(it.as_chunk().wire->name);
						sig_in_name.insert(name);
					}
					else if(it.chunks().size() > 1){ //more to one
						for(int i = 0; i < it.chunks().size(); i++){
							name = log_id(it.chunks().at(i).wire->name);
							sig_in_name.insert(name);
						}
					}
				}
			}
			for(auto &it : sig_out)
			{
				if(it.is_wire())
					name = log_id(it.as_wire()->name);
				else 
					name = log_id(it.as_chunk().wire->name);

				sig_out_name.insert(name);
			}
			// to fix the situlation a[0] and a

			for(auto &it : mod_iter.second->cells_)
			for(auto &sig : it.second->connections_)
			{

				if((sig_in.find(sig.second)!= sig_in.end()) && (sig_out.find(sig.second) != sig_out.end()))
					continue;
				if((sig_in.find(sig.second)!= sig_in.end()) && (sig_out.find(sig.second) == sig_out.end()))
				{	
					mod_start_cell.insert(it.second);
					mod_start_sig.insert(pair<RTLIL::Module *,RTLIL::SigSpec>(mod_iter.second,sig.second));
				}
				if((sig_out.find(sig.second) != sig_out.end()) && (sig_in.find(sig.second) == sig_in.end()))
				{
					
					name = log_id(sig.second.as_chunk().wire->name);
					if(sig_in_name.find(name) == sig_in_name.end()){
						mod_end_cell.insert(it.second);
						mod_end_sig.insert(pair<RTLIL::Module *,RTLIL::SigSpec>(mod_iter.second,sig.second));
					}
					/*
					if(sig.second.chunks().size() == 1){
						name = log_id(sig.second.as_chunk().wire->name);
						if(sig_in_name.find(name) == sig_in_name.end()){
							mod_end_cell.insert(it.second);
							mod_end_sig.insert(pair<RTLIL::Module *,RTLIL::SigSpec>(mod_iter.second,sig.second));
						}
					}
					else if(sig.second.chunks().size() > 1){
						for(int i = 0; i < sig.second.chunks().size(); i++){
							name = log_id(sig.second.chunks().at(i).wire->name);
							if(sig_in_name.find(name) == sig_in_name.end()){
								mod_end_cell.insert(it.second);
								mod_end_sig.insert(pair<RTLIL::Module *,RTLIL::SigSpec>(mod_iter.second,sig.second));
							}
						}
					}*/
					
				}	
			}
			//set the unique identifyu of mod on the last cell 
			printf("=======================\n");
			
			std::set<RTLIL::SigSig> mod_list = get_mod_CheckingList(attr);
			Yosys::mod_policy.insert(Yosys::mod_policy.end(), mod_list);
			string index = std::to_string(state_map["mod_num"] - attr.find_last_of("1") - 1);
			mod_policy_index.append(index);
			for(auto &it : mod_end_cell)
			{
				it->set_string_attribute(mod_flag, attr);
				log("cell is %s, \n enter it state: %s ;  \n", it->name.c_str(), attr.c_str());
			}
			//set attribute mod_flag when a cell in the mod_end_cell


			/*
			for(auto &mod_sig : mod_end_sig)
			{
				string prefix = "\\";
				string name = prefix.append(mod_sig.first->name.c_str());

				RTLIL::SigSpec out_old = mod_sig.first->addWire(name + "_old", mod_sig.second.size());
				RTLIL::SigSpec out_new = mod_sig.second;
				RTLIL::SigSpec compare = mod_sig.first->addWire(name + "_compare", 1);
				RTLIL::Wire *set = mod_sig.first->addWire(name + "_set", 1);
				RTLIL::Wire *flag = mod_sig.first->addWire(name + "_flag", 1); // mod exed or not
				mod_sig.first->addDff(NEW_ID,set,out_new,out_old);
				mod_sig.first->addEq(NEW_ID,out_new,out_old,compare);
				mod_sig.first->addNot(NEW_ID,compare,flag);

				mod_sig.first->fixup_ports();	
			}
			*/	
		}   
    }    
} ModExePass;
