#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <fstream>
#include <windows.h>
#include <cstring>
#include <assert.h>
#include <cmath>
#include "jsoncpp/json.h"


using namespace std;
namespace doudizhu{

    // 所有的牌，在发牌时使用
    int all_cards[30]={24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53};
    const char card_name[] = {'3','3','3','3','4','4','4','4','5','5','5','5','6','6','6','6','7','7','7','7','8','8','8','8','9','9','9','9','T','T','T','T','J','J','J','J','Q','Q','Q','Q','K','K','K','K','A','A','A','A','2','2','2','2','w','W'};

    // 游戏使用的牌总数，不要修改
    const int MAX_CARD_NUM = 54;    
    // 游戏使用的牌类型的数目，不要修改
    const int MAX_CARD_TYPE_NUM = 15;

    // 对每种牌的数目编码，每种占4bit
    typedef unsigned long long EncodedCards;
    const EncodedCards NO_CARDS = 0ull;

    // bitmap，用于枚举子集
    typedef unsigned long long Bitmap;
    const Bitmap EMPTY_SET = 0ull;

    // 牌张按照3-JOKER的顺序从0-53编号
    typedef int Card;

    // // botzone()函数的工作模式
    // typedef int Workmode;

    // 定义牌张类型：一共15种不同大小的牌
    typedef enum{
        THREE = 0, FOUR = 1, FIVE = 2, SIX = 3, 
        SEVEN = 4, EIGHT = 5, NINE = 6, TEN = 7, 
        JACK = 8, QUEEN = 9, KING = 10, ACE = 11,
        TWO = 12, Joker = 13, JOKER = 14
    } CardType;

    //定义botzone()函数中的模式
    typedef enum {
        RANDON = 0, WORK = 1
    } WorkMode;

    typedef enum {
        WIN = 1, LOSE = 0
    } GameResult;

    // 游戏中最小的牌张类型（可修改为NINE，适配迷你斗地主）
    const CardType START_CARD = NINE;

    //user 取值为0，1，2
    typedef enum{
        USER0 = 0,
        USER1 = 1,
        USER2 = 2
    } User;

    // 主牌类型
    // 对于连牌 (456789:SINGLE; 334455:PAIR; JJJQQQ:TRIPLET, 77778888:QUADRUPLE)
    // 对于含副牌的 (333A:TRIPLET; 222255JJ:QUADRUPLE) 
    // 对于连牌含副牌的 (34KKKAAA:TRIPLET; 4444555589JQ:QUADRUPLE)
    typedef enum{
        PASS = 0, // 过
        SINGLE = 1, // 单张
        PAIR = 2, // 对子
        TRIPLET = 3, // 三张
        QUADRUPLE = 4, // 四张
        ROCKET = 5 //火箭
    } MainCardComboType;

    // 某种主牌类型要想形成合法序列（顺子、连对、飞机、连炸），所需的最小长度
    const int SEQ_MIN_LENGTH[] = {0, 5, 3, 2, 2, 1};   //TODO: why???

    // 返回具体牌张的类型：（0-14编号，对应于THREE, FOUR,... Joker, JOKER）
    inline CardType cardTypeOf(Card c){
        return CardType((c >> 2) + int(bool(c & 1) && (c >= MAX_CARD_NUM-2)));
    }

    // 将牌的数目向量转为牌张序列序列
    vector<Card> toCardnumVector(const vector<int>& card_count) {
        vector<Card> card_list(MAX_CARD_NUM);
        for (int type=6; type<=12; type++) {
            Card end = 4*(type+1);
            int num = card_count[type];
            while (end--) {
                if (num==0) break;
                card_list[end] = 1;
                num--;
            }
        }
        card_list[52] = card_count[Joker];
        card_list[53] = card_count[JOKER];
        return card_list;
    }
    // 将牌张序列转为各种牌的数目向量
    vector<int> toCardCountVector(const vector<Card>& card_combo){
        vector<int> card_counter(MAX_CARD_TYPE_NUM);
        for (Card c : card_combo){
            card_counter[cardTypeOf(c)] ++;
        }
        return card_counter;
    }

