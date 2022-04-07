#include "miniddz.h"

unordered_map<ID, Node> nodes;

#define MCTSTimeLimit 0.5
#define MCTSGameSize 50
#define MCTS_v1 0.5

#define debug
// #define showdetail

Node Node::newchild(Hand hd) {
    Node newnode = *this;
    newnode.isroot = false;
    newnode.isexpand = false;
    newnode.childNum = 0;
    newnode.fromHand = hd;
    newnode.states[user].myaction = hd;
    newnode.states[user].cards.mycards -= hd.action;
    newnode.user = next(user);
    newnode.parent = Id;
    newnode.visit = 0; newnode.reward = 0;

    if (hd != PASS) newnode.states[newnode.user].last_action = hd;
    else if (states[last(user)].myaction==PASS) 
        {newnode.states[newnode.user].last_action = PASS_HAND;}
    else {newnode.states[newnode.user].last_action = states[user].last_action;}

    newnode.nodeID();
    children[childNum] = newnode.Id;
    childNum ++;
    return newnode;
}

void Node::expand() {
    #ifdef debug
    // cout << "$ ";
    #endif
    vector<Hand>actions;
    states[user].make_decision(actions);
    for (vector<Hand>::iterator i=actions.begin(); i!=actions.end();i++) {
        Node newnode = newchild(*i);
        nodes[newnode.Id] = newnode;
    }
    isexpand = true;
}

double Node::eval() {
    const Node& myparent = nodes[parent];
    double value;
    if (!visit) return 100;
    if (!myparent.visit) return 0;
    value = reward/visit + MCTS_v1*sqrt(log(myparent.visit)/visit +1e-5);
    return value;
}

double Node::reward_eval() {
    if (visit==0) return 0;
    double value = reward / visit;
    return value;    
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
    states[USER0].last_action = PASS_HAND;
}

ID nodeChoose(const Node & node) {
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
    if (nodes[node.children[bestnode]].isexpand && 
        !nodes[node.children[bestnode]].isLeaf()) 
        return nodeChoose(nodes[node.children[bestnode]]);
    return node.children[bestnode];
}

// 每个节点的eval值都是对于root节点的玩家的有利程度
// 在数据回传时也是根据root节点玩家的角度添加reward
// 考虑自己的队友
void node_check(Node& node,User rootuser) {
    int win = 0;
    const int rew = 1;
    int iswin;
// rollout
    for (int i=0;i<MCTSGameSize;i++) {
        #ifdef showdetail
        cout << "^ ";
        #endif
        if (rand_game(node) == LORD) {
            if (node.user == LORD) win++;
        } else {
            if (node.user != LORD) win++;
        }
    }
    if (win > MCTSGameSize/2) {
        if (rootuser == LORD) {
            if (node.user == LORD) iswin = 1;
            else iswin = 0;
        } else {
            if (node.user != LORD) iswin = 1;
            else iswin = 0;
        }
    } else {
        if (rootuser == LORD) {
            if (node.user != LORD) iswin = 1;
            else iswin = 0; 
        } else {
            if (node.user == LORD) iswin = 1;
            else iswin = 0; 
        }
    }


// data 回传
    ID thisid = node.Id;
    #ifdef debug
    int count = 0;
    #endif
    while (1) {
        #ifdef debug
            // cout << "^" ;
            count ++;
            if (count > 100000) {
                cout << "unend loop !!!"<< endl;
                system("pause");
            }
        #endif
        ++ nodes[thisid].visit ;
        if (iswin) {
             nodes[thisid].reward += rew;
        } 
        // else {
            //  nodes[hash].reward += rew;
        // }
        if (nodes[thisid].isroot) break;
        ID newid = nodes[thisid].parent;
        thisid = newid;
    }
}

void node_guess(Node& node, Cards rest, vector<Hand>* history, 
    int num_last, int num_next) {
    vector<int> rest_cards = rest.decode();
    random_shuffle(rest_cards.begin(),rest_cards.end());
    vector<int>last_cards,next_cards;
    last_cards.insert(last_cards.end(),rest_cards.begin(),rest_cards.begin()+num_last);
    next_cards.insert(next_cards.end(),rest_cards.end()-num_next,rest_cards.end());
    node.states[last(node.user)].cards = Cards::encode(last_cards);
    node.states[next(node.user)].cards = Cards::encode(next_cards);
}



