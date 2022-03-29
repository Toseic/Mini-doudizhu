#include "miniddz.h"


#define local
#define test

// #define Botzone
using namespace doudizhu;
vector<int> empty_vector(MAX_CARD_TYPE_NUM);
const Hand PASS_HAND = Hand(empty_vector);

LARGE_INTEGER seed;
inline int rand_int(int max) {
    QueryPerformanceFrequency(&seed);
    QueryPerformanceCounter(&seed);
    srand(seed.QuadPart);
    return (int)rand()%(long long)max;
}

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
User randonGame(vector<Card>& card0, vector<Card>& card1, 
        vector<Card>& card2, Hand last_action_, User user) {
    State states[3]{card0,card1,card2};
    states[user].last_action = last_action_;
    int actionID;
    vector<Card> action;
    // for (auto i = valid_actions.begin();i!=valid_actions.end();i++) {
    //     vector<Card> action = states[0].decodeAction(*i);
    //     for (auto j = action.begin();j!=action.end();j++) {
    //         cout <<card_name[*j] << ',';
    //     }
    //     cout << endl;
    // }
    int passTimes = 0;
    // State & state = states[user];
    while (1) {
        // state = states[user];
        if (states[user].my_cards.size()==0) break;
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
        User olduser = user;
        if (user==USER2) user=USER0;
        else user = User(user+1);

        states[user].last_action = states[olduser].last_action;
        if (action.size()!=0) states[user].last_action = Hand(toCardCountVector(action));
        else if (passTimes==2) states[user].last_action = PASS_HAND;
        
    }
    return user; 
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

//暂时不考虑规则，规则处理是botzone的事情
bool game_move() {
    return true;
}


// 暂时先不考虑规则，认为玩家足够自觉，因为在出牌时考虑了规则
void game() {
    // 三个变量分别表示三个玩家的手牌，在整局游戏中都会使用    
    vector<Card> user0_cards, user1_cards, user2_cards;
    // 地主的三张牌
    Card lord_cards[3] = {};
    game_init(user0_cards, user1_cards, user2_cards, lord_cards);  //初始化游戏，分配手牌
    User user(USER0);
    while(1) {
        Json::Value ai_action; 
        AI(json_sender(user), ai_action);
        json_receiver(ai_action);
        if (!game_move()) // include gameover check
            break;  
        if (user==USER2) user = USER0;
        else user = User(user+1);
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
    User fina = randonGame(
        user0_cards,
        user1_cards,
        user2_cards, PASS_HAND, USER0
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
