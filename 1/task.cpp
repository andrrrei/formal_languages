#include "api.hpp"
#include <string>
#include <iostream>
#include <stack>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <queue>

using namespace std;

// Class representing a MyTree in the abstract syntax tree (AST)
class MyTree {
public:
    set<int> firstpos;  // Set of positions in the input string where this MyTree's symbol can occur first
    set<int> lastpos;   // Set of positions in the input string where this MyTree's symbol can occur last
    bool nullable;      // Indicates if this MyTree is nullable
    MyTree* left;         // Pointer to the left child MyTree
    MyTree* right;        // Pointer to the right child MyTree
    char symbol;        // Symbol represented by this MyTree

    // Constructors
    MyTree(): nullable(false), left(nullptr), right(nullptr) {}

    MyTree(char symb, set<int> fp = {}, set<int> lp = {}, bool nul = false, MyTree* l = nullptr, MyTree* r = nullptr):
        firstpos(fp), lastpos(lp), nullable(nul), left(l), right(r), symbol(symb) {}
};

int idx_all = 0;  // Global index variable for parsing input string
int idx_symb = 0; // Global index variable for symbols in the input string

// Function prototypes
MyTree* A(string st);
MyTree* D(string st);
MyTree* C(string st);
MyTree* B(string st);
MyTree* S(string st);
void null_1pos_lastpos(MyTree* tree);
void follow_pos(MyTree* tree);
vector<int> pos_ind(string st, char symbol, set<int> Ra);
void delete_MyTree(MyTree* tree);
DFA re2dfa(const string& st);

// Parse a concatenation of expressions
MyTree* A(string st) {
    MyTree* tmp = B(st);
    while (st[idx_all] == '|') {
        idx_all++;
        MyTree* news = B(st);
        tmp = new MyTree('|', {}, {}, false, tmp, news);
    }
    return tmp;
}

// Parse an expression within parentheses
MyTree* D(string st) {
    MyTree* tmp;
    if (st[idx_all] == '(') {
        idx_all++;
        tmp = A(st);
        idx_all++;
    } else {
        cout << st[idx_all];
        tmp = new MyTree(st[idx_all], {idx_symb}, {idx_symb}, false, nullptr, nullptr);
        idx_all++;
        idx_symb++;
    }
    return tmp;
}

// Parse a Kleene star or a concatenation of symbols
MyTree* C(string st) {
    auto tmp = D(st);
    while (st[idx_all] == '*') {
        idx_all++;
        tmp = new MyTree('*', {}, {}, true, tmp, nullptr);
    }
    return tmp;
}

// Parse a single symbol or a concatenation of symbols
MyTree* B(string st) {
    MyTree* tmp;
    if (isalnum(st[idx_all]) || st[idx_all] == '(' || st[idx_all] == '#') {
        tmp = C(st);
        while (isalnum(st[idx_all]) || st[idx_all] == '(' || st[idx_all] == '#') {
            auto many = C(st);
            tmp = new MyTree('.', {}, {}, false, tmp, many);
        }
        return tmp;
    }
    return new MyTree(st[idx_all], {}, {}, true, nullptr, nullptr);
}

// Parse the entire regular expression, adding necessary parentheses and an end marker
MyTree* S(string st) {
    st = '(' + st + ')' + '#';
    return A(st);
}

// Compute nullable, firstpos, and lastpos properties of MyTrees in the AST
void null_1pos_lastpos(MyTree* tree) {
    if (tree->left == nullptr && tree->right == nullptr) {
        return;
    }
    if (tree->symbol == '|') {
        null_1pos_lastpos(tree->left);
        null_1pos_lastpos(tree->right);
        tree->nullable = tree->left->nullable || tree->right->nullable;
        set<int> fp(tree->left->firstpos);
        fp.insert(tree->right->firstpos.begin(), tree->right->firstpos.end());
        tree->firstpos = fp;
        set<int> lp(tree->left->lastpos);
        lp.insert(tree->right->lastpos.begin(), tree->right->lastpos.end());
        tree->lastpos = lp;
    } else if (tree->symbol == '.') {
        null_1pos_lastpos(tree->left);
        null_1pos_lastpos(tree->right);
        tree->nullable = tree->left->nullable && tree->right->nullable;
        if (tree->left->nullable) {
            set<int> fp(tree->left->firstpos);
            fp.insert(tree->right->firstpos.begin(), tree->right->firstpos.end());
            tree->firstpos = fp;
        } else {
            tree->firstpos = tree->left->firstpos;
        }
        if (tree->right->nullable) {
            set<int> lp(tree->left->lastpos);
            lp.insert(tree->right->lastpos.begin(), tree->right->lastpos.end());
            tree->lastpos = lp;
        } else {
            tree->lastpos = tree->right->lastpos;
        }
    } else if (tree->symbol == '*') {
        null_1pos_lastpos(tree->left);
        tree->nullable = true;
        tree->firstpos = tree->left->firstpos;
        tree->lastpos = tree->left->lastpos;
    }
}