    // 将各种牌的数目向量转为EncodedCards表示
    EncodedCards toEncodedCards(const vector<int>& card_counter){
        EncodedCards combo = NO_CARDS;
        for (CardType i = START_CARD; i <= JOKER; i = CardType(i+1)){
            // 4i~(4i+3)的位置上编码第i种牌的数目
            combo |= (EncodedCards(card_counter[i]) << (i << 2)); 
        }
        return combo;
    }
    
    // 在EncodedCards中判断某种牌有多少张
    inline int numCardOfEncoded(
        CardType ct, EncodedCards combo){
        return int((combo >> (ct << 2)) & 0xfull);
    }

    // 将encodedcards转化为数目向量格式
    vector<int> toDecodeCards(const EncodedCards combo) {
        vector<int> card_vector(MAX_CARD_TYPE_NUM);
        for (CardType type=NINE;type<=JOKER;type = CardType(type+1)) {
            card_vector[type] = numCardOfEncoded(type, combo);
        }
        return card_vector;
    }

    // 在EncodedCards combo中将CardType ct类型的牌的数目增加n
    inline EncodedCards addToEncodedCards(
        CardType ct, EncodedCards combo, int n=1){
        return combo + (EncodedCards(n) << (ct << 2));
    }

    // 在EncodedCards combo中将CardType ct类型的牌的数目减少n
    inline EncodedCards minusFromEncodedCards(
        CardType ct, EncodedCards combo, int n=1){
        return combo - (EncodedCards(n) << (ct << 2));
    }
    


    // 分析一手牌的类型、大小等
    struct Hand{
        // 一手牌的编码形式，每4位对应一种牌的数目。
        EncodedCards combo;
        // 一手牌主牌的类型：单张（顺）、两张（顺）……火箭
        MainCardComboType type;
        // 一手牌主牌开始的牌张
        CardType start;
        // 主牌的长度：单牌1，对牌1，顺子按其长度来
        int length;
        // 副牌不带、带单还是带双(0,1,2)
        int appendix;
        
        // 解析对手的一手牌（牌张编码、主牌类型、主牌开始、主牌长度、副牌所带数目）
        // card_counter表示这一手牌每种有多少张
        Hand(const vector<int>& card_counter){
            // 初始化这一手牌的编码形式为空
            combo = NO_CARDS;
            // 最多的牌是哪一种（取最先出现的那一种）
            CardType max_freq_card = START_CARD;
            // 最多出现的牌出现了多少次
            int max_freq = 0;
            // 出现次数最多的牌有几种
            int max_freq_length = 0;
            // 最小出现的手牌是什么牌
            CardType min_card = JOKER;
            // 这一手牌一共有多少张
            int total_cards = 0;
            // 编码对手这一手牌到位图中
            combo = toEncodedCards(card_counter);
            // 扫一遍对手的牌，看看出现次数最多的牌、主牌的长度
            for (CardType i = START_CARD; i <= JOKER; i = CardType(i+1)){
                total_cards += card_counter[i];
                // 记录出现次数最多的牌是哪一种，及其出现次数
                if (card_counter[i] > max_freq){
                    max_freq = card_counter[i];
                    max_freq_card = i;
                    max_freq_length = 1;
                } else if (card_counter[i] == max_freq){
                    max_freq_length ++;
                }
                // 记录出现的最小牌张
                if (card_counter[i] != 0 && i < min_card){
                    min_card = i;
                }
            }
            // 下面判断主要牌型
            // 空牌，PASS
            if (max_freq == 0){
                type = PASS;
                start = START_CARD;
                length = 0;
                appendix = 0;
                return; 
            }
            // 火箭
            if (min_card == Joker && total_cards == 2){
                type = ROCKET;
                start = Joker;
                length = 2;
                appendix = 0;
                return;
            }
            // 序列牌（单、双、三、四：独立出现或者连续出现）
            type = MainCardComboType(max_freq);
            // 主牌开始的牌张
            start = max_freq_card;
            // 主牌长度
            length = max_freq_length;
            // 副牌带0,1,2张
            appendix = total_cards / length - int(type);   //TODO: why???
        }
        Hand() {type = PASS;}
        bool isPass(){
            return type == PASS;
        }
        bool isSingle(){
            return type == SINGLE && length == 1;
        }
        bool isPair(){
            return type == PAIR && length == 1;
        }
        bool isBomb(){
            return type == QUADRUPLE && length == 1 && appendix == 0;
        }
        bool isRocket(){
            return type == ROCKET;
        }