Hand mcts(Node mainnode) {
    clock_t start,finish;
    mainnode.isroot = true;
    nodes.clear();
    nodes[mainnode.Id] = mainnode;
    Node & node = nodes[mainnode.Id];
    if (!node.isexpand && !(node.isLeaf())) node.expand();
    if (node.childNum == 0 ) return PASS_HAND;
    start = clock();
    while (1) {
        #ifdef showdetail
        cout << "* ";
        #endif
        finish = clock();
        if ((double)(finish-start)/CLOCKS_PER_SEC >= MCTSTimeLimit) {
            #ifdef showdetail
            cout <<endl <<"---------------------"<< endl;
            #endif
            break;
        }
        Node & choosedNode = nodes[nodeChoose(node)];

        node_check(choosedNode, mainnode.user);

        #ifdef showdetail
        // cout << nodes.size() << " ";
        if(choosedNode.isLeaf()) cout << "& "; 
        #endif

        if(!choosedNode.isLeaf()) choosedNode.expand();
        // start = clock();

    }
    int bestNode = -1;
    double max_eval = -1e5;
    for (int i=0;i<node.childNum; i++) {
        double nodeeval = nodes[node.children[i]].reward_eval();
        if (nodeeval > max_eval) {
            max_eval = nodeeval;
            bestNode = i;
        } else if (nodeeval == max_eval && rand_int(2)) {
            max_eval = nodeeval;
            bestNode = i;
        }
    }
    #ifdef debug
    // cout <<nodes.size()<<" "<< node.visit << " " << node.reward<< endl ;
    #endif
    return nodes[node.children[bestNode]].fromHand;

}
// #define timecheck

int main() {
    Srand();
    State states[3]{USER0, USER1, USER2};
    vector<Hand> history[3];
    cards lord_card;
    game_init(states, lord_card);
    #ifdef debug
    vector<int>card_vec[3];
    card_vec[0] = states[0].cards.decode();
    card_vec[1] = states[1].cards.decode();
    card_vec[2] = states[2].cards.decode();
    for (int i=0;i<3;i++) {
        for (int j=0;j<card_vec[i].size();j++) {
            cout << card_name[card_vec[i][j]]<<" ";
        }
        cout << endl;
    }
    cout << "-------------" << endl;
    #endif
    User user = USER0;
    while(1) {
        #ifdef timecheck
        clock_t mainstart,mainend;
        mainstart = clock();
        #endif
        if (states[user].cards.mycards == EMPTY_CARDS) break;
        Node mainnode(states[0],states[1],states[2],user,lord_card,BLIND);
        Node::nowId = 0;
        mainnode.nodeID();
        Cards rest_card = states[next(user)].cards.mycards + states[last(user)].cards.mycards;
        int num_last = Cards::cards_sum(states[last(user)].cards.mycards);
        int num_next = Cards::cards_sum(states[next(user)].cards.mycards);
        node_guess(mainnode, rest_card, history, num_last, num_next);
        Hand action = mcts(mainnode);
        states[user].cards.mycards -= action.action; 
        states[user].myaction = action;
        history[user].push_back(action);
        if (action != PASS) {
            states[next(user)].last_action = action;
        } else if (states[last(user)].myaction == PASS) {
            states[next(user)].last_action = PASS_HAND;
        } else states[next(user)].last_action = states[user].last_action;
        
        #ifdef debug
        
        vector<int>action_vec = Cards::decode(action.action);
        cout << "user" << user << " ";
        if (action_vec.size() == 0) cout << "pass";
        for (int j=0;j<action_vec.size();j++) {
            cout << card_name[action_vec[j]]<<" ";
        }
        cout << endl;
        // system("pause");
        #endif
        user = next(user);
        #ifdef timecheck
        mainend = clock();

        cout << (double)(mainend - mainstart)/CLOCKS_PER_SEC << endl;
        #endif
    }
    cout << "WINNER: " << user << endl; 
}