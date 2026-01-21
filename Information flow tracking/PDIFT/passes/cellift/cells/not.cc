#include "kernel/rtlil.h"
#include "kernel/utils.h"
#include "kernel/log.h"
#include "kernel/yosys.h"

USING_YOSYS_NAMESPACE
extern std::vector<RTLIL::SigSpec> get_corresponding_taint_signals(RTLIL::Module* module, std::vector<string> *excluded_signals, const RTLIL::SigSpec &sig, unsigned int num_taints);
extern std::vector<RTLIL::SigSpec> get_corresponding_label_signals(RTLIL::Module* module, std::vector<string> *excluded_signals, const RTLIL::SigSpec &sig, unsigned int num_taints, unsigned int num_states);
extern RTLIL::SigSig Check_and_Refresh(RTLIL::Module* module, RTLIL::SigSpec y_states, string attr);
/**
 * @param module the current module instance
 * @param cell the current cell instance
*
 * @return keep_current_cell
 */
bool cellift_not(RTLIL::Module *module, RTLIL::Cell *cell, unsigned int num_taints, unsigned int num_states, std::vector<string> *excluded_signals) {

    const unsigned int A = 0, Y = 1;
    const unsigned int NUM_PORTS = 2;
    RTLIL::SigSpec ports[NUM_PORTS] = {cell->getPort(ID::A), cell->getPort(ID::Y)};
    std::vector<RTLIL::SigSpec> port_taints[NUM_PORTS];
    int output_width = ports[Y].size();

    for (unsigned int i = 0; i < NUM_PORTS; ++i)
        port_taints[i] = get_corresponding_label_signals(module, excluded_signals, ports[i], num_taints, state_map["state_num"]);

    for (unsigned int taint_id = 0; taint_id < num_taints; taint_id++){
        module->connect(port_taints[Y][taint_id], port_taints[A][taint_id]);
        
        /*
        RTLIL::SigSpec y_states = port_taints[A][taint_id].extract_end(output_width);
        
        string attr = cell->get_string_attribute(mod_flag);
        if(cell->get_string_attribute(mod_flag) != "")
            y_states = Check_and_Refresh(module, y_states, attr).second;


        RTLIL::SigSpec y_state_extend = RTLIL::SigSpec(RTLIL::State::S0, output_width);
        y_state_extend.append(y_states);
        module->addOr(NEW_ID, y_taint_extend, y_state_extend, port_taints[Y][taint_id]);
        */

        
    }


    return true;
}