        // 判断是三带(0,1,2)还是四带(0,2,4)
        bool isTripletOrQuadruple(){
            return (type == TRIPLET || type == QUADRUPLE) && length == 1; 
        }
        
        // 判断有无副牌
        bool hasAppendix(){
            return appendix > 0;
        }
        
        // 含有副牌的连续三带、四带也算Chain，但是长度至少得是2
        bool isChain(){
            return ((type == SINGLE && length >= SEQ_MIN_LENGTH[SINGLE])
                || (type == PAIR && length >= SEQ_MIN_LENGTH[PAIR])
                || (type == TRIPLET && length >= SEQ_MIN_LENGTH[TRIPLET])
                || (type == QUADRUPLE && length >= SEQ_MIN_LENGTH[QUADRUPLE]));
        }
    };

    // 使用上一手牌、我方现有的牌，构造游戏状态。可分析我方可行动作
    struct State{ //TODO: 或需要考虑加快数据传输速度，以编码格式传输
        // TODO: 增加改变state的函数

        // 轮到我出的时候，我有什么牌
        vector<Card> my_cards;
        // 我各种牌都有多少张
        vector<int> my_card_counter;
        // 上一手出了什么牌
        Hand last_action;

        // 传入我有的牌、对手上一次出的牌（0-53）编码
        State(vector<Card> mine, const vector<Card>& last): 
            my_cards(mine), 
            my_card_counter(toCardCountVector(mine)),
            last_action(toCardCountVector(last)){}

        State(vector<Card> mine): 
            my_cards(mine), 
            my_card_counter(toCardCountVector(mine)){}  

        // State(EncodedCards mine, const EncodedCards last=0ull):
        //     my_cards(toCardnumVector(toDecodeCards(mine))),
        //     my_card_counter(toDecodeCards(mine)),
        //     last_action(toDecodeCards(last)){}

        // // 在单独修改了mine后，通过这个函数更新my_card_counter
        void refresh() { my_card_counter=toCardCountVector(my_cards); }


        // 我的牌中是否有火箭，如果有，则返回该牌型的EncodedCards表示
        EncodedCards genRocket(){
            EncodedCards action = NO_CARDS;
            if (my_card_counter[Joker] == 1 && my_card_counter[JOKER] == 1){
                action = addToEncodedCards(Joker, NO_CARDS, 1);
                action = addToEncodedCards(JOKER, action, 1);
            }
            return action;
        }
        
        // 返回我的牌中所有炸弹的列表，均用EncodedCards类型表示
        vector<EncodedCards> genBombs(){
            vector<EncodedCards> actions;
            EncodedCards action = NO_CARDS;
            for (CardType i = START_CARD; i <= TWO; i = CardType(i+1)){
                if (my_card_counter[i] == 4){
                    action = addToEncodedCards(i, NO_CARDS, 4);
                    actions.push_back(action);
                }
            }
            return actions;
        }

