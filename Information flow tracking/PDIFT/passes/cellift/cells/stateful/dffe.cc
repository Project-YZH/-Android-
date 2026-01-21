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
bool cellift_dffe(RTLIL::Module *module, RTLIL::Cell *cell, unsigned int num_taints, std::vector<string> *excluded_signals) {

    const unsigned int CLK = 0, EN = 1, D = 2, Q = 3;
    const unsigned int NUM_PORTS = 4;
    RTLIL::SigSpec ports[NUM_PORTS] = {cell->getPort(ID::CLK), cell->getPort(ID::EN), cell->getPort(ID::D), cell->getPort(ID::Q)};
    std::vector<RTLIL::SigSpec> port_taints[NUM_PORTS];
    for (unsigned int i = 0; i < NUM_PORTS; ++i)
        port_taints[i] = get_corresponding_label_signals(module, excluded_signals, ports[i], num_taints, state_map["state_num"]);
        //port_taints[i] = get_corresponding_taint_signals(module, excluded_signals, ports[i], num_taints);

    int data_width    = cell->getParam(ID(WIDTH)).as_int();//128
    int clk_polarity  = cell->getParam(ID(CLK_POLARITY)).as_bool();//1
    int en_polarity   = cell->getParam(ID(EN_POLARITY)).as_bool();//1

    RTLIL::SigSpec d_xor_q = module->Xor(NEW_ID, ports[D], ports[Q]);//128

    RTLIL::SigBit en_bit = ports[EN][0];//1
    RTLIL::SigBit not_en_bit = module->Not(NEW_ID, ports[EN]);//1

    // Invert the en bits if necessary.
    RTLIL::SigSpec en_spec;
    RTLIL::SigSpec not_en_spec;
    if (en_polarity) {
        en_spec = RTLIL::SigSpec(en_bit, data_width);
        not_en_spec = RTLIL::SigSpec(not_en_bit, data_width);
    }
    else {
        not_en_spec = RTLIL::SigSpec(en_bit, data_width);
        en_spec = RTLIL::SigSpec(not_en_bit, data_width);
    }

    for (unsigned int taint_id = 0; taint_id < num_taints; taint_id++) {
        RTLIL::SigSpec d_taint_or_q_taint = module->Or(NEW_ID, port_taints[D][taint_id].extract(0, data_width), port_taints[Q][taint_id].extract(0, data_width));
        RTLIL::SigSpec d_q_tainted_or_distinct = module->Or(NEW_ID, d_xor_q, d_taint_or_q_taint);

        // Intermediate signals to OR together.
        RTLIL::SigSpec reduce_or_arr[3];
        reduce_or_arr[0] = module->And(NEW_ID, en_spec, port_taints[D][taint_id].extract(0, data_width));
        reduce_or_arr[1] = module->And(NEW_ID, not_en_spec, port_taints[Q][taint_id].extract(0, data_width));
        reduce_or_arr[2] = module->And(NEW_ID, d_q_tainted_or_distinct, port_taints[EN][taint_id].extract_end(ports[1].size()));

        RTLIL::SigSpec reduce_or_interm = module->Or(NEW_ID, reduce_or_arr[0], reduce_or_arr[1]);
        RTLIL::SigSpec reduce_or_output = module->Or(NEW_ID, reduce_or_interm, reduce_or_arr[2]);
        reduce_or_output.append(RTLIL::SigSpec(port_taints[D][taint_id].extract_end(data_width)));//taint

/*

//////// state extend process
        RTLIL::SigSpec y_states_output = port_taints[D][taint_id].extract_end(data_width);
        string attr = cell->get_string_attribute(mod_flag);
        if(cell->get_string_attribute(mod_flag) != "")
            y_states_output = Check_and_Refresh(module, y_states_output, attr).second;

        RTLIL::SigSpec y_state_extend = RTLIL::SigSpec(RTLIL::State::S0, data_width);
        y_state_extend.append(y_states_output);
//////// end of state process

        RTLIL::SigSpec D_output = module->Or(NEW_ID, reduce_or_output, y_state_extend);
        */
        // No enable signal in the instrumentation. Everything is adapted in the input signal.
        RTLIL::Cell *new_ff = module->addDff(NEW_ID, ports[CLK], reduce_or_output, port_taints[Q][taint_id], clk_polarity);
        new_ff->set_bool_attribute(ID(taint_ff));

    }

    return true;
}
