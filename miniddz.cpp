#include "miniddz.h"
using namespace doudizhu;

#define local
// #define test
#define debug

// #define Botzone

#define mctsTimeLimit 0.1
#define mcts_v1 1
#define mcts_game_size 15

const vector<int> empty_vector(MAX_CARD_TYPE_NUM);
const Hand PASS_HAND = Hand(empty_vector);
unordered_map<Hash, Node> nodes;



void srand() {
    QueryPerformanceFrequency(&seed);
    QueryPerformanceCounter(&seed);
    srand(seed.QuadPart);   
}

void AI(string line, Json::Value& ai_action){
    // 我的牌具体有哪些
	bool my_cards_bm[MAX_CARD_NUM] = {};
	// 我的身份
    int pos;
	Json::Value input;
	Json::Reader reader;
    #ifdef Botzone
	string line;  // TODO: 由外界函数传入
	getline(cin, line);  //TODO: 修改为内部程序发送，而且不停止程序
    #endif
	reader.parse(line, input);
    // 这对大括号不要删掉
    {
        auto req = input["requests"][0u];
        auto own = req["own"];
        auto history = req["history"];
        // 标记一开始发给我的牌
        for(unsigned i = 0; i < own.size(); ++ i){
            my_cards_bm[own[i].asInt()] = true;
        }
        if(history[0u].size() > 0){
            pos = 2;
        } else if(history[1u].size() > 0){
            pos = 1;
        } else {
            pos = 0;
        }
    }

	// 把我出过的牌从初始手牌中去掉
	for(unsigned i = 0; i < input["responses"].size(); ++ i) {
		auto resp = input["responses"][i];
		for(unsigned j = 0; j < resp.size(); ++ j) {
            my_cards_bm[resp[j].asInt()] = false;
        }
	}

    // 我当前实际拥有的牌、待响应的上一手牌（0-53编码）
    vector<Card> my_cards, last_action;
    for (int i = 0; i < MAX_CARD_NUM; ++ i){
        // 如果这牌我没出过，那么加入列表
        if (my_cards_bm[i]){
            my_cards.push_back(i);
        }
    }

	// 看看之前玩家出了什么牌
    auto history = input["requests"][input["requests"].size() - 1]["history"];
    if (history[1u].size() > 0){
        for (unsigned i = 0; i < history[1u].size(); ++ i){
            int c = history[1u][i].asInt();
            last_action.push_back(c);
        }
    } else if (history[0u].size() > 0){
        for (unsigned i = 0; i < history[0u].size(); ++ i){
            int c = history[0u][i].asInt();
            last_action.push_back(c);
        }
    }

    // 根据我现有手牌、待响应的上一手牌，构造当前游戏状态
    State state(my_cards, last_action);

    // 根据当前游戏状态，生成全部可行动作，用EncodedCards编码
    vector<EncodedCards> valid_actions = state.validActions();
    
    // 决定出的的动作，用牌张列表表示（0-53编码）
    vector<Card> action; 
    
    // 随机选择得到的动作在所有可行动作中的序号
    unsigned random_action_id;
    srand(time(nullptr));

    #ifdef local
        // ------ user act begin ---------

        // ------ user act end -----------
        
    #endif

    // // 之前人都Pass了，我就得出牌，不能pass
    // if (state.last_action.isPass()){
    //     random_action_id = rand() % valid_actions.size();
    //     action = state.decodeAction(valid_actions[random_action_id]);
    // } else {
    //     // 之前人没有都Pass，我可以选择pass
    //     random_action_id = rand() % (valid_actions.size() + 1);
    //     // 此时我方动作选择不是pass，需要计算具体action
    //     if (random_action_id != valid_actions.size()){
    //         action = state.decodeAction(valid_actions[random_action_id]);
    //     }
    // }
    // Json::Value result, response(Json::arrayValue);
    // for (Card c : action){
    //     response.append(c);
    // }



    #ifdef Botzone
        result["response"] = response;
        Json::FastWriter writer;
        cout << writer.write(result) << endl;
    #endif

    #ifdef local
        
    #endif
}

