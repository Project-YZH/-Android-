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
//#include "ModTools.cc"
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

//extern RTLIL::Wire *add_mod_end_flag(RTLIL::Module *module, RTLIL::SigSpec sig, RTLIL::SigSpec set);


extern void mod_cell_map(RTLIL::Design *design, std::map<RTLIL::Cell *, string> *cell_state_map) 
    {
        for(auto &mod_iter : design->modules_)
        {
			string attr;
			log("module is %s, \n enter it state ;  \n", mod_iter.second->name.c_str());
			scanf("%s", &attr);
 			CellTypes ct;
			SigMap assign_map;
			ct.setup_internals();
			ct.setup_stdcells();

			SigSet<RTLIL::Cell*> sig2driver_out;
			SigSet<RTLIL::Cell*> sig2driver_in;
			std::set<RTLIL::SigSpec> sig_in;
			std::set<RTLIL::SigSpec> sig_out;

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

			std::set<RTLIL::Cell*> mod_end_cell;
			std::set<RTLIL::Cell*> mod_start_cell;
			std::map<RTLIL::Module *, RTLIL::SigSpec> mod_end_sig;
			std::map<RTLIL::Module *, RTLIL::SigSpec> mod_start_sig;
			

		
			for(auto &it : mod_iter.second->cells_)
			for(auto &sig : it.second->connections_)
			{

				if((sig_in.find(sig.second)!= sig_in.end()) && (sig_out.find(sig.second)!= sig_out.end()))
					continue;
				if((sig_in.find(sig.second)!= sig_in.end()) && (sig_out.find(sig.second) == sig_out.end()))
				{	
					mod_start_cell.insert(it.second);
					mod_start_sig.insert(pair<RTLIL::Module *,RTLIL::SigSpec>(mod_iter.second,sig.second));
				}
				if((sig_out.find(sig.second) != sig_out.end()) && (sig_in.find(sig.second) == sig_in.end()))
				{
					mod_end_cell.insert(it.second);
					mod_end_sig.insert(pair<RTLIL::Module *,RTLIL::SigSpec>(mod_iter.second,sig.second));
				}	
			}

			/*
			for(auto &it : mod_end_cell)
			{
				it->set_bool_attribute(mod_flag);
				cell_state_map.inster(pair<RTLIL::Cell *, string>(it, attr));
			}
			*/


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
    }
