#include "kernel/register.h"
#include "kernel/rtlil.h"
#include "kernel/utils.h"
#include "kernel/log.h"
#include "kernel/yosys.h"

#include <iostream>
#include <string>


USING_YOSYS_NAMESPACE


/*
extern bool modift_set(RTLIL::Module *module, RTLIL::Cell *cell, unsigned int num_states, string state) {
    RTLIL::SigSpec sig_y = cell->getPort(ID::Y);
    int width = cell->getPort(ID::Y).size();
    //sig_y.extract(width-num_states,num_states);

    
    int index = state.find_first_of("1");
    RTLIL::SigSpec states_judge = sig_y.extract(index-1,1);
    RTLIL::SigSpec states_new = (RTLIL::SigBit(RTLIL::State::S0), width-index-1);
    states_new.append(RTLIL::SigBit(RTLIL::State::S1));
    states_new.append(RTLIL::SigBit(RTLIL::State::S1, index));
    
    if(sig_y[index-1].data == RTLIL::State::S1)
    {
        sig_y.remove(0,1);
        sig_y.append(RTLIL::SigBit(RTLIL::State::S0));
        cell->getPort(ID::Y).replace(width-num_states, sig_y);
        sig_y.replace(width-num_states, sig_y);
    }    
   
    module->addTribuf(NEW_ID, states_new, states_judge, states_new);
    module->addOr(NEW_ID, states_new, sig_y, cell->getPort(ID::Y));

    return true;
}
*/


// Checks whether the signal name is included in the exclude-signals command line argument.
bool is_signal_excluded(std::vector<string> *excluded_signals, string signal_name) {
    if (!signal_name.size())
        return false;

    // Remove the first character of the string.
    string name_to_compare = signal_name.substr(1, signal_name.size()-1);
    // log("Found: %d.", std::find(excluded_signals->begin(), excluded_signals->end(), name_to_compare) != excluded_signals->end());
    return std::find(excluded_signals->begin(), excluded_signals->end(), name_to_compare) != excluded_signals->end();
}

// Transforms an identifier name into the corresponding taint name.
std::string get_wire_taint_idstring(RTLIL::IdString id_string, unsigned int taint_id) {
    return id_string.str() + "_t" + std::to_string(taint_id);
}


// For a given SigSpec, returns the corresponding taint SigSpec.
std::vector<RTLIL::SigSpec> get_corresponding_taint_signals(RTLIL::Module* module, std::vector<string> *excluded_signals, const RTLIL::SigSpec &sig, unsigned int num_taints) {
    std::vector<RTLIL::SigSpec> ret(num_taints);
    int expand = 0;

    // Get a SigSpec for the corresponding taint signal for the given cell port, creating a new SigSpec if necessary.
    for (unsigned int taint_id = 0; taint_id < num_taints; taint_id++) {
        for (auto &chunk_it : sig.chunks()) {
            if (chunk_it.is_wire() && !is_signal_excluded(excluded_signals, chunk_it.wire->name.str())) {
                RTLIL::Wire *w = module->wire(get_wire_taint_idstring(chunk_it.wire->name.str(), taint_id));
                if (w == nullptr) {
                    w = module->addWire(get_wire_taint_idstring(chunk_it.wire->name.str(), taint_id), chunk_it.wire->width);
                    w->port_input = false;
                    w->port_output = false;
                }
                ret[taint_id].append(RTLIL::SigChunk(w, chunk_it.offset, chunk_it.width));
            }
            else
                ret[taint_id].append(RTLIL::SigChunk(RTLIL::State::S0, chunk_it.width));
                //ret[taint_id].append(RTLIL::SigBit(RTLIL::State::S0, 4));
        }  
        int sig_size = sig.size();
        log_assert(ret[taint_id].size() == sig_size);
    }
    return ret;
}

//for more sigchunk to one port sitution, the get_corresponding_label_signals can not deal with that;
RTLIL::SigSpec get_corresponding_port_signals(RTLIL::Module* module, std::vector<string> *excluded_signals, const RTLIL::SigSpec &sig, unsigned int num_taints) {
    RTLIL::SigSpec ret;

    // Get a SigSpec for the corresponding taint signal for the given cell port, creating a new SigSpec if necessary.
    //moudle\ports
    std::vector<RTLIL::SigSpec> taint_state_vector;
    for (auto &chunk_it : sig.chunks()) {
        RTLIL::Wire *w = module->wire(get_wire_taint_idstring(chunk_it.wire->name.str(), 0));
        taint_state_vector.insert(taint_state_vector.end(), RTLIL::SigSpec(w, w->width-state_map["state_num"], state_map["state_num"]));
    }
    ret = module->Or(NEW_ID, taint_state_vector[0], taint_state_vector[1]);
    for(int index = 2; index < sig.chunks().size(); ++index){
        ret = module->Or(NEW_ID, ret, taint_state_vector[index]);
    }

    return ret;
}


