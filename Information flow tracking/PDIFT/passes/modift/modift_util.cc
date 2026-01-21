#include "kernel/register.h"
#include "kernel/rtlil.h"
#include "kernel/utils.h"
#include "kernel/log.h"
#include "kernel/yosys.h"
#include <set>
#include <iostream>
#include <string>
#include <map>

USING_YOSYS_NAMESPACE

char front_mod = '0';


RTLIL::SigSpec get_front_mod_state(char front_mod, int mod_num){
    RTLIL::SigSpec mod_map_sigspec;
    if(front_mod == 'A'){
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S1, 1));
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S0, mod_num-1));
    }
    else if(front_mod == 'B'){
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S0, 1));
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S1, 1));
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S0, mod_num-2));
    }
    else if(front_mod == 'C'){
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S0, 2));
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S1, 1));
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S0, mod_num-3));
    }
    else if(front_mod == 'D'){
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S0, 3));
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S1, 1));
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S0, mod_num-4));
    }
    else if(front_mod == 'E'){
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S0, 4));
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S1, 1));
        mod_map_sigspec.append(RTLIL::SigSpec(RTLIL::State::S0, mod_num-5));
    }
    else
        //log_id("Baby, too much stat\n");
        printf("Baby, too much stat\n");
    return mod_map_sigspec;
}

char find_the_front_mod(int index){
    for(int i = index - 1; i >= 0; i--)
    {
        if( STM_express.at(i) >= 'A'){
            front_mod = STM_express.at(i);
            break;
        }
        else
            continue;
    }
    return front_mod;
}

/*
extern std::set<RTLIL::SigSig> get_mod_CheckingList(string attr){
            //check all fronter mod of the select mod, and collect into the pool [CheakList_loopmod]
            //typedef std::pair<SigSpec, SigSpec> SigSig;
            extern string STM_express;
            extern std::map<std::string, int> state_map;
            std::set<SigSig> CheakList_loopmod;
            //pool<std::string, std::string> FreshList_loopmod;
            RTLIL::SigSpec loop_list;
            RTLIL::SigSpec mod_list;
            char mod = state_map["mod_num"] - attr.find_first_of("1") - 1 + 'A';
            //char front_mod; turn to globle
            for(int it = 0; it < STM_express.length(); it++){
                if(STM_express.at(it) != mod)
                    continue;

                //analysis the behaiver of mod S in the STM_express
                //like S + .....
                if(it == 0){
                    mod_list.append(RTLIL::SigSpec(RTLIL::State::S0, state_map.at("mod_num"))); //mod
                    loop_list.append(RTLIL::SigSpec(RTLIL::State::S0, state_map.at("loop_num"))); //loop
                    CheakList_loopmod.insert(make_pair(loop_list, mod_list));
                    mod_list.empty();
                    loop_list.empty();
                }
                //like a + b +(c? + S + d?)*3 + e; S inside in the loop()
                else if(STM_express.rfind("(", it) != std::string::npos && STM_express.rfind(")",it) == std::string::npos){
                    int begin_of_loop = STM_express.rfind("(", it);
                    int end_of_loop = STM_express.find_first_of(")", it);
                    //int index_of_mul = STM_express.find_first_of("*", it);
                    int loop = STM_express.at(end_of_loop + 2) - '0';
                    if(it == begin_of_loop + 1){//..(S+a+...) S is begin of loop
                        //a+b+(S+c+d) 
                        if(find_the_front_mod(begin_of_loop) != '0'){
                            //front_mod = STM_express.at(STM_express.rfind("ABCDE", begin_of_loop));
                            mod_list = get_front_mod_state(front_mod, state_map["mod_num"]);
                            loop_list.append(RTLIL::SigSpec(RTLIL::State::S0, state_map["loop_num"]));
                            CheakList_loopmod.insert(make_pair(loop_list, mod_list));
                            loop_list.empty();
                            mod_list.empty();
                        }
                        //(S+c+d)
                        else{
                            loop_list.append(RTLIL::SigSpec(RTLIL::State::S0, state_map["loop_num"]));
                            mod_list.append(RTLIL::SigSpec(RTLIL::State::S0, state_map["mod_num"]));
                            CheakList_loopmod.insert(make_pair(loop_list, mod_list));
                            loop_list.empty();
                            mod_list.empty();
                        }
                        //the last mod of loop can enter S
                        front_mod = find_the_front_mod(end_of_loop);
                        mod_list = get_front_mod_state(front_mod, state_map["mod_num"]);
                        loop_list.append(RTLIL::SigSpec(loop));
                        CheakList_loopmod.insert(make_pair(loop_list, mod_list));
                        loop_list.empty();
                        mod_list.empty();
                        //S is the front mod of loop(), it need add loop count in the fresh stage
                        //cell->set_string_attribute(loop_front, STM_express.substr(index_of_mul+1, 1));
                    }
                    else{
                        front_mod = find_the_front_mod(it);
                        mod_list = get_front_mod_state(front_mod, state_map["mod_num"]);
                        loop_list.append(RTLIL::SigSpec(loop));
                        CheakList_loopmod.insert(make_pair(loop_list, mod_list));
                        loop_list.empty();
                        mod_list.empty();
                    }
                }
                //like a + (b + c)*3 + S + ...; before S has a loop()
                else if(STM_express.rfind("*", it) != std::string::npos){
                    int index_of_mul = STM_express.rfind("*", it);
                    int loop = STM_express.at(index_of_mul+1) - '0';
                    front_mod = find_the_front_mod(it);
                    mod_list = get_front_mod_state(front_mod, state_map["mod_num"]);
                    loop_list.append(RTLIL::SigSpec(RTLIL::State::S0, state_map["loop_num"]));
                    CheakList_loopmod.insert(make_pair(loop_list, mod_list));
                    loop_list.empty();
                    mod_list.empty();
                }
                //like a+ b + S + d, without loop
                else if(find_the_front_mod(it) != '0'){
                    //front_mod = STM_express.at(STM_express.rfind("ABCDE", it));
                    mod_list = get_front_mod_state(front_mod, state_map["mod_num"]);
                    loop_list.append(RTLIL::SigSpec(RTLIL::State::S0, state_map["loop_num"]));
                    CheakList_loopmod.insert(make_pair(loop_list, mod_list));
                    loop_list.empty();
                    mod_list.empty();
                }
                //if(it == STM_express.rfind("ABCDE"))    
                //S is the end mod of STM_express
                //    cell->set_bool_attribute(express_end, true);
            }
            return CheakList_loopmod;
}
*/