// 按照user0，user1，user2的顺序输入数据,返回赢家
// 此函数准备弃用
User randonGame__(vector<Card>& card0, vector<Card>& card1, 
        vector<Card>& card2, Hand last_action_, User user) {
    State states[3]{card0,card1,card2};
    states[user].last_action = last_action_;
    int actionID;
    vector<Card> action;

    int passTimes = 0;
    // State & state = states[user];
    while (1) {
        // state = states[user];
        vector<EncodedCards> valid_actions = states[user].validActions();
        // cout << "<<<<<<<<<<<debug------------" << endl;
        // cout << "ispass " << states[user].last_action.isPass()<<endl;
        // for (auto i = valid_actions.begin();i!=valid_actions.end();i++) {
        //     vector<Card> action = states[0].decodeAction(*i,1);
        //     for (auto j = action.begin();j!=action.end();j++) {
        //         cout <<card_name[*j] << ',';
        //     }
        //     cout << endl;
        // }
        // cout << ">>>>>>>>>>>debug------------" << endl;

        // 类似于例码中的L614-623
        if (valid_actions.size()==0) action.clear();
        else if (states[user].last_action.isPass()) {
            actionID = rand_int(valid_actions.size());
            action = states[user].decodeAction(valid_actions[actionID]);
        } else {
            actionID = rand_int(valid_actions.size()+1);
            if (actionID != valid_actions.size()){
                action = states[user].decodeAction(valid_actions[actionID]);
            } else action.clear();
        } // 将玩家打出的牌从手牌中去掉的动作已经在state.decodeAction(valid_actions[actionID])中完成
        #ifdef test
        cout << "user" << user<<": ";
        for (int i=0;i<action.size();i++) {
            cout << card_name[action[i]] <<',';
        }
        if (action.size()==0) cout << "pass"; 
        cout << endl;
        // system("pause");
        #endif
        if (action.size()==0) passTimes ++; else passTimes = 0;
        if (states[user].my_cards.size()==0) break;
        User olduser = user;
        // if (user==USER2) user=USER0;
        // else user = User(user+1);
        user = next(user);

        states[user].last_action = states[olduser].last_action;
        if (action.size()!=0) states[user].last_action = Hand(toCardCountVector(action));
        else if (passTimes==2) states[user].last_action = PASS_HAND;
        
    }
    return user; 
}

// 按照user0，user1，user2的顺序输入数据,返回赢家
User randonGame(State state0, State state1, 
        State state2, User user) {
    State states[3]{state0, state1, state2};
    int actionID;
    vector<Card> action;

    int passTimes = 0;
    // State & state = states[user];
    while (1) {
        // state = states[user];
        vector<EncodedCards> valid_actions = states[user].validActions();
        // cout << "<<<<<<<<<<<debug------------" << endl;
        // cout << "ispass " << states[user].last_action.isPass()<<endl;
        // for (auto i = valid_actions.begin();i!=valid_actions.end();i++) {
        //     vector<Card> action = states[0].decodeAction(*i,1);
        //     for (auto j = action.begin();j!=action.end();j++) {
        //         cout <<card_name[*j] << ',';
        //     }
        //     cout << endl;
        // }
        // cout << ">>>>>>>>>>>debug------------" << endl;

        // 类似于例码中的L614-623
        if (valid_actions.size()==0) action.clear();
        else if (states[user].last_action.isPass()) {
            actionID = rand_int(valid_actions.size());
            action = states[user].decodeAction(valid_actions[actionID]);
        } else {
            actionID = rand_int(valid_actions.size()+1);
            if (actionID != valid_actions.size()){
                action = states[user].decodeAction(valid_actions[actionID]);
            } else action.clear();
        } // 将玩家打出的牌从手牌中去掉的动作已经在state.decodeAction(valid_actions[actionID])中完成
        #ifdef test
        cout << "user" << user<<": ";
        for (int i=0;i<action.size();i++) {
            cout << card_name[action[i]] <<',';
        }
        if (action.size()==0) cout << "pass"; 
        cout << endl;
        // system("pause");
        #endif
        if (action.size()==0) passTimes ++; else passTimes = 0;
        if (states[user].my_cards.size()==0) break;
        User olduser = user;
        // if (user==USER2) user=USER0;
        // else user = User(user+1);
        user = next(user);

        states[user].last_action = states[olduser].last_action;
        if (action.size()!=0) states[user].last_action = Hand(toCardCountVector(action));
        else if (passTimes==2) states[user].last_action = PASS_HAND;
        
    }
    return user; 
}