// For a given SigSpec, returns the corresponding Label SigSpec.
std::vector<RTLIL::SigSpec> get_corresponding_label_signals(RTLIL::Module* module, std::vector<string> *excluded_signals, const RTLIL::SigSpec &sig, unsigned int num_taints, unsigned int num_states) {
    std::vector<RTLIL::SigSpec> ret(num_taints);
    int expand = 0;

    // Get a SigSpec for the corresponding taint signal for the given cell port, creating a new SigSpec if necessary.
    for (unsigned int taint_id = 0; taint_id < num_taints; taint_id++) {

        std::vector<RTLIL::SigSpec> taint_state_vector;

        for (auto &chunk_it : sig.chunks()) {
            if(sig.chunks().size() == 1)
                expand = num_states;
            //if(chunk_it == sig.chunks().back())
              //  expand = 4;
            if (chunk_it.is_wire() && !is_signal_excluded(excluded_signals, chunk_it.wire->name.str())) {
                if(chunk_it.width < chunk_it.wire->width)
                    expand = 0;

                RTLIL::Wire *w = module->wire(get_wire_taint_idstring(chunk_it.wire->name.str(), taint_id));     
                if (w == nullptr) {
                    w = module->addWire(get_wire_taint_idstring(chunk_it.wire->name.str(), taint_id), chunk_it.wire->width + num_states);
                    w->port_input = false;
                    w->port_output = false;
                }
                //if (w->width == chunk_it.wire->width)
                  //w.append(num_taint)

                ret[taint_id].append(RTLIL::SigChunk(w, chunk_it.offset, chunk_it.width + expand));
                if(sig.chunks().size() != 1)
                    taint_state_vector.insert(taint_state_vector.end(), RTLIL::SigSpec(w, w->width-state_map["state_num"], state_map["state_num"]));
                if(sig.chunks().size() == 1 && chunk_it.width < chunk_it.wire->width)
                    ret[taint_id].append(RTLIL::SigChunk(w, w->width-state_map["state_num"], state_map["state_num"]));
            }
            else{
                ret[taint_id].append(RTLIL::SigChunk(RTLIL::State::S0, chunk_it.width + expand));
                //ret[taint_id].append(RTLIL::SigBit(RTLIL::State::S0, 4));
                if(sig.chunks().size() != 1)
                    taint_state_vector.insert(taint_state_vector.end(), RTLIL::SigSpec(RTLIL::State::S0, state_map["state_num"]));
            }
        }  
        if(sig.chunks().size() > 1){
            RTLIL::SigSpec state_append;
            state_append = module->Or(NEW_ID, taint_state_vector[0], taint_state_vector[1]);
            for(int index = 2; index < sig.chunks().size(); ++index){
                state_append = module->Or(NEW_ID, state_append, taint_state_vector[index]);
            }
            //ret[taint_id].append(RTLIL::SigChunk(state_append));
            ret[taint_id].append(state_append);
        }

        int sig_size = sig.size()+ num_states;  
            log_assert(ret[taint_id].size() == sig_size);
    }
    return ret;
}




RTLIL::SigSpec get_mod_state_by_index(int index, int mod_num){
    RTLIL::SigSpec mod_map_sigspec;
    mod_map_sigspec = RTLIL::SigSpec(RTLIL::State::S0, mod_num-index-1);
    mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S1, 1));
    mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S0, index));
    return mod_map_sigspec;
}



RTLIL::SigSpec get_mod_state(string front_mod, int mod_num){
    RTLIL::SigSpec mod_map_sigspec;
    if(front_mod.front() == 'A' || front_mod.find("1") == mod_num - 1){
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S1, 1));
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S0, mod_num-1));
    }
    else if(front_mod.front() == 'B' || front_mod.find("1") == mod_num - 2){
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S0, 1));
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S1, 1));
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S0, mod_num-2));
    }
    else if(front_mod.front() == 'C' || front_mod.find("1") == mod_num - 3){
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S0, 2));
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S1, 1));
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S0, mod_num-3));
    }
    else if(front_mod.front() == 'D' || front_mod.find("1") == mod_num - 4){
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S0, 3));
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S1, 1));
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S0, mod_num-4));
    }
    else if(front_mod.front() == 'E' || front_mod.find("1") == mod_num - 5){
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S0, 4));
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S1, 1));
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S0, mod_num-5));
    }
    else
        //log_id("Baby, too much stat\n");
        printf("Baby, too much stat\n");
    return mod_map_sigspec;
}