extern std::set<RTLIL::SigSig> get_mod_CheckingList(string attr){
    //check all fronter mod of the select mod, and collect into the pool [CheakList_loopmod]
    //typedef std::pair<SigSpec, SigSpec> SigSig;
    std::set<SigSig> state_checklist;
            
    RTLIL::SigSpec loop_mod_list;
    RTLIL::SigSpec check_refresh_list;
    //<sigsig> loop:mod, check:refresh
            
    char mod = state_map["mod_num"] - attr.find_first_of("1") - 1 + 'A';
    
    for(int it = 0; it < STM_express.length(); it++){
        if(STM_express.at(it) != mod)
            continue;

        //loop:0000, mod:front  a+b+c.....      //logic check and refresh 
        if(STM_express.rfind("(", it) == std::string::npos){
            loop_mod_list = RTLIL::SigSpec(RTLIL::State::S0, state_map["loop_num"]);
            if(it == 0){
                loop_mod_list.append(RTLIL::SigSpec(RTLIL::State::S0, state_map["mod_num"]));
                //state_checklist.insert(make_pair(RTLIL::SigSpec(RTLIL::State::S0, state_map["loop_num"]), RTLIL::SigSpec(RTLIL::State::S0, state_map["mod_num"])));    
            }
            else{
                loop_mod_list.append(get_front_mod_state(find_the_front_mod(it),state_map["mod_num"]));
            }
                //state_checklist.insert(make_pair(RTLIL::SigSpec(RTLIL::State::S0, state_map["loop_num"]), get_front_mod_state(find_the_front_mod(it),state_map["mod_num"])));
            check_refresh_list = RTLIL::SigSpec(1, 5); //check mod : loop eq , mod le
            check_refresh_list.append(RTLIL::SigSpec(1, 5)); //refresh mod : policy 1 defined in cellift_util.cc 
            state_checklist.insert(make_pair(loop_mod_list, check_refresh_list));
        }
        //loop:0/n, mod:front   a+/ (a+b+c...
        else if(STM_express.rfind("(", it) != std::string::npos && STM_express.rfind(")",it) == std::string::npos){
            if(it == STM_express.rfind("(", it) + 1){ // a+(S+....) //first mod in the loop
                loop_mod_list = RTLIL::SigSpec(RTLIL::State::S0, state_map["loop_num"]);
                if(find_the_front_mod(STM_express.rfind("(", it)) != '0' && STM_express.rfind("(", it) != 0){
                    loop_mod_list.append(get_front_mod_state(find_the_front_mod(it),state_map["mod_num"]));
                    //state_checklist.insert(make_pair(RTLIL::SigSpec(0), get_front_mod_state(find_the_front_mod(it),state_map["mod_num"])));
                }
                else{
                    loop_mod_list.append(RTLIL::SigSpec(RTLIL::State::S0, state_map["mod_num"]));
                    //state_checklist.insert(make_pair(RTLIL::SigSpec(0), RTLIL::SigSpec(RTLIL::State::S0, state_map["mod_num"])));
                }
                check_refresh_list = RTLIL::SigSpec(1, 5); //check mod is loop eq, mod le;
                check_refresh_list.append(RTLIL::SigSpec(2, 5)); //refresh mod : policy 2 defined in cellift_util.cc
                state_checklist.insert(make_pair(loop_mod_list, check_refresh_list));
                //state_checklist.insert(make_pair(RTLIL::SigSpec(state_map["loop"]-1), get_front_mod_state(find_the_front_mod(STM_express.find_first_of("*",it)), state_map["mod_num"])));

                // the last mod in the loop can enter the first mod
                loop_mod_list = RTLIL::SigSpec(state_map["loop"] - 1, state_map["loop_num"]);
                loop_mod_list.append(get_front_mod_state(find_the_front_mod(STM_express.find_first_of(")", it)),state_map["mod_num"]));
                check_refresh_list = RTLIL::SigSpec(2, 5); //check mod is loop le, mod le;
                check_refresh_list.append(RTLIL::SigSpec(2, 5)); //refresh mod : policy 2 defined in cellift_util.cc
                state_checklist.insert(make_pair(loop_mod_list, check_refresh_list));
            }
            else{ // not the first mod in the loop
                loop_mod_list = RTLIL::SigSpec(state_map["loop"], state_map["loop_num"]);
                loop_mod_list.append(get_front_mod_state(find_the_front_mod(it),state_map["mod_num"]));
                check_refresh_list = RTLIL::SigSpec(2, 5); //check mod is loop le, mod le;
                check_refresh_list.append(RTLIL::SigSpec(1, 5)); //refresh mod : policy 1 defined in cellift_util.cc
                state_checklist.insert(make_pair(loop_mod_list, check_refresh_list));
                //state_checklist.insert(make_pair(RTLIL::SigSpec(state_map["loop"]), get_front_mod_state(find_the_front_mod(it),state_map["mod_num"])));
            }
        }
        //loop:nnnn, mod:front   (a+b+c)*7 + S  / +....+ S
        else if(STM_express.rfind(")",it) != std::string::npos){
            //loop_mod_list = RTLIL::SigSpec(state_map["loop"] + 1, state_map["loop_num"]);
            //loop_mod_list.append(get_front_mod_state(find_the_front_mod(it),state_map["mod_num"]));
            //check_refresh_list = RTLIL::SigSpec(2, 5); //check mod is loop le, mod le;
            if(it == STM_express.rfind(")") + 4 && it < STM_express.size()-1){
                loop_mod_list = RTLIL::SigSpec(state_map["loop"], state_map["loop_num"]);
                loop_mod_list.append(get_front_mod_state(find_the_front_mod(it),state_map["mod_num"]));
                check_refresh_list = RTLIL::SigSpec(1, 5); //check mod is loop eq, mod le;
                check_refresh_list.append(RTLIL::SigSpec(2, 5)); //refresh mod : policy 3 defined in cellift_util.cc
            }
            if(it == STM_express.rfind(")") + 4 && it == STM_express.size()-1){
                loop_mod_list = RTLIL::SigSpec(state_map["loop"], state_map["loop_num"]);
                loop_mod_list.append(get_front_mod_state(find_the_front_mod(it),state_map["mod_num"]));
                check_refresh_list = RTLIL::SigSpec(1, 5); //check mod is loop eq, mod le;
                check_refresh_list.append(RTLIL::SigSpec(4, 5)); //refresh mod : policy 3 defined in cellift_util.cc
            }
            else{
                loop_mod_list = RTLIL::SigSpec(state_map["loop"]+1, state_map["loop_num"]);
                loop_mod_list.append(get_front_mod_state(find_the_front_mod(it),state_map["mod_num"]));
                check_refresh_list = RTLIL::SigSpec(1, 5); //check mod is loop eq, mod le; 
                check_refresh_list.append(RTLIL::SigSpec(1, 5)); //refresh mod : policy 3 defined in cellift_util.cc
            }

            state_checklist.insert(make_pair(loop_mod_list, check_refresh_list));
            //state_checklist.insert(make_pair(RTLIL::SigSpec(SigChunk(state_map["loop"])), get_front_mod_state(find_the_front_mod(it),state_map["mod_num"])));
            //bug if sigspec(sigchunk(int)) == sigspec(int), refresh operation policy3 will not be tuch;
        }
    }
    return state_checklist;
}