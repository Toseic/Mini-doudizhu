#include "miniddz.h"

unordered_map<Hash, Node> nodes;

void Node::newchild(Hand hd) {
    Node newnode = *this;
    states[user].myaction = hd;
    newnode.states[user].cards.mycards -= hd.action;
    newnode.user = next(newnode.user);
    if (hd != PASS) newnode.states[newnode.user].last_action = hd;
    else if (states[last(user)].myaction==PASS) 
        {newnode.states[newnode.user].last_action = PASS_HAND;}
    else {newnode.states[newnode.user].last_action = states[user].last_action;}
}

int rand_game(Node& node ) {

}


void game_init(State * states) {

}

void node_guess(Node& node, Cards rest, vector<Hand>* history) {
    
}

Hand mcts(const Node& mainnode) {
    
}


int main() {
    State states[3]{USER0, USER1, USER2};
    vector<Hand> history[3];
    game_init(states);
    User user = USER0;
    while(1) {
        Node mainnode(states[0],states[1],states[2],user,BLIND);
        Cards rest_card = states[next(user)].cards.mycards + states[last(user)].cards.mycards;
        node_guess(mainnode, rest_card, history);
        Hand action = mcts(mainnode);
        states[user].cards.mycards -= action.action; 
        states[user].myaction = action;
        history[user].push_back(action);
        if (action != PASS) {
            states[next(user)].last_action = action;
        } else if (states[last(user)].myaction == PASS) {
            states[next(user)].last_action = PASS_HAND;
        } else states[next(user)].last_action = states[user].last_action;

    }
}