        // 下一手可以出什么牌，返回所有可能动作的列表，元素是EncodedCards的形式
        // generate_appendix == true: 生成的动作中包含三带单、对，四带两单、对的各种情况，包含带的副牌
        // generate_appendix == false: 只生成主牌（三带四带的连三连四部分），不含带的副牌，且主牌部分不重复
        vector<EncodedCards> validActions(bool generate_appendix=true){
            // 待返回动作列表
            vector<EncodedCards> actions;
            // 用于临时保存同一手主牌对应的不同副牌选择
            vector<EncodedCards> appendix_actions;
            // 用于构造主牌动作
            EncodedCards action = NO_CARDS;
            // 用于构造副牌选择
            EncodedCards appendix_action = NO_CARDS;
            // 如果有火箭，生成火箭作为动作，否则为NO_CARDS
            EncodedCards rocket = genRocket();
            // 生成当前我方持有的炸弹列表
            vector<EncodedCards> bombs = genBombs();

            // 上家牌非Pass并且带副牌，生成动作才需考虑副牌，否则只考虑我方意愿
            generate_appendix = generate_appendix && 
                (last_action.hasAppendix() || last_action.isPass()); //TODO: why?
            
            // 如果轮到我先出牌
            if (last_action.isPass()){
                // 生成单、对（范围从最小牌到大王）
                for (CardType i = START_CARD; i <= JOKER; i = CardType(i+1)){
                    // j=1单/j=2对
                    for (int j = 1; j <= 2 && j <= my_card_counter[i]; ++ j){
                        action = addToEncodedCards(i, NO_CARDS, j);
                        actions.push_back(action);
                    }
                }

                // 生成三带，四带（有副牌当且仅当generate_appendix，范围从最小牌到2）
                for (CardType i = START_CARD; i <= TWO; i = CardType(i+1)){
                    // 如果牌有3张，考虑三带；如果有4张，三带、四带都要考虑
                    for (int j = 3, num_appendix; j <= my_card_counter[i]; ++ j){
                        action = addToEncodedCards(i, NO_CARDS, j);
                        if (generate_appendix){
                            // 是三带1还是四带2
                            num_appendix = j == 3 ? 1 : 2;
                            // 带单牌还是双牌
                            for (int k = 1; k <= 2; ++ k){
                                // 生成副牌
                                appendix_actions = generateAppendix(i, 1, num_appendix, k); //TODO: go back
                                for (EncodedCards ec : appendix_actions){
                                    appendix_action = ec;
                                    // 副牌可以直接加到主牌动作上（位串设计保证了这一点）
                                    actions.push_back(action + appendix_action);
                                }
                            }
                        // 如果不生成副牌，这里不添加四张，因为后面添加了
                        } else if (j < QUADRUPLE){
                            actions.push_back(action);
                        }
                    }
                }
                // accumulated_length[1,2,3,4]: 统计到某种牌时，记录以其为结尾的最长（单/对/三/四）连牌长度
                vector<int> accumulated_length(5);
                // 暂时保存action的值
                vector<EncodedCards> a(5), _a(5);
                // 生成连续牌：连单、连双、连三（带）、连四（带）
                for (CardType i = START_CARD; i <= ACE; i = CardType(i+1)){
                    for (int j = 1; j <= my_card_counter[i]; ++ j){
                        // 能进循环说明牌张数目够
                        accumulated_length[j] ++;
                        // 将j张类型i的牌添加到a[j]中
                        a[j] = addToEncodedCards(i, a[j], j);    
                    }
                    for (int j = my_card_counter[i]+1; j <= 4; ++ j){
                        // 进了这个循环说明牌张数目不够，清空连牌长度，连牌断裂
                        accumulated_length[j] = 0;
                        a[j] = NO_CARDS;
                    }
                    for (int j = 1; j <= 4; ++ j){
                        _a[j] = a[j];
                    }
                    // 考虑连单、双、三、四的序列
                    for (int j = 1, num_appendixes; j <= 4; ++ j){
                        // 当前序列以i结尾，以k开始
                        for (CardType k = CardType(i-accumulated_length[j]+1); 
                            // j重的牌序列最短长度要大于等于SEQ_MIN_LENGTH[j]，因此k最多到i-SEQ_MIN_LENGTH[j]+1
                            k <= i-SEQ_MIN_LENGTH[j]+1; k = CardType(k+1)){
                            action = _a[j];
                            if (generate_appendix && j >= 3){
                                // 如果j=3，那么需要为每段生成一份副牌，如果j=4，那么需要为每段生成2份副牌
                                num_appendixes = j == 3 ? 1 : 2;
                                // 副牌是带单牌还是对牌（l=1单，l=2对）
                                for (int l = 1; l <= 2; ++ l){
                                    // 生成所有以牌种i为结尾，长度为i-k+1的连三/连四的副牌
                                    //（每段配num_appendixes份副牌，并且副牌张数为l(1/2)）
                                    appendix_actions = generateAppendix(i, i-k+1, num_appendixes, l);
                                    for (EncodedCards ec : appendix_actions){
                                        appendix_action = ec;
                                        actions.push_back(action + appendix_action);
                                    }
                                }
                            } else {
                                actions.push_back(action);
                            }
                            // 考虑过k开始到i-SEQ_MIN_LENGTH[j]+1的子序列后，考虑从k+1开始的子序列
                            _a[j] = minusFromEncodedCards(k, _a[j], j);
                        }
                    }
                }
                // 生成火箭、炸弹的情况在最后考虑了，这里不返回。
            } else if (last_action.isRocket()){
                // 对面火箭，直接开摆，返回空动作集
                return actions;
            } else if (last_action.isBomb()){
                // 对面是炸弹，找更大的炸弹
                for (EncodedCards ec : bombs){
                    // 炸弹比较只需要比较bitmap大小即可
                    if (ec > last_action.combo){
                        action = ec;
                        actions.push_back(action);
                    }
                }
                // 考虑火箭的情况
                if (rocket != NO_CARDS){
                    actions.push_back(rocket);
                }
                // 直接返回，下面非炸弹的情况才需要考虑所有能打的炸弹
                return actions;
            } else if (last_action.isSingle() || last_action.isPair()){
                // 考虑对面出单牌、对牌的情况
                for (CardType i = CardType(last_action.start+1); i <= JOKER; i = CardType(i+1)){
                    // 如果我的牌够出，那么直接加入动作列表
                    if (my_card_counter[i] >= last_action.type){
                        action = addToEncodedCards(i, NO_CARDS, last_action.type);
                        actions.push_back(action);
                    }
                }
                // 生成炸弹火箭最后考虑了，这里不返回
            } else if (last_action.isTripletOrQuadruple()){
                // 三带、四带等（不连）
                for (CardType i = CardType(last_action.start+1); i <= TWO; i = CardType(i+1)){
                    if (my_card_counter[i] >= last_action.type){
                        action = addToEncodedCards(i, NO_CARDS, last_action.type);
                        // 如果需要生成副牌，那么将其考虑到action中
                        if (generate_appendix){
                            appendix_actions = generateAppendix(i);
                            for (EncodedCards ec : appendix_actions){
                                appendix_action = ec;
                                actions.push_back(action + appendix_action);
                            }
                        } else if (last_action.type < QUADRUPLE){
                            // 后面会添加炸弹，所以这里不重复添加，只加入三张
                            actions.push_back(action);
                        }
                    }
                }
                // 生成炸弹火箭的情况最后考虑了，这里不返回    
            } else if (last_action.isChain()){
                // 连续牌：可能为连单、双、三、四。对于连三、四还可能带副牌
                // 当前找到的序列长度
                int cur_length = 0;

                // 从前一手牌最小牌张+1开始考虑，寻找连牌
                for (CardType i = CardType(last_action.start+1); i <= ACE; i = CardType(i+1)){
                    // 如果我拥有的此种牌张数目大于上一手序列的重复数
                    if (my_card_counter[i] >= last_action.type){
                        cur_length ++;
                        action = addToEncodedCards(i, action, last_action.type);
                        // 如果当前发现的序列长度超过上一手序列的长度，将序列头部剪去
                        if (cur_length > last_action.length){
                            action = minusFromEncodedCards(
                                CardType(i-last_action.length), action, last_action.type);
                        }
                        // 如果序列长度达标了，可以加入动作列表
                        if (cur_length >= last_action.length){
                            if (generate_appendix){
                                appendix_actions = generateAppendix(i);
                                for (EncodedCards ec : appendix_actions){
                                    appendix_action = ec;
                                    actions.push_back(action + appendix_action);
                                }
                            } else {
                                actions.push_back(action);
                            }
                        }                    
                    } else {
                        cur_length = 0;
                        action = NO_CARDS;
                    }
                }
            }
            // 最后考虑炸弹的情况
            for (EncodedCards ec : bombs){
                action = ec;
                actions.push_back(action);
            }
            // 最后考虑火箭的情况
            if (rocket != NO_CARDS){
                actions.push_back(rocket);
            }
            return actions;
        }
        
