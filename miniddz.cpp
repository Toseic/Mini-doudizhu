#include "miniddz.h"

unordered_map<Hash, Node> nodes;

#define MCTSTimeLimit 0.1
#define MCTSGameSize 50

void Node::newchild(Hand hd) {
    Node newnode = *this;
    newnode.fromHand = hd;
    newnode.states[user].myaction = hd;
    newnode.states[user].cards.mycards -= hd.action;
    newnode.user = next(newnode.user);
    if (hd != PASS) newnode.states[newnode.user].last_action = hd;
    else if (states[last(user)].myaction==PASS) 
        {newnode.states[newnode.user].last_action = PASS_HAND;}
    else {newnode.states[newnode.user].last_action = states[user].last_action;}
}

void Node::expand() {

}

User rand_game(Node node ) {
    int actionID;
    User user = node.user;
    
    while (1) {
        vector<Hand> actions;
        if (node.states[user].cards.mycards == EMPTY_CARDS) break;
        node.states[user].make_decision(actions);
        Hand thisaction = actions[rand_int(actions.size())];
        node.states[user].cards.mycards -= thisaction.action;
        node.states[user].myaction = thisaction;
        if (thisaction == PASS) node.states[next(user)].last_action = thisaction;
        else if (node.states[last(user)].myaction==PASS)
            node.states[next(user)].last_action = PASS_HAND;
        else node.states[next(user)].last_action = node.states[user].last_action;
    }
    return user;
}


void game_init(State * states, cards& lord_card) {
    vector<int> user0;
    vector<int> user1;
    vector<int> user2;
    vector<int> lord_cards;
    random_shuffle(all_cards, all_cards+30);

    for (int i=0;i<9;i++) {
        user0.push_back(all_cards[i*3]);
        user1.push_back(all_cards[i*3+1]);
        user2.push_back(all_cards[i*3+2]);
    }
    lord_cards.push_back(all_cards[27]);
    lord_cards.push_back(all_cards[28]);
    lord_cards.push_back(all_cards[29]);
    user0.insert(user0.end(), lord_cards.begin(), lord_cards.end());

    lord_card = Cards::encode(lord_cards);
    states[USER0].cards = Cards::encode(user0);
    states[USER1].cards = Cards::encode(user1);
    states[USER2].cards = Cards::encode(user2);
}

Hash nodeChoose(const Node & node) {
    double best_eval = -1e5; int bestnode = -1;
    for (int i=0;i<node.childNum;i++) {
        Node & child = nodes[node.children[i]];
        double eval_ = child.eval();
        if (eval_ > best_eval) {
            best_eval = eval_; bestnode = i;
        } else if (eval_ == best_eval && rand_int(2)) {
            best_eval = eval_; bestnode = i;
        }
    }
    if (nodes[node.children[bestnode]].isexpand) 
        return nodeChoose(nodes[node.children[bestnode]]);
    return node.children[bestnode];
}

// 每个节点的eval值都是对于root节点的玩家的有利程度
// 在数据回传时也是根据root节点玩家的角度添加reward
// 考虑自己的队友
void node_check(Node& node) {
    int win = 0;
    const int rew = 1;
    User winner;
// rollout
    for (int i=0;i<MCTSGameSize;i++) {
        if (rand_game(node) == LORD) {
            if (node.user == LORD) win++;
        } else {
            if (node.user != LORD) win++;
        }
    }
    if (win > MCTSGameSize/2) {
        if (node.user == LORD) winner = LORD;
        else winner = FARMER;
    } else {
        if (node.user == LORD) winner = FARMER;
        else winner = LORD;
    }

// data 回传
    Hash hash = node.hash;
    while (1) {
        ++ nodes[hash].visit ;
        if (winner == LORD) {
            if (nodes[hash].user==LORD) nodes[hash].reward += rew;
        } else {
            if (nodes[hash].user != LORD) nodes[hash].reward += rew;
        }
        if (nodes[hash].isroot) break;
        Hash newhash = nodes[hash].parent;
        hash = newhash;
    }
}

void node_guess(Node& node, Cards rest, vector<Hand>* history) {

}

Hand mcts(Node mainnode) {
    clock_t start,finish;
    Node & node = mainnode;
    if (!node.isexpand) node.expand();
    node.isroot = true;
    if (node.childNum == 0 ) return PASS_HAND;
    start = clock();
    while (1) {
        finish = clock();
        if ((double)(finish-start)/CLOCKS_PER_SEC >= MCTSTimeLimit) break;
        Node & choosedNode = nodes[nodeChoose(node)];
        node_check(choosedNode);
    }
    int bestNode = -1;
    double min_eval = -1e5;
    for (int i=0;i<node.childNum; i++) {
        double nodeeval = nodes[i].reward_eval();
        if (nodeeval > min_eval) {
            min_eval = nodeeval;
            bestNode = i;
        } else if (nodeeval == min_eval && rand_int(2)) {
            min_eval = nodeeval;
            bestNode = i;
        }
    }
    return nodes[bestNode].fromHand;

}


int main() {
    State states[3]{USER0, USER1, USER2};
    vector<Hand> history[3];
    cards lord_card;
    game_init(states, lord_card);
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