// Compute followpos property of MyTrees in the AST
vector<set<int>> follow_position;
void follow_pos(MyTree* tree) {
    if (tree->left == nullptr && tree->right == nullptr) {
        return;
    }
    if (tree->symbol == '.') {
        for (auto i : tree->left->lastpos) {
            follow_position[i].insert(tree->right->firstpos.begin(), tree->right->firstpos.end());
        }
        follow_pos(tree->left);
        follow_pos(tree->right);
    } else if (tree->symbol == '*') {
        for (auto i : tree->left->lastpos) {
            follow_position[i].insert(tree->left->firstpos.begin(), tree->left->firstpos.end());
        }
        follow_pos(tree->left);
    } else if (tree->symbol == '|') {
        follow_pos(tree->left);
        follow_pos(tree->right);
    }
}

// Find positions of a symbol in the input string
vector<int> pos_ind(string st, char symbol, set<int> Ra) {
    vector<int> res;
    int idx = 0;
    for (int i = 0; i < st.size(); i++) {
        if (st[i] == symbol && Ra.count(idx) != 0) {
            res.push_back(idx++);
        } else if (isalnum(st[i]) || st[i] == '#') {
            idx++;
        }
    }
    return res;
}

// Recursively delete MyTrees in the AST
void delete_MyTree(MyTree* tree) {
    if (tree == nullptr) {
        return;
    }
    delete_MyTree(tree->left);
    delete_MyTree(tree->right);
    delete tree;
}

// Convert regular expression to DFA
DFA re2dfa(const string& st) {
    if (st.empty()) return DFA::from_string("a\n[[a]]\n");
    // Construct the AST from the regular expression
    MyTree* tree = S(st);
    null_1pos_lastpos(tree);

    // Initialize followpos data structure
    follow_position.resize(idx_symb);
    follow_pos(tree);

    // Initialize initial state
    string q_0 = "";
    for (auto elem : tree->firstpos) {
        if (elem == idx_symb - 1) {
            q_0 += '#';
        }
        q_0 += to_string(elem);
    }

    // Initialize sets and maps
    set<string> Q;
    Q.insert(q_0);
    set<string> markered;
    map<string, set<int>> str2set;
    str2set[q_0] = tree->firstpos;

    // Initialize DFA
    DFA res = DFA(Alphabet(st));
    res.create_state(q_0, false);
    res.set_initial(q_0);

    // Construct DFA states and transitions
    while (markered.size() != Q.size()) {
        set<string> diff;
        set_difference(Q.begin(), Q.end(), markered.begin(), markered.end(), inserter(diff, diff.end()));
        string R = *(diff.begin());
        markered.insert(R);
        for (auto terminate : Alphabet(st)) {
            set<int> S;
            for (int p_i : pos_ind(st, terminate, str2set[R])) {
                S.insert(follow_position[p_i].begin(), follow_position[p_i].end());
            }
            if (!S.empty()) {
                string ss = "";
                for (auto elem : S) {
                    if (elem == idx_symb - 1) {
                        ss += '#';
                    }
                    ss += to_string(elem);
                }

                if (Q.count(ss) == 0) {
                    Q.insert(ss);
                    str2set[ss] = S;
                    res.create_state(ss, false);
                }

                res.set_trans(R, terminate, ss);
            }
        }
    }

    // Mark final states
    for (auto elem : Q) {
        if (elem.find('#') != elem.npos) {
            res.make_final(elem);
        }
    }

    // Clean up
    delete_MyTree(tree);

    return res;
}
