#include "kernel/rtlil.h"
#include "kernel/utils.h"
#include "kernel/log.h"
#include "kernel/yosys.h"
#include <string>
#include <algorithm>


USING_YOSYS_NAMESPACE
extern RTLIL::IdString Yosys::mod_flag;
RTLIL::IdString Yosys::loop_front;
extern std::vector<RTLIL::SigSpec> get_corresponding_taint_signals(RTLIL::Module* module, std::vector<string> *excluded_signals, const RTLIL::SigSpec &sig, unsigned int num_taints);
extern std::vector<RTLIL::SigSpec> get_corresponding_label_signals(RTLIL::Module* module, std::vector<string> *excluded_signals, const RTLIL::SigSpec &sig, unsigned int num_taints, unsigned int num_states);
extern RTLIL::SigSpec get_mod_state(string front_mod, int mod_num);
extern RTLIL::SigSpec check_list(RTLIL::Module* module, RTLIL::SigSpec y_states_output, std::set<RTLIL::SigSig> Enter_Check_List);
extern RTLIL::SigSig Check_and_Refresh(RTLIL::Module* module, RTLIL::SigSpec y_states, string attr);
extern std::string STM_express;
extern std::map<std::string, int> state_map;
vector<std::set<RTLIL::SigSig>> Yosys::mod_policy;
std::string mod_policy_index;


/**
 * @param module the current module instance
 * @param cell the current cell instance
*
 * @return keep_current_cell
 */
