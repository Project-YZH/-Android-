#include "kernel/rtlil.h"
#include "kernel/utils.h"
#include "kernel/log.h"
#include "kernel/yosys.h"
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
bool cellift_mux(RTLIL::Module *module, RTLIL::Cell *cell, unsigned int num_taints, unsigned int num_states, std::vector<string> *excluded_signals) {

    const unsigned int A = 0, B = 1, S = 2, Y = 3;
    const unsigned int NUM_PORTS = 4;
    RTLIL::SigSpec ports[NUM_PORTS] = {cell->getPort(ID::A), cell->getPort(ID::B), cell->getPort(ID::S), cell->getPort(ID::Y)};
    std::vector<RTLIL::SigSpec> port_taints[NUM_PORTS];

    if (ports[A].size() != ports[B].size() || ports[B].size() != ports[Y].size())
        log_cmd_error("In $mux, all data ports must have the same size.\n");
    for (unsigned int i = 0; i < NUM_PORTS; ++i)
        port_taints[i] = get_corresponding_label_signals(module, excluded_signals, ports[i], num_taints, state_map["state_num"]);

    int data_size = ports[A].size();
    int sele_size = ports[S].size();


    for (unsigned int taint_id = 0; taint_id < num_taints; taint_id++) {
        RTLIL::SigSpec extended_a_taint = port_taints[A][taint_id].extract(0, data_size);
        //extended_a_taint.remove(output_width,num_states);
        RTLIL::SigSpec extended_b_taint = port_taints[B][taint_id].extract(0, data_size);
        //extended_b_taint.remove(output_width,num_states);

        RTLIL::SigSpec extended_s = RTLIL::SigSpec(RTLIL::SigBit(ports[S]), data_size);
        RTLIL::SigSpec extended_s_taint = RTLIL::SigSpec(RTLIL::SigBit(port_taints[S][taint_id].extract(0, sele_size)), data_size);
        
        ///*[2024.11.28]
        if(cell->get_bool_attribute(ID(untaint_area)) == true){
            module->connect(port_taints[A][taint_id], port_taints[Y][taint_id]);
        }
        else if(cell->get_bool_attribute(ID(mem_first)) == true){
            module->addXor(NEW_ID, ports[A], ports[B], port_taints[Y][taint_id]);
        }
        else{//[2024.11.28]*/
        
        // Taints coming from the data input port.
        RTLIL::SigSpec not_s = module->Not(NEW_ID, extended_s);
        RTLIL::SigSpec not_s_or_s_taint = module->Or(NEW_ID, extended_s_taint, not_s);
        RTLIL::SigSpec s_or_s_taint = module->Or(NEW_ID, extended_s_taint, extended_s);

        RTLIL::SigSpec a_taint_and_not_s_or_s_taint = module->And(NEW_ID, extended_a_taint, not_s_or_s_taint);
        RTLIL::SigSpec b_taint_and_s_or_s_taint = module->And(NEW_ID, extended_b_taint, s_or_s_taint);
        RTLIL::SigSpec data_taint_stream = module->Or(NEW_ID, a_taint_and_not_s_or_s_taint, b_taint_and_s_or_s_taint);

        // Taint coming from the control input port.
        RTLIL::SigSpec a_xor_b = module->Xor(NEW_ID, ports[A], ports[B]);
        RTLIL::SigSpec s_taint_and_a_xor_b = module->And(NEW_ID, extended_s_taint, a_xor_b);

        module->addOr(NEW_ID, s_taint_and_a_xor_b, data_taint_stream, port_taints[Y][taint_id]);
        }
        


        ///////////////////////////////////////////////
        RTLIL::SigSpec a_states = port_taints[A][taint_id].extract_end(data_size);
        RTLIL::SigSpec b_states = port_taints[B][taint_id].extract_end(data_size);
        RTLIL::SigSpec s_states = port_taints[S][taint_id].extract_end(sele_size);


        //RTLIL::SigSpec sele_extend = module->Ge(NEW_ID, a_states, b_states);
        //RTLIL::SigSpec y_states = module->Mux(NEW_ID, b_states, a_states, sele_extend);
        //RTLIL::SigSpec y_states_1 = module->Or(NEW_ID, a_states, s_states);
        //RTLIL::SigSpec y_states_2 = module->Or(NEW_ID, b_states, s_states);

        //RTLIL::SigSpec y_states = module->Mux(NEW_ID, y_states_1, y_states_2, cell->getPort(ID::S));
        
        ///////////////////////////////////////////////

        string attr = cell->get_string_attribute(mod_flag);
        if(cell->get_string_attribute(mod_flag) != "")
            port_taints[Y][taint_id].append(Check_and_Refresh(module, a_states, attr).second);             

        port_taints[Y][taint_id].append(a_states);
        //RTLIL::SigSpec y_state_extend = RTLIL::SigSpec(RTLIL::State::S0, data_size);
        //y_state_extend.append(y_states);

        // Output taint & state.
        //module->addOr(NEW_ID, y_taint_extend, y_state_extend, port_taints[Y][taint_id]);
        //module->addOr(NEW_ID, s_taint_and_a_xor_b, data_taint_stream, port_taints[Y][taint_id]);
    }

    return true;
}
