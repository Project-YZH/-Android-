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
bool cellift_or(RTLIL::Module *module, RTLIL::Cell *cell, unsigned int num_taints, unsigned int num_states, std::vector<string> *excluded_signals) {

    const unsigned int A = 0, B = 1, Y = 2;
    const unsigned int NUM_PORTS = 3;
    RTLIL::SigSpec ports[NUM_PORTS] = {cell->getPort(ID::A), cell->getPort(ID::B), cell->getPort(ID::Y)};
    std::vector<RTLIL::SigSpec> port_taints[NUM_PORTS];

    for (unsigned int i = 0; i < NUM_PORTS; ++i)
        port_taints[i] = get_corresponding_label_signals(module, excluded_signals, ports[i], num_taints, state_map["state_num"]);

    int output_width = ports[Y].size();
    RTLIL::SigSpec extended_a = ports[A];
    RTLIL::SigSpec extended_b = ports[B];
    if (ports[A].size() < output_width)
        extended_a.append(RTLIL::SigSpec(RTLIL::State::S0, output_width-ports[A].size()));
    else if (ports[A].size() > output_width)
        extended_a.extract(0, output_width);
    if (ports[B].size() < output_width)
        extended_b.append(RTLIL::SigSpec(RTLIL::State::S0, output_width-ports[B].size()));
    else if (ports[B].size() > output_width)
        extended_b.extract(0, output_width);

    for (unsigned int taint_id = 0; taint_id < num_taints; taint_id++) {
        RTLIL::SigSpec extended_a_taint = port_taints[A][taint_id].extract(0, output_width);
        //extended_a_taint.remove(output_width,num_states);
        RTLIL::SigSpec extended_b_taint = port_taints[B][taint_id].extract(0, output_width);
        //extended_b_taint.remove(output_width,num_states);
        if (ports[A].size() < output_width)
            extended_a_taint.append(RTLIL::SigSpec(RTLIL::State::S0, output_width-ports[A].size()));
        else if (ports[A].size() > output_width)
            extended_a_taint.extract(0, output_width);
        if (ports[B].size() < output_width)
            extended_b_taint.append(RTLIL::SigSpec(RTLIL::State::S0, output_width-ports[B].size()));
        else if (ports[B].size() > output_width)
            extended_b_taint.extract(0, output_width);


        //module->addOr(NEW_ID, a_taint_and_b_or_reverse, a_taint_and_b_taint, port_taints[Y][taint_id]);
        // The one must be tainted and the other must be 1 for the taint to propagate.
        RTLIL::SigSpec not_a = module->Not(NEW_ID, extended_a);
        RTLIL::SigSpec not_b = module->Not(NEW_ID, extended_b);
        RTLIL::SigSpec a_taint_and_not_b = module->And(NEW_ID, extended_a_taint, not_b);
        RTLIL::SigSpec b_taint_and_not_a = module->And(NEW_ID, extended_b_taint, not_a);
        RTLIL::SigSpec a_taint_and_b_taint = module->And(NEW_ID, extended_a_taint, extended_b_taint);
        RTLIL::SigSpec a_taint_and_b_or_reverse = module->Or(NEW_ID, a_taint_and_not_b, b_taint_and_not_a);
        
        //RTLIL::SigSpec y_taint_extend = module->Or(NEW_ID, a_taint_and_b_or_reverse, a_taint_and_b_taint);
        module->addOr(NEW_ID, a_taint_and_b_or_reverse, a_taint_and_b_taint, port_taints[Y][taint_id]);

        //y_taint_extend.append(RTLIL::SigSpec(RTLIL::State::S0, num_states));

        ///////////////////////////////////////////////
        RTLIL::SigSpec a_states = port_taints[A][taint_id].extract_end(output_width);
        RTLIL::SigSpec b_states = port_taints[B][taint_id].extract_end(output_width);

        //w_1
        //RTLIL::SigSpec sele_extend = module->Ge(NEW_ID, a_states, b_states);
        //RTLIL::SigSpec y_states = module->Mux(NEW_ID, b_states, a_states, sele_extend);
        //w_2
        //RTLIL::SigSpec y_states = module->Or(NEW_ID, a_states, b_states);

        ///////////////////////////////////////////////
        //if mod_flag cell, check y_staes and refresh y_states;
        string attr = cell->get_string_attribute(mod_flag);
        if(cell->get_string_attribute(mod_flag) != "")
            port_taints[Y][taint_id].append(Check_and_Refresh(module, a_states, attr).second);


        port_taints[Y][taint_id].append(a_states);
        //RTLIL::SigSpec y_state_extend = RTLIL::SigSpec(RTLIL::State::S0, ports[Y].size());
        //y_state_extend.append(y_states);

        //module->addOr(NEW_ID, y_taint_extend, y_state_extend, port_taints[Y][taint_id]);
    }

    return true;
}