bool cellift_and(RTLIL::Module *module, RTLIL::Cell *cell, unsigned int num_taints, unsigned int num_states, std::vector<string> *excluded_signals) {

    //const RTLIL::IdString mod_flag = ID(mod_flag);
    const unsigned int A = 0, B = 1, Y = 2;
    const unsigned int NUM_PORTS = 3;
    RTLIL::SigSpec ports[NUM_PORTS] = {cell->getPort(ID::A), cell->getPort(ID::B), cell->getPort(ID::Y)};
    std::vector<RTLIL::SigSpec> port_taints[NUM_PORTS];

    // Give to all inputs the same size as the output.
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

    for (unsigned int i = 0; i < NUM_PORTS; ++i)
        port_taints[i] = get_corresponding_label_signals(module, excluded_signals, ports[i], num_taints, num_states);


    for (unsigned int taint_id = 0; taint_id < num_taints; taint_id++) {
       
        if(cell->get_bool_attribute(ID(untaint_area)) == true){
            module->connect(port_taints[A][taint_id], port_taints[Y][taint_id]);
        }
        else if(cell->get_string_attribute(ID(const_opt)) == "B"){
            RTLIL::SigSpec a_taint_and_b = module->And(NEW_ID, port_taints[A][taint_id].extract(0, output_width), extended_b);
            //RTLIL::SigSpec b_taint_and_a = module->And(NEW_ID, port_taints[B][taint_id].extract(0, output_width), extended_a); 00000
            //RTLIL::SigSpec a_taint_and_b_taint = module->And(NEW_ID, port_taints[A][taint_id].extract(0, output_width), port_taints[B][taint_id].extract(0, output_width)); 00000
            //RTLIL::SigSpec a_taint_and_b_or_reverse = module->Or(NEW_ID, a_taint_and_b, b_taint_and_a); //a_taint_and_b
            //RTLIL::SigSpec y_taint_extend = module->Or(NEW_ID, a_taint_and_b_or_reverse, a_taint_and_b_taint); //a_taint_and_b
            //module->addOr(NEW_ID, a_taint_and_b_or_reverse, a_taint_and_b_taint, port_taints[Y][taint_id]);
            module->connect(a_taint_and_b, port_taints[Y][taint_id]);
            
        }
        else if(cell->get_string_attribute(ID(const_opt)) == "B"){
            RTLIL::SigSpec b_taint_and_a = module->And(NEW_ID, port_taints[B][taint_id].extract(0, output_width), extended_a);
            module->connect(b_taint_and_a, port_taints[Y][taint_id]);
        }
        else{
        RTLIL::SigSpec extended_a_taint = port_taints[A][taint_id].extract(0, output_width);
        //extended_a_taint.remove(output_width,num_states);
        RTLIL::SigSpec extended_b_taint = port_taints[B][taint_id].extract(0, output_width);
        //extended_b_taint.remove(output_width,num_states);

        if (ports[A].size() < output_width)
            extended_a_taint.append(RTLIL::SigSpec(RTLIL::State::S0, output_width-ports[A].size() + num_states));
        else if (ports[A].size() > output_width)
            extended_a_taint.extract(0, output_width);
        if (ports[B].size() < output_width)
            extended_b_taint.append(RTLIL::SigSpec(RTLIL::State::S0, output_width-ports[B].size() + num_states));
        else if (ports[B].size() > output_width)
            extended_b_taint.extract(0, output_width);
   
        //caculate the vlaue of taints, and extend with after_zero;  
        RTLIL::SigSpec a_taint_and_b = module->And(NEW_ID, extended_a_taint, extended_b);
        RTLIL::SigSpec b_taint_and_a = module->And(NEW_ID, extended_b_taint, extended_a);
        RTLIL::SigSpec a_taint_and_b_taint = module->And(NEW_ID, extended_a_taint, extended_b_taint);
        RTLIL::SigSpec a_taint_and_b_or_reverse = module->Or(NEW_ID, a_taint_and_b, b_taint_and_a);
        RTLIL::SigSpec y_taint_extend = module->Or(NEW_ID, a_taint_and_b_or_reverse, a_taint_and_b_taint);
        module->addOr(NEW_ID, a_taint_and_b_or_reverse, a_taint_and_b_taint, port_taints[Y][taint_id]);
        }
        //y_taint_extend.append(RTLIL::SigSpec(RTLIL::State::S0, num_states));
        
        
        //caculate the vlaue of states by bits
        /*
        RTLIL::SigSpec extended_a_states = port_taints[A][taint_id];
        RTLIL::SigSpec extended_b_states = port_taints[B][taint_id];
        std::vector<RTLIL::SigBit> a_bits = extended_a_states.bits();
        std::vector<RTLIL::SigBit> b_bits = extended_b_states.bits();
        std::vector<RTLIL::SigBit>::const_iterator a_extend_first = a_bits.begin() + output_width;
        std::vector<RTLIL::SigBit>::const_iterator a_extend_last= a_bits.end();
        std::vector<RTLIL::SigBit>::const_iterator b_extend_first = b_bits.begin() + output_width;
        std::vector<RTLIL::SigBit>::const_iterator b_extend_last = b_bits.end();

        std::vector<RTLIL::SigBit> a_n_extend;
        a_n_extend.assign(a_extend_first,a_extend_last);
        std::vector<RTLIL::SigBit> b_n_extend;
        b_n_extend.assign(b_extend_first,b_extend_last);

        RTLIL::SigSpec a_extend = RTLIL::SigSpec(a_n_extend);
        RTLIL::SigSpec b_extend = RTLIL::SigSpec(b_n_extend);
        RTLIL::SigSpec sele_extend = module->Ge(NEW_ID,a_extend,b_extend);
        RTLIL::SigSpec y_extend = module->Mux(NEW_ID,a_extend,b_extend,sele_extend);
        */


        //caculate the vlaue of states by SigSpece way, and extend with before_zero
        RTLIL::SigSpec a_states = port_taints[A][taint_id].extract_end(output_width);
        RTLIL::SigSpec b_states = port_taints[B][taint_id].extract_end(output_width);
        
        //method_1
        //RTLIL::SigSpec sele_extend = module->Ge(NEW_ID, a_states, b_states);
        //RTLIL::SigSpec y_states = module->Mux(NEW_ID, b_states, a_states, sele_extend);
        //method_2
        //RTLIL::SigSpec y_states_output = module->Or(NEW_ID, a_states, b_states);
        
        //whether the state value should be updated

        string attr = cell->get_string_attribute(mod_flag);
        
        if(cell->get_string_attribute(mod_flag) != "")
            port_taints[Y][taint_id].append(Check_and_Refresh(module, a_states, attr).second);
        //extend Check_and_Refresh parameter; accept all input signal of cell, and decide the change of path label;

        port_taints[Y][taint_id].append(a_states);
        //RTLIL::SigSpec y_state_extend = RTLIL::SigSpec(RTLIL::State::S0, output_width);
        //y_state_extend.append(y_states_output);

        //output the y_extend to port[Y]
        //module->addOr(NEW_ID, a_taint_and_b_or_reverse, a_taint_and_b_taint, port_taints[Y][taint_id]);
        //module->addOr(NEW_ID, y_taint_extend, y_state_extend, port_taints[Y][taint_id]);
    }
    return true;
}
/*
if(cell->get_string_attribute(block_flag) && cell->get_bool_attribute(block_begin)){
    sig = get_block_ioput(cell->get_string_attribute);

    block_tag_input = module->wire(tag_id(sig.first)).extract_end(output_width);
    if(block_tag_input == null)
        block_tag_input = module->AddWire(block_tag_id(sig.second));
        module->wire(tag_id(sig.first)).append(block_tag_input);
    
    block_tag_output = module->wire(tag_id(sig.second)).extract_end(output_width); 
    if(block_tag_output == null)
        block_tag_output = module->AddWire(block_tag_id(sig.first));

    for(auto check_fresh_rule : get_block_check_fresh(cell->get_string_attribute)){
        //for(auto check_it = 1; check_it <= check_fresh_rule.first.size(); check_it++)
        block_check = module->addEq(NEW_ID, block_tag_input.extract_end(loop), check_fresh_rule.first.extract_end(loop));
        loop_check = module->addEq(NEW_ID, block_tag_input.extract(loop), check_fresh_rule.first.extract(loop));
        check = module->addAnd(NEW_ID, loop_check, block_check);
        //all check item(chekc.reducland == 1)is pass ,than refresh corresponding refresh rule;
        block_refresh = check_fresh_rule.second.extract_end(loop);
        loop_refresh = check_fresh_rule.second.extract(loop);
        block_tag_output = module->addMux(NEW_ID, block_tag_input, loop_refresh.append(block_refresh), check);
        blocl_tag_input = block_tag_output;
    }

    output_tag.push_back(tag_id(sig.second), block_tag_output);
}

if(cell->get_string_attribute(block_flag) && cell->get_bool_attribute(block_end))
{
    if(block_tag_output == null)
        log_error("without block_tag_output");
    block_tag_output = output_tag.find(block_tag_id(sig.second));
    block_tag_output.append(y_taint_extend);
    module->connect(port_taints[Y][taint_id].append(RTLIL::SigSpec(RTLIL::State::S0, path)).extract_end(), block_tag_output);
}
*/