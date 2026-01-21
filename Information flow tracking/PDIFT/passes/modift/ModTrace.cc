/*
 *  yosys -- Yosys Open SYnthesis Suite
 *
 *  Copyright (C) 2020  Alberto Gonzalez <boqwxp@airmail.cc> & Flavien Solt <flsolt@ethz.ch>
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

#include "kernel/yosys.h"
#include "kernel/register.h"
#include "kernel/celltypes.h"
//#include "kernel/consteval.h"
#include "ModTools.cc"
#include "kernel/log.h"
#include "kernel/register.h"
#include "kernel/satgen.h"
#include "kernel/sigtools.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN



struct ModTracePass : public Pass{
    
    
    ModTracePass() : Pass("ModTrace", "Provide the execution path of the specified signal") {}

/*  
    void mod_exeing(std::vector<std::string> args, RTLIL::Design *design)
    {
        SigMap assign_map;
		SigMap values_map;
        SigSet<RTLIL::Cell*> sig2driver_out;
		SigSet<RTLIL::Cell*> sig2driver_in;
		std::set<RTLIL::SigSpec> sig_in;
		std::set<RTLIL::SigSpec> sig_out;
        std::map<RTLIL::Module, RTLIL::SigSpec> mod_end_sig;
        std::set<RTLIL::Cell*> end_of_mod_all;
		std::set<RTLIL::Cell*> start_of_mod_all;
        for(auto &mod_iter : design->modules_)
        {
            RTLIL::Module* module = mod_iter.second;
            //judege if module need to be exe_imstrumented;

            CellTypes ct;
			ct.setup_internals();
			ct.setup_stdcells();
            for (auto &it : module->cells_)
            {
				if (!ct.cell_known(it.second->type))
				    continue;
                for (auto &it2 : it.second->connections())
                {
                    if (ct.cell_output(it.second->type, it2.first))
					{
						sig2driver_out.insert(assign_map(it2.second), it.second);
						sig_in.insert(it2.second);
					}
				else if(ct.cell_input(it.second->type, it2.first))
					{
						sig2driver_in.insert(assign_map(it2.second), it.second);
						sig_out.insert(it2.second);
					}
                }
            }

            for(auto &it : module->cells_)
			for(auto &sig : it.second->connections_)
			{
				if((sig_in.find(sig.second)!= sig_in.end()) && (sig_out.find(sig.second) == sig_out.end()))
					start_of_mod_all.insert(it.second);
				if((sig_out.find(sig.second) != sig_out.end()) && (sig_in.find(sig.second) == sig_in.end()))
					{
                        end_of_mod_all.insert(it.second);
                        mod_end_sig.insert(make_pair(*module,sig.second));
                    }
				if((sig_in.find(sig.second)!= sig_in.end()) && (sig_out.find(sig.second)!= sig_out.end()))
					if(start_of_mod_all.find(it.second)!=start_of_mod_all.end())
						start_of_mod_all.erase(it.second);
					continue;		
			}

            for(auto it : mod_end_sig)
            {
                RTLIL::Wire *set;
                RTLIL::SigSpec out_old;
		        RTLIL::SigSpec out_new = it.second;
    		    RTLIL::SigSpec compare,compare_en;
	    	    RTLIL::Wire *tag = module->addWire(it.first.name,1);// mod exed or not
		        module->addDff(NEW_ID,set,out_new,out_old);
		        module->addEq(NEW_ID,out_new,out_old,compare);
		        module->addNot(NEW_ID,compare,compare_en);
		        module->addDff(NEW_ID,compare_en,1,tag);
            }

            //connect

            module->fixup_ports();

        }


        //insturment each out_sig [tag of mod]
        
    }
*/

    void execute(std::vector<std::string> args, RTLIL::Design *design) override
    {
        std::vector<std::pair<std::string, std::string>> sets;
        //std::vector<std::string> taints;
        std::deque<std::string> taints;
		std::vector<std::string> shows;

        //extract the parameters
        size_t argidx;
		for (argidx = 1; argidx < args.size(); argidx++) {
			if (args[argidx] == "-set" && argidx + 2 < args.size()) {
				std::string arg = args[++argidx].c_str();
				std::string value = args[++argidx].c_str();
				sets.push_back(std::pair<std::string, std::string>(arg, value));
				continue;
			}
            if(args[argidx] == "-taint" && argidx + 1 < args.size()){
                taints.push_back(args[++argidx]);
                continue;
            }
            if (args[argidx] == "-show" && argidx + 1 < args.size()) {
				shows.push_back(args[++argidx]);
				continue;
			}
            break; 
        }
        extra_args(args, argidx, design);
        //extract the parameters

        //set the const input for whole design
        RTLIL::Module *module = NULL; 
        for(auto mod : design->selected_modules())
        {
            if(module)
                log_cmd_error("Try to use flatten pass.\n");
            module = mod;
        } 
        if (module == NULL)
            log_cmd_error("Can't find the module.\n");
    
        DesignMap ce(module);
        for (auto &it : sets) {
			RTLIL::SigSpec lhs, rhs;
			if (!RTLIL::SigSpec::parse_sel(lhs, design, module, it.first))
				log_cmd_error("Failed to parse lhs set expression `%s'.\n", it.first.c_str());
			if (!RTLIL::SigSpec::parse_rhs(lhs, rhs, module, it.second))
				log_cmd_error("Failed to parse rhs set expression `%s'.\n", it.second.c_str());
			if (!rhs.is_fully_const())
				log_cmd_error("Right-hand-side set expression `%s' is not constant.\n", it.second.c_str());
			if (lhs.size() != rhs.size())
				log_cmd_error("Set expression with different lhs and rhs sizes: %s (%s, %d bits) vs. %s (%s, %d bits)\n",
					      it.first.c_str(), log_signal(lhs), lhs.size(), it.second.c_str(), log_signal(rhs), rhs.size());
			ce.set(lhs, rhs.as_const());
            //ce.set_taints(lhs, taints.front());
            //taints.pop_front();
		}
         //set the const input for whole design
        
        //Traverse all cell by module, add module attribute to cell[flatten cause only one module]



        //Traverse signal drivers, add to wrie's attribute
        for (auto &it : shows) {
				RTLIL::SigSpec signal, value, undef;
				if (!RTLIL::SigSpec::parse_sel(signal, design, module, it))
					log_cmd_error("Failed to parse show expression `%s'.\n", it.c_str());
				value = signal;
                RTLIL::Wire *wire = signal.as_wire();
                log("Executing MoDIFT pass (provide the exection path of %s .)\n", wire->name.c_str());
               
                    
                if(!ce.traceSig(value, undef))
                    log("Failed to eval and traverse signal %s: Missing value for %s.\n", log_signal(signal), log_signal(undef));
                else
					ce.showtrace();
        }
    }
} ModTracePass;

PRIVATE_NAMESPACE_END
