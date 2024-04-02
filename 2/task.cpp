#include "api.hpp"
#include <string>
#include <set>
#include <map>
#include <iostream>
#include <vector>

using namespace std;

// Function to convert DFA to regular expression
std::string dfa2re(DFA &d) {
    auto terms = d.get_alphabet().to_string();
    if (terms == "") {
        return "";
    }
    auto all = d.get_states();
    if (all.empty()) {
        return "";
    }

    map<pair<int, int>, string> m;
    map<int, string> idx2;
    map<string, int> name_idx;
    int id = 0;

    for (auto state : all) {
        name_idx[state] = id;
        idx2[id] = state;
        id += 1;
    }

    // Populate transition map with DFA transitions
    for (auto it = all.begin(); it != all.end(); ++it) {
        auto state = *it;
        if (d.is_final(state)) {
            pair<int, int> pair_to_insert = make_pair(name_idx[state], all.size());
            m[pair_to_insert] = "";
        }
        for (auto term_it = terms.begin(); term_it != terms.end(); ++term_it) {
            auto terminate = *term_it;
            if (d.has_trans(state, terminate)) {
                pair<int, int> p = make_pair(name_idx[state], name_idx[d.get_trans(state, terminate)]);
                string s(1, terminate);
                if (m.count(p) == 0) {
                    m[p] = s;
                } else {
                    m[p] += "|" + s;
                }
            }
        }
    }


    // Eliminate self-loops and generate regular expressions
    for (int i = 0; i < all.size(); i++) {
        if (!d.is_initial(idx2[i])) {
            string in_itself = "";
            // Check if there is a self-loop for the current state
            if (m.count(make_pair(i, i)) != 0) {
                in_itself = m[make_pair(i, i)];
                m.erase(make_pair(i, i));
            }

            set<pair<int, int>> for_delete;
            for (auto p_from : m) {
                // Check transitions from other states to the current state
                if (p_from.first.second == i && p_from.first.first != i) {
                    for (auto p_to : m) {
                        if (p_to.first.second != i && p_to.first.first == i) {
                            string add = "(" + p_from.second + ")" + "(" + in_itself + ")" + "*" + "(" + p_to.second + ")";
                            if (m.count(make_pair(p_from.first.first, p_to.first.second)) != 0) {
                                add += "|" + m[make_pair(p_from.first.first, p_to.first.second)];
                            }
                            m[make_pair(p_from.first.first, p_to.first.second)] = add;
                            for_delete.insert(p_from.first);
                            for_delete.insert(p_to.first);
                        }
                    }
                }
            }
            // Delete marked transitions
            for (auto elem : for_delete) {
                m.erase(elem);
            }
        }
    }

    // Construct final regular expression
    string res = "";
    pair<int, int> initial_pair = make_pair(name_idx[d.get_initial_state()], name_idx[d.get_initial_state()]);
    if (m.count(initial_pair) != 0) {
        res = "(" + m[initial_pair] + ")" + "*";
    }
    pair<int, int> final_pair = make_pair(name_idx[d.get_initial_state()], all.size());
    res += '(' + m[final_pair] + ')';

    return res;
}