RTLIL::SigSpec check_list(RTLIL::Module* module, RTLIL::SigSpec y_states, std::set<RTLIL::SigSig> Enter_Check_List){
          //for given input y_states[y_states_loop : y_states_mod], check it leagal
            RTLIL::SigSpec input_loop = y_states.extract_end(state_map["mod_num"]);
            RTLIL::SigSpec input_mod = y_states.extract(0, state_map["mod_num"]);
            
            
            //iterator all check list
            RTLIL::SigSpec loop_check;
            RTLIL::SigSpec mod_check;
            RTLIL::SigSpec check_part;
            RTLIL::SigSpec check_list;

            for(auto iter : Enter_Check_List){
                if(iter.first.size() == 0){
                    loop_check = RTLIL::SigSpec(Yosys::State::S1);
                    mod_check = module->Eq(NEW_ID, input_mod, iter.second);
                }
                else if(iter.first.is_fully_const()){
                    loop_check = module->Ge(NEW_ID, iter.first, input_loop);
                    mod_check = module->Eq(NEW_ID, input_mod, iter.second);
                }
                else if(iter.first.is_chunk()){
                    loop_check = module->Eq(NEW_ID, iter.first, input_loop);
                    mod_check = module->Eq(NEW_ID, input_mod, iter.second);
                }
                check_list.append(module->And(NEW_ID, loop_check, mod_check));
            }// all check ,if pass set 1, else set 0;
    RTLIL::SigSpec pass = module->Gt(NEW_ID, check_list, RTLIL::SigSpec(0, check_list.size()));
    return pass;
}

bool first_after_loop(string attr){
    int index = STM_express.find_first_of("*");
    if(STM_express.at(index+3) == ('A' + state_map["mod_num"] - attr.find("1") - 1))
        return true;
    else
        return false;
}

