#include "api.hpp"
#include <string>
#include <set>
#include <map>
#include <vector>
#include <stack>
#include <iostream>

using namespace std;

DFA dfa_minim(DFA &d) {
    // Extract necessary information from the DFA
    string trans = (d.get_alphabet()).to_string();
    set<string> names = d.get_states();
    set<string> unreachable;

    string init_st = d.get_initial_state();
    stack<string> states;
    states.push(init_st);

    unreachable.insert(init_st);
    // Find unreachable states
    while (!states.empty()) {
        string state = states.top();
        states.pop();

        for (const auto& symbol : d.get_alphabet()) {
            if (d.has_trans(state, symbol)) {
                string next_state = d.get_trans(state, symbol);
                if (unreachable.count(next_state) == 0) {
                    states.push(next_state);
                    unreachable.insert(next_state);
                }
            }
        }
    }

    // Remove unreachable states
    for (auto i : names)
        if (unreachable.count(i) == 0)
            d.delete_state(i);

    // Create a 'deadlock' state
    d.create_state("dead_state");
    names = d.get_states();
    map<int, map<string, map<char, int>>> table;

    map<string, int> new_states;
    // Assign new state IDs based on whether they are final or not
    for (auto i : names)
        new_states[i] = (d.is_final(i)) ? 1 : 0;

    new_states["dead_state"] = 0;

    // Initialize the transition table
    for (const auto& state : names) {
        int index = (d.is_final(state)) ? 1 : 0; 
        for (const auto& symbol : trans) {
            if (!d.has_trans(state, symbol)) {
                d.set_trans(state, symbol, "dead_state");
                table[index][state][symbol] = 0;
            } else {
                table[index][state][symbol] = new_states.at(d.get_trans(state, symbol));
            }
        }
    }

    int last_num = 2;
    bool flag = true; 
    // Minimize the DFA using Hopcroft's algorithm
    while (flag) {
        flag = false;
        for (auto i = table.begin(); i != table.end(); i++) {
            auto elem1 = i->second.begin();
            auto j = elem1;
            j++;
            map<string, map<char, int>> tmp;
            
            // Group states based on equivalence classes
            for (auto it = i->second.begin(); it != i->second.end();) {
                if (it->second != elem1->second) {
                    flag = true;
                    tmp[it->first] = it->second;
                    new_states[it->first] = last_num;
                    it = i->second.erase(it);
                } else
                    ++it;
            }

            if (!tmp.empty())
                table[last_num] = tmp;
            
            for (auto& state : table) {
                for (auto& j : state.second) {
                    for (auto& sym : j.second) {
                        // Update transition based on new state mappings
                        sym.second = new_states[d.get_trans(j.first, sym.first)];
                    }
                }
            }

            last_num += 1;
        }
    }

    // Create the minimized DFA
    DFA new_d = DFA(trans);
    string dead_state = to_string(new_states["dead_state"]);
    for (auto i = table.begin(); i != table.end(); i++) {
        string name_state = to_string(i->first);
        if (name_state != dead_state) {
            new_d.create_state(name_state);
            if (d.is_final(table[i->first].begin()->first)) {
                new_d.make_final(name_state);
            }
        }
    }

    // Add transitions to the new DFA
    for (auto i = table.begin(); i != table.end(); i++) {
        string name_state = to_string(i->first);
        if (name_state != dead_state) {
            auto j = i->second.begin();
            for (auto k = j->second.begin(); k != j->second.end(); k++) {
                if (k->second != new_states["dead_state"]) {
                    new_d.set_trans(name_state, k->first, to_string(k->second));
                }
            }
        }
    }

    new_d.set_initial(to_string(new_states[init_st]));

    return new_d;
}
