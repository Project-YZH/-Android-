#include "kernel/yosys.h"
#include "kernel/sigtools.h"

#include <string>
#include <map>
#include <set>
#include <algorithm>

#include "kernel/log.h"
#include "kernel/register.h"
#include "kernel/rtlil.h"
#include "kernel/utils.h"
#include "kernel/yosys.h"
#include <stdio.h>

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

static void find_stub_nets(RTLIL::Design *design, RTLIL::Module *module, bool report_bits)
{
    //WSM use sigmap() function
    //SigMap sigmap(module);
    
    //count how many times a single-bit signal is used
    //std::map<RTLIL::SigBit, int> bit_usage_count;
    
    //count output lines for this module (needed only for summary output at the end)
    //int line_count = 0;
    
    log("Looking for stub wires in module %s : \n", RTLIL::id2cstr(module->name));

    std::vector<RTLIL::SigSig> connections(module->connections());
    for (auto &conn : connections) {
		log_assert(conn.first.size() == conn.second.size());
        log("the driving sig is [%s] , size is %d ===> ", log_signal(conn.second), conn.second.size());
        log("the driven sig is [%s] , size is %d \n", log_signal(conn.first), conn.first.size());
        
    }
    //find the map relations 
    /*
    for(auto &cell_iter : module->cells_)
    for(auto &conn : cell_iter.second->connections())
    {
        string cell_stirng = log_id(cell_iter.first);
        string cell_name = log_id(cell_iter.second->name);
        string cell_type = log_id(cell_iter.second->type);
        
        RTLIL::SigSpec Sig = conn.second;
        string portSig_stirng = log_id(conn.first);
        string protSig_name = conn.second.is_wire()? conn.second.as_wire()->name.c_str() : "Not a wire";

        RTLIL::SigSpec sig = sigmap(conn.second);
        string mapSig_name = sig.is_wire()? log_id(sig.as_wire()->name) : "Not a wire";


        for(auto &wire_iter : module->wires_)
        {
            if (Sig[0].wire == wire_iter.second)
            printf("this sig_wire in module->wires_");
        }
        //sig.is_wire() ? log_id(sig.as_wire()->name) : log_id("sig is not a wire\n");
        for(auto sigsig : module->connections())
        {   
            if(sigsig.first == Sig)
                if(sigsig.first.is_wire()) 
                    log_id(sigsig.first.as_wire()->name);
                else
                printf("sigsig.first is not a wire\n");
            continue;
        }    
        //auto it = find(module->connections_.begin(),module->connections_.end(),SigSig);
    }*/


    //For each wire in the module
    /*for(auto &wire_iter : module->wires_)
    {
        RTLIL::Wire *wire = wire_iter.second;

        if(!design->selected(module,wire))
            continue;

        //add +1 usage if this wire acutally is a port
        int usage_offset = wire->port_id > 0 ? 1 : 0;

        std::set<int> stub_bits;

        RTLIL::SigSpec sig = wire;

        for(int i = 0; i < GetSize(sig); i++)
            if(sig[i].wire != NULL && (bit_usage_count[sig[i]]+usage_offset)<2)
                stub_bits.insert(i);

        if(stub_bits.size() == 0)
            continue;

        //report stub bits and/or stub wires, don't report single bits
        //if called with report_bits set to false
        if (GetSize(stub_bits) == GetSize(sig)) {
 			log("  found stub wire: %s\n", RTLIL::id2cstr(wire->name));
 		} else {
 			if (!report_bits)
 			continue;
 		log("  found wire with stub bits: %s [", RTLIL::id2cstr(wire->name));
 			for (int bit : stub_bits)
 				log("%s%d", bit == *stub_bits.begin() ? "" : ", ", bit);
 			log("]\n");
 		}
        line_count++;
    }
    if(report_bits)
        log("   found %d stub wires or wires with stub bits. \n",line_count);
    else  
        log("   found %d stub wires.\n",line_count);*/

}

//each pass contains a singleton object that is derived from pass
struct StubnetsPass : public Pass
{
    StubnetsPass() : Pass("stubnets"){}
    void execute(std::vector<std::string> args, RTLIL::Design *design) override
    {
        bool report_bits = 0;
        log_header(design, "Executing STUBNETS pass (find stub nets) .\n");

        //parse options 
        size_t argidx;
        for(argidx = 1; argidx < args.size(); argidx++){
            std::string arg = args[argidx];
            if(arg == "-report_bits"){
                report_bits = true;
                continue;
            }
            break;
        }

        extra_args(args, argidx, design);

        for(auto &it : design->modules_)
            if(design->selected_module(it.first))
                find_stub_nets(design, it.second, report_bits);

    }
}StubnetsPass;

PRIVATE_NAMESPACE_END