RTLIL::SigSig Check_and_Refresh(RTLIL::Module* module, RTLIL::SigSpec y_states, string attr){
    RTLIL::SigSpec loop_check;
    RTLIL::SigSpec mod_check;
    //check_policy 1 : mod le; loop eq;
    //check_policy 2 : mod le; loop ge;
    RTLIL::SigSpec refresh_policy1;//old_loop->new_loop; old_mod or attr->new_mod;
    RTLIL::SigSpec refresh_policy5;//old_loop->new_loop; attr->new_mod;
    RTLIL::SigSpec refresh_policy2;//old_loop+1->new_loop; attr->new_mod;
    RTLIL::SigSpec refresh_policy3;//old_loop->new_loop; old_mod or attr->new_mod;//add taint_to_zero operation @
    //RTLIL::SigSpec refresh_policy4;//old_loop+1->new_loop; attr->new_mod;//add taint_to_zero operation
    RTLIL::SigSpec refresh_policy4;//old_loop->new_loop; attr->new_mod;//add taint_to_zero operation
    RTLIL::SigSpec refresh_sele;//ZZZ or refresh result

    RTLIL::SigSpec loop_refresh;
    RTLIL::SigSpec mod_refresh;


    std::set<RTLIL::SigSig> Enter_Check_List;
    RTLIL::SigSpec pass; 
    int index = mod_policy_index.find(state_map["mod_num"] - attr.find("1") - 1 + '0');
    Enter_Check_List = Yosys::mod_policy[index];

    for(auto check_item : Enter_Check_List){
        mod_check = module->Le(NEW_ID, check_item.first.extract_end(state_map["loop_num"]), y_states.extract(0, state_map["mod_num"]));

   
        ////////////
        if(check_item.second.extract(0,5).as_int(false) == 1){
            loop_check = module->Eq(NEW_ID, check_item.first.extract(0,state_map["loop_num"]), y_states.extract_end(state_map["mod_num"]));
        }
        else if(check_item.second.extract(0,5).as_int(false) == 2){
            loop_check = module->Ge(NEW_ID, check_item.first.extract(0,state_map["loop_num"]), y_states.extract_end(state_map["mod_num"]));
        }
        else if(state_map["loop_num"]==0)
            loop_check = SigSpec(RTLIL::SigSpec(RTLIL::State::S1, 1));

        RTLIL::SigSpec check_loop_mod = module->And(NEW_ID, loop_check, mod_check);
        //SigSpec check = module->ReduceOr(NEW_ID, loop_check.append(mod_check));
        if(check_item.second.extract_end(5).as_int(false) == 1)
            refresh_policy1.append(check_loop_mod);//reduce_or can replace and gate
        else if(check_item.second.extract_end(5).as_int(false) == 2)
            refresh_policy2.append(check_loop_mod);
        else if(check_item.second.extract_end(5).as_int(false) == 3)
            refresh_policy3.append(check_loop_mod);
        else if(check_item.second.extract_end(5).as_int(false) == 4)
            refresh_policy4.append(check_loop_mod);
        
        
        /*
        if(check_item.second.extract_end(5).as_int(false) == 1){
            loop_refresh = y_states.extract_end(state_map["mod_num"]);
            mod_refresh = module->Or(NEW_ID, y_states.extract(0, state_map["mod_num"]), get_mod_state(attr, state_map["mod_num"]));
        }
        else if(check_item.second.extract_end(5).as_int(false) == 2){
            loop_refresh = module->Add(NEW_ID, y_states.extract_end(state_map["mod_num"]), RTLIL::SigSpec(1));
            mod_refresh = get_mod_state(attr, state_map["mod_num"]);
        }
        else if(check_item.second.extract_end(5).as_int(false) == 3){
            loop_refresh = y_states.extract_end(state_map["mod_num"]);
            mod_refresh = get_mod_state(attr, state_map["mod_num"]);
        }
        else
            printf("out of refresh policy stage");
        */
    }


//**

    RTLIL::SigSpec pass_1 = module->Gt(NEW_ID, refresh_policy1, RTLIL::SigSpec(0, refresh_policy1.size()));
    RTLIL::SigSpec pass_2 = module->Gt(NEW_ID, refresh_policy2, RTLIL::SigSpec(0, refresh_policy2.size()));
    RTLIL::SigSpec pass_3 = module->Gt(NEW_ID, refresh_policy3, RTLIL::SigSpec(0, refresh_policy3.size()));
    RTLIL::SigSpec pass_4 = module->Gt(NEW_ID, refresh_policy4, RTLIL::SigSpec(0, refresh_policy4.size()));
        

    //RTLIL::SigSpec policy_1_2_3 = pass_3.append(pass_2.append(pass_1));
    refresh_sele.append(refresh_policy1);
    refresh_sele.append(refresh_policy2);
    refresh_sele.append(refresh_policy3);
    refresh_sele.append(refresh_policy4);
    //RTLIL::SigSpec pass_all = module->Gt(NEW_ID, refresh_sele, RTLIL::SigSpec(0, refresh_policy1.size()));//_195_ refresh sele only one bit, maybe the size problem
    RTLIL::SigSpec pass_all = module->Gt(NEW_ID, refresh_sele, RTLIL::SigSpec(0, refresh_sele.size()));//_044_


        //policy_1
        //loop_refresh_1 = y_states.extract_end(state_map["mod_num"]);
        //mod_refresh_1 = module->Or(NEW_ID, y_states.extract(0, state_map["mod_num"]), get_mod_state(attr, state_map["mod_num"]));

        //policy_2
        //loop_refresh_2 = module->Add(NEW_ID, y_states.extract_end(state_map["mod_num"]), RTLIL::SigSpec(1));
        //mod_refresh_2 = get_mod_state(attr, state_map["mod_num"]);

        //policy_3
        //loop_refresh_3 = y_states.extract_end(state_map["mod_num"]);
        //mod_refresh_3 = get_mod_state(attr, state_map["mod_num"]);

        //compare 2\3 by p3, then then result compare 1 by p1;
    mod_refresh = module->Mux(NEW_ID,  module->Or(NEW_ID, y_states.extract(0, state_map["mod_num"]), get_mod_state(attr, state_map["mod_num"])), get_mod_state(attr, state_map["mod_num"]), module->Or(NEW_ID, pass_2, pass_4));
    if(state_map["loop_num"] > 0){
        //RTLIL::SigSpec loop_refresh_tep = module->Mux(NEW_ID, module->Add(NEW_ID, y_states.extract_end(state_map["mod_num"]), RTLIL::SigSpec(1, state_map["mod_num"])), y_states.extract_end(state_map["mod_num"]), pass_3);
        loop_refresh = module->Mux(NEW_ID, y_states.extract_end(state_map["mod_num"]), module->Add(NEW_ID, y_states.extract_end(state_map["mod_num"]), RTLIL::SigSpec(1, state_map["mod_num"])), pass_2);
    }
    
 
    ////
    mod_refresh.append(RTLIL::SigSpec(RTLIL::State::S0, state_map["loop_num"]));
    RTLIL::SigSpec output_loop = RTLIL::SigSpec(RTLIL::State::S0, state_map["mod_num"]);
    output_loop.append(loop_refresh);
    RTLIL::SigSpec y_states_refresh = module->Or(NEW_ID, output_loop, mod_refresh);//output_loop.append(mod_refresh);

    y_states = module->Mux(NEW_ID, RTLIL::SigSpec(RTLIL::State::Sz, state_map["state_num"]), y_states_refresh, pass_all);

    
    RTLIL::SigSpec taint_refresh = module->Or(NEW_ID, pass_3, pass_4);
    
    RTLIL::SigSig check_fresh_ret = make_pair(taint_refresh, y_states);

    return check_fresh_ret;

}