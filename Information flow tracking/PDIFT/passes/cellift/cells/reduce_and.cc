#include "kernel/rtlil.h"
#include "kernel/utils.h"
#include "kernel/log.h"
#include "kernel/yosys.h"

USING_YOSYS_NAMESPACE
extern std::vector<RTLIL::SigSpec> get_corresponding_taint_signals(RTLIL::Module* module, std::vector<string> *excluded_signals, const RTLIL::SigSpec &sig, unsigned int num_taints);
extern std::vector<RTLIL::SigSpec> get_corresponding_label_signals(RTLIL::Module* module, std::vector<string> *excluded_signals, const RTLIL::SigSpec &sig, unsigned int num_taints, unsigned int num_states);
extern RTLIL::SigSig Check_and_Refresh(RTLIL::Module* module, RTLIL::SigSpec y_states, string attr);
extern RTLIL::SigSpec get_corresponding_port_signals(RTLIL::Module* module, std::vector<string> *excluded_signals, const RTLIL::SigSpec &sig, unsigned int num_taints);

/**
 * @param module the current module instance
 * @param cell the current cell instance
*
 * @return keep_current_cell
 */
bool cellift_reduce_and(RTLIL::Module *module, RTLIL::Cell *cell, unsigned int num_taints, std::vector<string> *excluded_signals) {

    const unsigned int A = 0, Y = 1;
    const unsigned int NUM_PORTS = 2;
    RTLIL::SigSpec ports[NUM_PORTS] = {cell->getPort(ID::A), cell->getPort(ID::Y)};
    std::vector<RTLIL::SigSpec> port_taints[NUM_PORTS];

    for (unsigned int i = 0; i < NUM_PORTS; ++i)
        port_taints[i] = get_corresponding_label_signals(module, excluded_signals, ports[i], num_taints, state_map["state_num"]);

    for (unsigned int taint_id = 0; taint_id < num_taints; taint_id++) {
        RTLIL::SigSpec reduce_or_taint = module->ReduceOr(NEW_ID, port_taints[A][taint_id].extract(0, ports[A].size()));

        // Check that all the zero-valued bits are tainted.
        RTLIL::SigSpec a_or_a_taint = module->Or(NEW_ID, ports[A], port_taints[A][taint_id].extract(0, ports[A].size()));
        RTLIL::SigSpec and_reduced = module->ReduceAnd(NEW_ID, a_or_a_taint);

        module->addAnd(NEW_ID, reduce_or_taint, and_reduced, port_taints[Y][taint_id][0]);
        //RTLIL::SigSpec y_taint_extend = module->And(NEW_ID, reduce_or_taint, and_reduced);
        //y_taint_extend.append(RTLIL::SigSpec(RTLIL::State::S0, state_map["state_num"]));
        //RTLIL::SigSpec y_state_extend = RTLIL::SigSpec(RTLIL::State::S0, ports[A].size());
        //y_state_extend.append(port_taints[A][taint_id].extract_end(ports[A].size()));
        //port_taints[Y][taint_id] = port_taints[Y][taint_id].extract(0, ports[Y].size()).append(port_taints[A][taint_id].extract_end(ports[A].size()));

        //module->addOr(NEW_ID, y_taint_extend, y_state_extend, port_taints[Y][taint_id]);
        

        // For the other bits, taint the output as a constant.
        if (ports[Y].size() > 1)
            module->connect(port_taints[Y][taint_id].extract(1, ports[Y].size()-1), RTLIL::SigSpec(RTLIL::State::S0, ports[Y].size()-1));

        module->connect(port_taints[Y][taint_id].extract_end(ports[Y].size()), port_taints[A][taint_id].extract_end(ports[A].size()));
        }

    return true;
}
