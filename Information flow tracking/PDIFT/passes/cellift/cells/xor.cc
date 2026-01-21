#include "kernel/rtlil.h"
#include "kernel/utils.h"
#include "kernel/log.h"
#include "kernel/yosys.h"
#include <map>
#include <string>
#include <vector>
#include <algorithm>


USING_YOSYS_NAMESPACE
extern RTLIL::IdString Yosys::mod_flag;
extern RTLIL::IdString Yosys::loop_front;
extern std::vector<RTLIL::SigSpec> get_corresponding_taint_signals(RTLIL::Module* module, std::vector<string> *excluded_signals, const RTLIL::SigSpec &sig, unsigned int num_taints);
extern std::vector<RTLIL::SigSpec> get_corresponding_label_signals(RTLIL::Module* module, std::vector<string> *excluded_signals, const RTLIL::SigSpec &sig, unsigned int num_taints, unsigned int num_states);
extern RTLIL::SigSpec get_mod_state(string front_mod, int mod_num);
extern RTLIL::SigSpec check_list(RTLIL::Module* module, RTLIL::SigSpec y_states_output, std::set<RTLIL::SigSig> Enter_Check_List);
extern RTLIL::SigSig Check_and_Refresh(RTLIL::Module* module, RTLIL::SigSpec y_states, string attr);
/**
 * @param module the current module instance
 * @param cell the current cell instance
*
 * @return keep_current_cell
 */
bool cellift_xor(RTLIL::Module *module, RTLIL::Cell *cell, unsigned int num_taints, unsigned int num_states, std::vector<string> *excluded_signals) {

    const unsigned int A = 0, B = 1, Y = 2;
    const unsigned int NUM_PORTS = 3;
    RTLIL::SigSpec ports[NUM_PORTS] = {cell->getPort(ID::A), cell->getPort(ID::B), cell->getPort(ID::Y)};
    std::vector<RTLIL::SigSpec> port_taints[NUM_PORTS];
    int output_width = ports[Y].size();

    for (unsigned int i = 0; i < NUM_PORTS; ++i)
        port_taints[i] = get_corresponding_label_signals(module, excluded_signals, ports[i], num_taints, state_map["state_num"]);
    
    /*
    //for output is one part sof wire [more to one]
    if(ports[2].chunks().size() == 1 && ports[2].as_chunk().wire->port_output){
        if(ports[2].as_chunk().offset + ports[2].as_chunk().width < ports[2].as_chunk().wire->width){
            //RTLIL::SigSpec state_spec = port_taints[2].front().extract(0, ports[2].as_chunk().width);
            //port_taints[2].clear();
            //port_taints[2][0].append(RTLIL::SigChunk(port_taints[2].front().as_chunk().wire, port_taints[2].front().as_chunk().offset, ports[2].as_chunk().width));
            //port_taints[2][0].append(RTLIL::SigChunk(RTLIL::State::S0, state_map["state_num"]));
            port_taints[2][0].remove(ports[2].size()-1, state_map["state_num"]);
            port_taints[2][0].append(RTLIL::SigChunk(RTLIL::State::S0, state_map["state_num"]));
        }
    }*/

    for (unsigned int taint_id = 0; taint_id < num_taints; taint_id++)
    {
        //module->addOr(NEW_ID, port_taints[A][taint_id], port_taints[B][taint_id], port_taints[Y][taint_id]);
        //RTLIL::SigSpec extended_a_taint = port_taints[A][taint_id].extract(0, output_width);
        //RTLIL::SigSpec extended_b_taint = port_taints[B][taint_id].extract(0, output_width);

        if(cell->get_string_attribute(ID(port_opt)) == "A"){
            module->connect(port_taints[B][taint_id], port_taints[Y][taint_id]);
        }
        else if(cell->get_string_attribute(ID(port_opt)) == "B"){
            module->connect(port_taints[A][taint_id], port_taints[Y][taint_id]);
        }
        else if(cell->get_bool_attribute(ID(untaint_area)) == true){
            module->connect(port_taints[A][taint_id], port_taints[Y][taint_id]);
        }
        else{
        module->addOr(NEW_ID, port_taints[A][taint_id].extract(0, output_width), port_taints[B][taint_id].extract(0, output_width), port_taints[Y][taint_id]);
        //RTLIL::SigSpec y_taint_extend = module->Or(NEW_ID, extended_a_taint, extended_b_taint);
        //y_taint_extend.append(RTLIL::SigSpec(RTLIL::State::S0, num_states));
        }
        RTLIL::SigSpec a_states = port_taints[A][taint_id].extract_end(output_width);
        RTLIL::SigSpec b_states = port_taints[B][taint_id].extract_end(output_width);
        //RTLIL::SigSpec sele_extend = module->Ge(NEW_ID, a_states, b_states);
        //RTLIL::SigSpec y_states = module->Mux(NEW_ID, b_states, a_states, sele_extend);
        //RTLIL::SigSpec y_states = module->Or(NEW_ID, a_states, b_states);
        
        string attr = cell->get_string_attribute(mod_flag);
        if(cell->get_string_attribute(mod_flag) != ""){
            port_taints[Y][taint_id].append(Check_and_Refresh(module, a_states, attr).second);             

            //y_states = Check_and_Refresh(module, y_states, attr).second;
            //y_taint_extend = module->Mux(NEW_ID, y_taint_extend ,RTLIL::SigSpec(RTLIL::State::S0, output_width + num_states), Check_and_Refresh(module, y_states, attr).first) ;
        }
  
        port_taints[Y][taint_id].append(a_states);
        //RTLIL::SigSpec y_state_extend = RTLIL::SigSpec(RTLIL::State::S0, output_width);
        //y_state_extend.append(y_states);
        
        //module->addOr(NEW_ID, y_taint_extend, y_state_extend, port_taints[Y][taint_id]);
    }
    return true;
}
