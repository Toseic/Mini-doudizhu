#include "miniddz.h"

#define local
// #define Botzone

void botzone(string line=""){
	using namespace doudizhu;
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
    DoudizhuState state(my_cards, last_action);
    
    // 根据当前游戏状态，生成全部可行动作，用EncodedCards编码
    vector<EncodedCards> valid_actions = state.validActions();
    
    // 随机选择得到的动作，用牌张列表表示（0-53编码）
    vector<Card> action; 
    
    // 随机选择得到的动作在所有可行动作中的序号
    unsigned random_action_id;
    srand(time(nullptr));

    // 之前人都Pass了，我就得出牌，不能pass
    if (state.last_action.isPass()){
        random_action_id = rand() % valid_actions.size();
        action = state.decodeAction(valid_actions[random_action_id]);
    } else {
        // 之前人没有都Pass，我可以选择pass
        random_action_id = rand() % (valid_actions.size() + 1);
        // 此时我方动作选择不是pass，需要计算具体action
        if (random_action_id != valid_actions.size()){
            action = state.decodeAction(valid_actions[random_action_id]);
        }
    }
    Json::Value result, response(Json::arrayValue);
    for (Card c : action){
        response.append(c);
    }
    #ifdef Botzone
    result["response"] = response;
    Json::FastWriter writer;
	cout << writer.write(result) << endl;
    #endif
    #ifdef local
    // TODO: 设置返回接口
    #endif
}

#ifdef Botzone
int main(){

    botzone();

    return 0;
}
#endif

#ifdef local

void game() {

}



int main() {
    return 0;
}
#endif