Node Node::newChild(EncodedCards encode_action) {
    Node newnode(states[0],states[1],states[2],next(user));
    newnode.from_action = encode_action;
    newnode.parent = hash; children[childNum] = newnode.hash; childNum++;
    // 在新的节点中把上一个玩家打出的牌扔掉，原本节点不应改变
    vector<Card> action = newnode.states[user].decodeAction(encode_action);
    if (action.size()!=0) newnode.states[newnode.user].last_action = Hand(toCardCountVector(action));
    else newnode.states[newnode.user].last_action = newnode.states[user].last_action;
    if (states[user].last_action==states[last(user)].last_action) 
        newnode.states[newnode.user].last_action = PASS_HAND;
}

double Node::eval() {
    Node myParent = nodes[parent];
    double value;
    if (!visit) visit = 1e-5;
    value = reward/(visit+1e-5) + mcts_v1*sqrt(log(myParent.visit+1e-5)/(visit) +1e-5);
    return value;
}

// 在搜索时间结束后挑选主节点的最佳节点时使用
inline double Node::evalnum() {
    if (visit==0) return 0;
    double value = reward / visit;
    return value;
}

void Node::expand() {
    isexpand = true;
    vector<EncodedCards> valid_actions = states[user].validActions();
    if (states[user].last_action == PASS_HAND) 
        valid_actions.push_back(PASS_HAND.combo);
    Node newnode;
    for (EncodedCards action_ : valid_actions) {
        // TODO: 在此处加上人类逻辑辅助判断
        newnode = newChild(action_);
        nodes[newnode.hash] = newnode;
    }
}


Hash nodeChoose(const Node& node) {
    double best_eval=-1e5; Hash bestnode;
    for (int i=0;i<node.childNum;i++) {
        Node child = nodes[node.children[i]];
        double eval_ = child.eval();
        if (eval_ > best_eval) {
            best_eval = eval_; bestnode = child.hash;
        }
    }
    if (nodes[bestnode].isexpand) return nodeChoose(nodes[bestnode]);
    return bestnode;
}

int node_check(Node& node) {
    int win = 0;
    for (int i=0;i<mcts_game_size;i++) {
        if (randonGame(node.states[0],node.states[1],node.states[2],node.user) == LORD) {
            if (node.user==LORD) win++;
        } else {
            if (node.user!=LORD) win++;
        }
    }
    if (win > mcts_game_size/2) return 1;
    return 0;
}

// 将随机对局的结果向根节点回传
void data_back(Node& node, int reward) {
    while (!node.isroot) {
        node.visit += 1;  //TODO: 此处可以优化
        node.reward += reward;
        node = nodes[node.parent];
    }
}

// mcts()搜索函数
// 在此之前就已经完成了card_guess
EncodedCards mcts(Node& node) {
    clock_t start,finish;
    if (nodes.find(node.hash)!=nodes.end()) node = nodes[node.hash];
    node.isroot = true;
    if (!node.isexpand) node.expand();
    if (node.childNum==0) return 0ull;
    start = clock();
    while (1) {
        finish = clock();
        if ((double)(finish-start)/CLOCKS_PER_SEC >= mctsTimeLimit) break;
        Node choosedNode = nodes[nodeChoose(node)];
        choosedNode.expand();
    }

    int best_node, best_eval=-1e5;
    for (int i=0;i<node.childNum;i++) {
        double num = nodes[node.children[i]].evalnum();
        if (num > best_eval) {
            best_eval = num;
            best_node = i;
        }
    }
    return nodes[node.children[best_node]].from_action;
}





#ifdef Botzone
int main(){

    botzone();

    return 0;
}
#endif

#ifdef local
// int arr[]{1,2,3};
void game_init(vector<Card>& user0_cards, vector<Card>& user1_cards, 
    vector<Card>& user2_cards, Card * lord_cards) {
    random_shuffle(all_cards, all_cards+30);
    for (int i=0;i<9;i++) {
        user0_cards.push_back(all_cards[i]);
        user1_cards.push_back(all_cards[i*3+1]);
        user2_cards.push_back(all_cards[i*3+2]);
    }
    
    lord_cards[0] = all_cards[27]; 
    lord_cards[1] = all_cards[28]; 
    lord_cards[2] = all_cards[29];
    user0_cards.insert(user0_cards.end(),lord_cards, lord_cards+3);
}

string json_sender(int a) {
    return "";
}

void json_receiver(Json::Value& ai_action) {

}

void node_Guess(Node& node, User user, State& state, int * cards, Card * lord_cards) {
    
}