        // 如果只传第一个参数意味着是针对对手的牌型反制，否则是在上一手Pass的情况下随意出牌
        // 参数分别为：我方主牌最大牌张、主牌序列长度、带牌数目（1单/对或2单/对）、带单(1)或双(2) -> 生成备选副牌列表
        vector<EncodedCards> generateAppendix(CardType end_type,
            int seq_length = 1, int num_appendixes = 1, int appendix_type = 1){
            // 存储所有可能的副牌
            vector<EncodedCards> appendixes;
            EncodedCards appendix = NO_CARDS;
            // 依据上家出的牌是三带还是四带还是pass，如果pass则采用输入的参数
            if (last_action.type == TRIPLET){
                // 三带主牌部分长度
                seq_length = last_action.length;
                // 三带副牌部分为1单或者1对
                num_appendixes = 1;
                // 三带副牌部分为单还是双
                appendix_type = last_action.appendix;
            } else if (last_action.type == QUADRUPLE){
                // 四带主牌部分长度
                seq_length = last_action.length;
                // 四带副牌部分为2单或者2对
                num_appendixes = 2;
                // 四带副牌部分为单还是双
                appendix_type = last_action.appendix;
            } else if (last_action.isPass()){
                // 按照输入的seq_length来
                // 按照输入的num_appendixes来
                // 按照输入的appendix_type来
                // 所以这里不用写任何东西
            }
            // 有多少种副牌可以用
            int useable_appendix_count = 0;
            // 需要多少种副牌
            int all_appendix_needed = seq_length * num_appendixes;
            // 保存可用的副牌种类
            vector<CardType> useable_appendix_set;
            // 遍历我方手牌，看看那些种类的牌数目够做副牌
            for (CardType i = START_CARD; i <= JOKER; i = CardType(i+1)){
                // 如果我有的牌大于副牌所需数目，并且当前的牌不在序列范围之内
                if (my_card_counter[i] >= appendix_type &&
                    (i > end_type || i <= end_type - seq_length)){
                    useable_appendix_count ++;
                    // 标记该种牌可以用作副牌
                    useable_appendix_set.push_back(i);
                }
            }
            // 如果可用的副牌种类数目不够，返回空集
            if (useable_appendix_count < all_appendix_needed){
                return appendixes;
            }
            
            // 临时保存副牌选择子集
            Bitmap appendix_subset = (1ull << all_appendix_needed) - 1ull;
            // 临时保存appendix_subset
            Bitmap s;
            // 副牌选择子集的范围
            Bitmap appendix_subset_limit = 1ull << useable_appendix_count;

            // Gosper's Hack Algorithm 
            // 枚举C(usable_appendix_count, all_appendixes_needed)个可行的副牌选择子集
            Bitmap lb, r;
            while (appendix_subset < appendix_subset_limit){
                s = appendix_subset;
                appendix = NO_CARDS;
                // 解析生成的子集，将之对应到牌的类别上
                for (int j = 0; s != EMPTY_SET; ++ j){
                    if (s & 1ull){
                        appendix = addToEncodedCards(useable_appendix_set[j], appendix, appendix_type);   
                    }
                    s >>= 1;
                }
                appendixes.push_back(appendix);
                
                // 以下代码负责枚举子集，不用管这一部分
                lb = appendix_subset & -appendix_subset;
                r = appendix_subset + lb;
                appendix_subset = (
                    (appendix_subset ^ r) >> (__builtin_ctzll(lb) + 2)) | r;
            }

            return appendixes;
        }

        
        // 对validAction生成的可能动作，解析出实际要出的牌张
        vector<Card> decodeAction(EncodedCards encoded_action,int ins){
            vector<Card> action;
            CardType ct;
            int encoded_ct_num;
            // 对每一张我有的牌，看看我打算打出去的动作中需不需要这张牌
            for (Card c : my_cards){
                ct = cardTypeOf(c);
                encoded_ct_num = numCardOfEncoded(ct, encoded_action);
                // 如果需要，就加入输出列表，并且在打出的动作中删掉一张这种牌
                if (encoded_ct_num > 0){
                    encoded_action = minusFromEncodedCards(ct, encoded_action, 1);
                    action.push_back(c);
                }
            }
            return action;
        }

        // 对validAction生成的可能动作，解析出实际要出的牌张
        // 同时去除自己的手牌中要出的牌
        vector<Card> decodeAction(EncodedCards encoded_action){
            vector<Card> action,cards1;
            CardType ct;
            int encoded_ct_num;
            // 对每一张我有的牌，看看我打算打出去的动作中需不需要这张牌
            for (vector<Card>::iterator c = my_cards.begin(); c!=my_cards.end(); ++c) {
                ct = cardTypeOf(*c);
                encoded_ct_num = numCardOfEncoded(ct, encoded_action);
                if (encoded_ct_num > 0) {
                    encoded_action = minusFromEncodedCards(ct, encoded_action, 1);
                    action.push_back(*c);
                } else cards1.push_back(*c);
            }
            my_cards = cards1;
            refresh();
            return action;
        }
    };


}