//暂时不考虑规则，规则处理是botzone的事情
bool game_move() {
    return true;
}


// 暂时先不考虑规则，认为玩家足够自觉，因为在出牌时考虑了规则
void game() {
    // 三个变量分别表示三个玩家的手牌，在整局游戏中都会使用    
    vector<Card> user0_cards, user1_cards, user2_cards;
    // 地主的三张牌
    Card lord_cards[3] = {}; User user(USER0);
    int cards[3] = {12,9,9};
    #ifdef debug
    Card user0_cds[12] = {24,25,26,27,28,32,33,36,37,44,52,53}
    , user1_cds[9]={29,34,40,41,45,46,47,48,49}
    , user2_cds[9] = {30,31,35,38,39,42,43,50,51};
    vector<Card> user0_cards(user0_cds,user0_cds+12), user1_cards(user1_cds,user1_cds+9), user2_cards(user2_cds,user2_cds+9);
    Card lord_cards[3] = {27,28,32} ;
    #else
    game_init(user0_cards, user1_cards, user2_cards, lord_cards);  //初始化游戏，分配手牌
    #endif
    #ifdef debug
    for (auto i = user0_cards.begin(); i != user0_cards.end(); i++) {cout << card_name[*i] << ','; }cout << endl;
    for (auto i = user1_cards.begin(); i != user1_cards.end(); i++) {cout << card_name[*i] << ','; }cout << endl;
    for (auto i = user2_cards.begin(); i != user2_cards.end(); i++) {cout << card_name[*i] << ','; }cout << endl;
    #endif
    State states[3]{user0_cards, user1_cards, user2_cards};
    // Node mainNode(states[0], states[1], states[2], user);
    while(1) {
        // Json::Value ai_action; 
        // AI(json_sender(user), ai_action);
        // json_receiver(ai_action);
        Node node;
        node_Guess(node, user, states[user], cards, lord_cards);
        EncodedCards encoded_action = mcts(node);
        vector<Card> action = states[user].decodeAction(encoded_action);   
        cards[user] -= action.size();     
        #ifdef debug

        cout << "user" << user<<": ";
        for (int i=0;i<action.size();i++) {
            cout << card_name[action[i]] <<',';
        }
        if (action.size()==0) cout << "pass"; 
        cout << endl;         

        #endif
        if (states[user].my_cards.size()==0) break;
        if (action.size()!=0) states[next(user)].last_action = Hand(action);
        else states[next(user)].last_action = states[user].last_action;
        if (states[user].last_action == states[last(user)].last_action) 
        states[next(user)].last_action=PASS_HAND;

        // if (!game_move()) // include gameover check
        //     break;  
        user = next(user);
    }

}


#ifdef test
int main() {
    // const char card_name[] = {'3','4'};
    srand();
    Card user0_cds[12] = {24,25,26,27,28,32,33,36,37,44,52,53}
    , user1_cds[9]={29,34,40,41,45,46,47,48,49}
    , user2_cds[9] = {30,31,35,38,39,42,43,50,51};
    // vector<Card> user0_cards, user1_cards, user2_cards;
    vector<Card> user0_cards(user0_cds,user0_cds+12), user1_cards(user1_cds,user1_cds+9), user2_cards(user2_cds,user2_cds+9);
    // Card lord_cards[3] = {};
    Card lord_cards[3] = {9,6,9};
    // game_init(user0_cards, user1_cards, user2_cards, lord_cards);  //初始化游戏，分配手牌  
    for (auto i = user0_cards.begin(); i != user0_cards.end(); i++) {cout << card_name[*i] << ','; }cout << endl;
    for (auto i = user1_cards.begin(); i != user1_cards.end(); i++) {cout << card_name[*i] << ','; }cout << endl;
    for (auto i = user2_cards.begin(); i != user2_cards.end(); i++) {cout << card_name[*i] << ','; }cout << endl;
    // cout << lord_cards[0]+3<<',' << lord_cards[1]+3<<',' << lord_cards[2]+3;
    // vector<int> last_action;
    State state0(user0_cards), state1(user1_cards), state2(user2_cards);
    state0.last_action = PASS_HAND;
    User fina = randonGame(
        state0,state1,state2,USER0
    );
    cout << fina << endl;
}
#else
int main() {
    game();
    return 0;
}
#endif  //test


#endif  //local
