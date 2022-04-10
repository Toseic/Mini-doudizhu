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
#include <unordered_map>
#include <set>
#include "jsoncpp/json.h"

using namespace std;


// 在程序中储存手牌有两种方式，一种是vector<int>储存
// 对应关系：
//  0  1  2  3  4  5  6    7    8
//  9  10 J  Q  K  A  2  Joker JOKER

// 第二种是编码方式，用一个unsigned long long int 储存一手牌
// 从右向左按照每四位编码一种牌，注意每四位对应于牌的张数
// ，而不是是否有此牌
//   <----   <----    <----  <----
//  0001 0001 0001 0010 0011 0100 0001 0001 0011    二进制表示
//     1    1    1    2    3    4    1    1    3   十六进制表示
//     0x111234113   十六进制表示
//     W    w    2    A    K    Q    J   10    9
//   <----   <----    <----  <----

//  0x000000004  -> 9*4
//  0x110000000  -> 王炸
// 
//  合法性检查范围：
//  仅仅在拆牌和压上家的牌时才会使用，
//  若添加人机交互功能，则仅仅限于合法调试
// 
// 

typedef unsigned long long ull;
typedef int ID;
typedef unsigned long long cards;

typedef enum {
    FAKECARD = -1,
    NINE = 0, TEN = 1,
    JACK = 2, QUEEN = 3,
    KING = 4, ACE = 5,
    TWO = 6, Joker = 7, 
    JOKER = 8, UNKNOWNCARD = -2,
} Cardtype;

typedef enum {
    ERRORHAND = 0,
    UNKNOWN = 1,
    PASS = 2,

    SINGLE = 3,
    PAIR = 4,
    THREECARDS = 5,
    LINE_1_5 = 6,
    LINE_1_6 = 7,

    LINE_2_3 = 8,
    LINE_2_4 = 9,
    LINE_2_5 = 10,
    LINE_2_6 = 11,

    PLANE_2 = 12,
    PLANE_3 = 13,
    PLANE_SUPER = 14,
    BOMB = 15,
    JUSTfFOUR = 16,
    ROCKET = 17,

} Handtype;

// 各种handtype的牌张个数
const int mainCardNum[18] = {
    0,0,0,
    1,2,3,5,6,
    6,8,10,12,
    6,9,8,4,4,2
};

// 副牌的类型
typedef enum {
    NONE = -1,
    WITHSINGLE = 0,
    WITHTWOSINGLE = 1,
    WITHPAIR = 2,
    WITHTWOPAIR = 3
} Appendtype;

//各种appendtype的牌张个数
const int appendCardNum[5] = {0,1,2,2,4};

typedef enum {
    USER0 = 0,
    USER1 = 1,
    USER2 = 2,
    LORD = 0,
    FARMER,
} User;

typedef enum {
    ANTI = 0, FREE = 1
} AppendWorkMode;

typedef enum {
    BLIND,
    FULL,
} TransType;

User next(User user) {if (user==USER2) return USER0; return User(user+1);}
User last(User user) {if (user==USER0) return USER2; return User(user-1);}

const cards EMPTY_CARDS = 0ull;
const cards ALL_CARDS = 0x114444444ull;
const cards FULL_MASK = 0x7ull;
const cards ONE_MASK = 0x1ull;
const cards EXP_ONE_MASK = 0x6ull;
const cards TWO_MASK = 0x2ull;
const cards EXP_TWO_MASK = 0x5ull;
const cards THREE_MASK = 0x3ull;
const cards EXP_THREE_MASK = 0x4ull;
const cards FOUR_MASK = 0x4ull;
const cards EXP_FOUR_MASK = 0x3ull;

const int CARD_TYPES = 9;


const cards Rocket = 0x110000000;
const int Line_1_5_num = 2;
const cards Line_1_5[2] = {
    0x000011111ull, // 9 ~ K
    0x000111110ull, // 10 ~ A    
};

const int Line_1_6_num = 1;
const cards Line_1_6 = 
    0x000111111ull; // 9 ~ A
 

const int Line_2_3_num = 4;
const cards Line_2_3[4] = {
// 三张连对
    0x000000222ull,  // 2* (9~J)
    0x000002220ull,  // 2* (10~Q)
    0x000022200ull,  // 2* (J~K)
    0x000222000ull,  // 2* (Q~A)
};

const int Line_2_4_num = 3;
const cards Line_2_4[3] = {
// 四张连对
    0x000002222ull,  // 2* (9~Q)
    0x000022220ull,  // 2* (10~K)
    0x000222200ull,  // 2* (J~A)
};

const int Line_2_5_num = 2;
const cards Line_2_5[2] = {
// 五张连对
    0x000022222ull,  // 2* (9~K)
    0x000222220ull,  // 2* (10~A)
};

const cards Line_2_6 = 
// 六张连对
    0x000222222ull;  // 2* (9~A)

const int BomBs_num = 7;
const cards BomBs[7] = {
    0x000000004ull, // 9
    0x000000040ull, // 10
    0x000000400ull, // J
    0x000004000ull, // Q
    0x000040000ull, // K
    0x000400000ull, // A
    0x004000000ull, // 2
};

const int threeCards_num = 7;
const cards threeCards[7] = {
    0x000000003ull,  // 3* 9
    0x000000030ull,  // 3* 10
    0x000000300ull,  // 3* J
    0x000003000ull,  // 3* Q
    0x000030000ull,  // 3* K
    0x000300000ull,  // 3* A
    0x003000000ull,  // 3* 2
};

const int Pairs_num = 7;
const cards Pairs[7] = {
    0x000000002ull, // 2* 9
    0x000000020ull, // 2* 10
    0x000000200ull, // 2* J
    0x000002000ull, // 2* Q
    0x000020000ull, // 2* K
    0x000200000ull, // 2* A
    0x002000000ull, // 2* 2
};

const int Single_num = 9;
const cards Single[9] = {
    0x000000001ull, // 2* 9
    0x000000010ull, // 2* 10
    0x000000100ull, // 2* J
    0x000001000ull, // 2* Q
    0x000010000ull, // 2* K
    0x000100000ull, // 2* A
    0x001000000ull, // 2* 2
    0x010000000ull, // 2* w
    0x100000000ull, // 2* W

};

const int Plane_2_num = 5;
const cards Plane_2[5]={
    0x000000033ull, // 3*(9~10) 
    0x000000330ull, // 3*(10~J) 
    0x000003300ull, // 3*(J~Q) 
    0x000033000ull, // 3*(Q~K)
    0x000330000ull, // 3*(K~A)
};

const int Plane_3_num = 4;
const cards Plane_3[4] = {
    0x000000333ull, // 3*(9~J) 
    0x000003330ull, // 3*(10~Q) 
    0x000033300ull, // 3*(J~K) 
    0x000333000ull, // 3*(Q~A) 
};

const int Plane_super_num = 5;
const cards Plane_super[5] = {
    0x000000044ull, // 4* (9~10)
    0x000000440ull, // 4* (10~J)
    0x000004400ull, // 4* (J~Q)
    0x000044000ull, // 4* (Q~K)
    0x000440000ull, // 4* (K~A)
};


LARGE_INTEGER seed;
inline int rand_int(int max) {
    QueryPerformanceFrequency(&seed);
    QueryPerformanceCounter(&seed);
    srand(seed.QuadPart);
    return (int)rand()%(long long)max;
}

inline void Srand() {
    QueryPerformanceFrequency(&seed);
    QueryPerformanceCounter(&seed);
    srand(seed.QuadPart);   
}


struct Cards {
    cards mycards;
    Cards(): mycards(EMPTY_CARDS) {}
    Cards(cards cd): mycards(cd) {}
    Cards(vector<int>& cards_vector):
        mycards(encode(cards_vector)) {}
    
    bool has(cards cd) {
        for (int i=0;i<16;++i) {
            if ((mycards<<(i*4)) < (cd<<(i*4))) return false;
        }
        return true;
    }


    bool is(cards cd) { //* operator
        return (mycards == cd);
    }


    void add(Cardtype type, int num) {
        mycards += ((ull)num << type*4);
    }

    static void add(cards& cd, Cardtype type, int num) {
        cd += ((ull)num << type*4);
    }

    void remove(Cardtype type, int num) {
        mycards -= ((ull)num << type*4);
    }

    void remove(cards& cd) {
        mycards -= cd;
    }

    vector<int> decode() {
        vector<int> decoded_cards;
        for (Cardtype i=NINE;i<=JOKER;i=Cardtype(i+1)) {
            int cardnum = (int)(FULL_MASK & (mycards>>i*4));
            for (int j=0;j<cardnum;++j) 
                decoded_cards.push_back(i);
        }
        return decoded_cards;
    }

    static vector<int> decode(cards cd) {
        vector<int> decoded_cards;
        for (Cardtype i=NINE;i<=JOKER;i=Cardtype(i+1)) {
            int cardnum = (int)(FULL_MASK & (cd>>i*4));
            for (int j=0;j<cardnum;++j) 
                decoded_cards.push_back(i);
        }
        return decoded_cards;
    } 
    static int cards_sum(cards cd) {
        int sum = 0,cardnum;
        for (Cardtype i=NINE;i<=JOKER;i=Cardtype(i+1)) {
            cardnum = (FULL_MASK & (cd>>i*4));
            sum += cardnum;
        }
        return sum;
    }
    static int card_types(cards cd) {
        int type_sum = 0,cardnum;
        for (Cardtype i=NINE;i<=JOKER;i=Cardtype(i+1)) {
            cardnum = (FULL_MASK & (cd>>i*4));
            if (cardnum) ++type_sum;
        }
        return type_sum;   
    }

    static cards encode(vector<int>& cards_vector) {
        cards cd = EMPTY_CARDS;
        for (vector<int>::iterator i = cards_vector.begin();
            i != cards_vector.end(); ++i) {
            add(cd, Cardtype(*i), 1);
        }
        return cd;
    }
    
    // void operator-=(Hand hd) {
    //     mycards -= hd.action;
    // }


};

// 出的一次牌
struct Hand {
    cards action;
    Handtype type;
    Appendtype append_type;
    Cardtype mainCard;
    int cards_num;
    Hand(cards act, Handtype mytype):
        action(act),type(mytype),append_type(NONE),
        mainCard(UNKNOWNCARD),cards_num(){} 
    Hand(cards act,Appendtype mytype):
        action(act),type(UNKNOWN),append_type(mytype),
        mainCard(UNKNOWNCARD),cards_num(){} 
    Hand(cards act, Handtype mytype, Cardtype mc):
        action(act),type(mytype),append_type(NONE),mainCard(mc) {}
    Hand(cards act,Handtype type_, Appendtype mytype, Cardtype mc):
        action(act),type(type_),append_type(mytype),
        mainCard(mc),cards_num(){}
    Hand():action(EMPTY_CARDS),type(UNKNOWN),mainCard(UNKNOWNCARD) {}
    Hand(cards action_): action(action_),type(UNKNOWN),mainCard(UNKNOWNCARD) {}
    Hand(vector<int>& card_vector): action(Cards::encode(card_vector)),
        mainCard(UNKNOWNCARD),type(UNKNOWN) {}

    void type_check();
    int cardsum() {
        cards_num = mainCardNum[type] + appendCardNum[append_type];
        return cards_num;
    }
    bool operator==(Handtype hd) { return (type==hd); }
    bool operator!=(Handtype hd) { return (type!=hd); }
    friend Hand operator+(const Hand& h1,const Hand& h2) {
        Hand h3;
        h3.action = h1.action + h2.action;
        h3.type_check();
        return h3;
    }
    friend Hand operator+(const Hand& h1,const cards& h2) {
        Hand h3;
        h3.action = h1.action + h2;
        h3.type_check();
        return h3;
    }
    bool operator<(const Hand& hd) {
        if (action < hd.action) return true;
        return false;
    }
    friend bool operator<(const Hand& h1, const Hand& h2) {
        if (h1.action < h2.action) return true;
        return false;
    }
};

const Hand PASS_HAND(EMPTY_CARDS,PASS);
const Hand ERROR_HAND(EMPTY_CARDS,ERRORHAND);

// 某个玩家的某个时间的状态
struct State {
    Cards cards;
    User user;
    Hand last_action;
    Hand myaction;

    State(Cards card0, User user_):
        cards(card0), user(user_) { last_action = PASS_HAND; }
    State(User user_): user(user_) { last_action = PASS_HAND; }

    void anti_action(vector<Hand>& );
    void apart_cards(vector<Hand>& );
    void free_action(vector<Hand>& );
    void rand_action(vector<Hand>& );
    void member_sum(vector<Hand>& , set<Handtype> * );
    void make_decision(vector<Hand>&);

    void find_rocket(vector<Hand>& vh) {
        if ((cards.mycards & Rocket) == ROCKET) 
            vh.push_back(Hand(Rocket,ROCKET,JOKER));
    }
    void find_bomb(vector<Hand>& vh, Cardtype minline = FAKECARD) {
        for (Cardtype i = Cardtype(minline+1); i<=TWO; i=Cardtype(i+1)) {
            if ((cards.mycards & BomBs[i]) == BomBs[i]) 
                vh.push_back(Hand(BomBs[i],BOMB,i));
        }
    }

    void find_single(vector<Hand>& vh, Cardtype minline = FAKECARD) {
        for (Cardtype i = Cardtype(minline+1); i<=JOKER; i=Cardtype(i+1)) {
            if ((cards.mycards & (FULL_MASK<<i*4)) >= (ONE_MASK<<i*4) )
                vh.push_back(Hand(Single[i],SINGLE,i));
        }
    }

    void find_pair(vector<Hand>& vh, Cardtype minline = FAKECARD) {
        for (Cardtype i = Cardtype(minline+1); i<=TWO; i=Cardtype(i+1)) {
            if ((cards.mycards & (FULL_MASK<<i*4)) >= (TWO_MASK<<i*4))
                vh.push_back(Hand(Pairs[i],PAIR,i));
        }
    }

    void find_three_cards(vector<Hand>& vh, Cardtype minline = FAKECARD) {
        for (Cardtype i = Cardtype(minline+1); i<=TWO; i=Cardtype(i+1)) {
            if ((cards.mycards & (FULL_MASK<<i*4)) >= (THREE_MASK<<i*4))
                vh.push_back(Hand(threeCards[i],THREECARDS,i));
        }
    }

    void find_three_cards_only(vector<Hand>& vh, Cardtype minline = FAKECARD) {
        for (Cardtype i = Cardtype(minline+1); i<=TWO; i=Cardtype(i+1)) {
            if ((cards.mycards & (FULL_MASK<<i*4)) == (THREE_MASK<<i*4))
                vh.push_back(Hand(threeCards[i],THREECARDS,i));
        }
    }

    void find_line_1_5(vector<Hand>& vh, Cardtype minline = FAKECARD) {
        for (Cardtype i = Cardtype(minline+1); i<=ACE-4; i=Cardtype(i+1)) {
            if (cards.has(Line_1_5[i])) {
                vh.push_back(Hand(Line_1_5[i],LINE_1_5,Cardtype(i+4)));
            }
        }
    }
    void find_line_2_3(vector<Hand>& vh, Cardtype minline = FAKECARD) {
        for (Cardtype i = Cardtype(minline+1); i<=ACE-2; i=Cardtype(i+1)) {
            if (cards.has(Line_2_3[i])) {
                vh.push_back(Hand(Line_2_3[i],LINE_2_3,Cardtype(i+2)));
            }
        }
    }

    void find_line_2_4(vector<Hand>& vh, Cardtype minline = FAKECARD) {
        for (Cardtype i = Cardtype(minline+1); i<=ACE-3; i=Cardtype(i+1)) {
            if ((cards.has(Line_2_4[i]))) {
                vh.push_back(Hand(Line_2_4[i],LINE_2_4,Cardtype(i+3)));
            }
        }
    }
    void find_line_2_5(vector<Hand>& vh, Cardtype minline = FAKECARD) {
        for (Cardtype i = Cardtype(minline+1); i<=ACE-4; i=Cardtype(i+1)) {
            if ((cards.has(Line_2_5[i]))) {
                vh.push_back(Hand(Line_2_5[i],LINE_2_5,Cardtype(i+4)));
            }
        }
    }

    void find_line_1_6(vector<Hand>& vh) {
        if (
            cards.has(Line_1_6))
                vh.push_back(Hand(Line_1_6,LINE_1_6,ACE));
    }

    void find_plane_2(vector<Hand>& vh, Cardtype minline = FAKECARD) {
        for (Cardtype i = Cardtype(minline+1); i<=ACE-1; i=Cardtype(i+1)) {
            if ((cards.has(Plane_2[i]))) {
                vh.push_back(Hand(Plane_2[i],PLANE_2,Cardtype(i+1)));
            }
        }
    }
    void find_plane_3(vector<Hand>& vh, Cardtype minline = FAKECARD) {
        for (Cardtype i = Cardtype(minline+1); i<=ACE-2; i=Cardtype(i+1)) {
            if ((cards.has(Plane_3[i]))) {
                vh.push_back(Hand(Plane_3[i],PLANE_3,Cardtype(i+2)));
            }
        } 
    }
    void find_plane_super(vector<Hand>& vh, Cardtype minline = FAKECARD) {
        for (Cardtype i = Cardtype(minline+1); i<=ACE-1; i=Cardtype(i+1)) {
            if ((cards.mycards & Plane_super[i]) == Plane_super[i]) {
                vh.push_back(Hand(Plane_super[i],PLANE_SUPER,Cardtype(i+1)));
            }
        } 
    }
};

// 三个玩家的状态放在一起成为一个节点
struct Node {
    State states[3];
    cards lord_card;
    User user;
    static ID nowId;
    ID Id;
    ID parent,children[50];
    Hand fromHand;
    bool isroot, isleaf, isexpand;
    int childNum;
    double visit,reward;

    Node():states{USER0, USER1, USER2},
        isroot(false), isleaf(false), visit(0), 
        reward(0), fromHand(PASS_HAND), childNum(0){}

    Node(User user_):states{USER0, USER1, USER2}, user(user_), 
        isroot(false), isleaf(false), isexpand(false),
        visit(0), reward(0), fromHand(PASS_HAND),childNum(0){}

    // Node(State state0, State state1, State state2, User user_):
    //     states{state0, state1, state2},user(user_) {}

    Node(State state0, State state1, State state2, User user_,cards lord_card_, TransType type):
        states{state0, state1, state2},user(user_),
        isroot(false),isleaf(false),isexpand(false),
        visit(0), reward(0), fromHand(PASS_HAND), 
        lord_card(lord_card_), childNum(0){
            if (type==BLIND) {
                states[next(user)].cards.mycards = EMPTY_CARDS;                
                states[last(user)].cards.mycards = EMPTY_CARDS;                
            }
        }

    void nodeID() {
        // Srand();
        // hash = myHash[rand_int(64)][user] ^ 
        // states[0].cards.mycards ^ states[1].cards.mycards ^ states[2].cards.mycards ; 
        Id = nowId;
        nowId ++;
    }
    bool isLeaf() {
        for (int i=0;i<3;++i) {
            if (states[i].cards.mycards == EMPTY_CARDS) {
                isleaf = true;
                return true;
            }
        }
        isleaf = false;
        return false;
    }
    double eval();
    double reward_eval();
    Node newchild(Hand hd);
    void expand();

};

ID Node::nowId = 0;

// mcd的每一种牌的数目都与cd相同或者更多
bool include(cards mcd,cards cd) {
    for (int i=0;i<16;++i) {
        if ((mcd<<(i*4)) < (cd<<(i*4))) return false;
    }
    return true;
}  

// cd的每一种牌的数目都与mcd相同
bool include_strict(cards mcd,cards cd) {
    for (int i=0;i<16;++i) {
        if ((cd & FULL_MASK<<(i*4))
            && ((mcd & FULL_MASK<<(i*4)) != (cd & FULL_MASK<<(i*4)))) 
            return false;
    }
    return true;
}  

inline int numof(cards mcd, Cardtype ct) {
    return (int)(mcd >> (ct*4) & FULL_MASK);
}

inline bool is(cards mcd, cards cd) {
    return (mcd == cd);
}

inline void remove(cards mcd, Cardtype type, int num) {
    mcd -= ((ull)num << type*4);
}

inline void remove(cards mcd, cards cd) {
    mcd -= cd;
}

// 在进行牌型分类时，找到一手牌的miancard
inline Cardtype find_1(cards cd) {
    Cardtype ans = UNKNOWNCARD;
    for (Cardtype i = NINE; i<=JOKER; i=Cardtype(i+1)) {
        if (((cd >> i*4) & FULL_MASK) == ONE_MASK)
            ans = i; 
    }
    return ans;
}

inline Cardtype find_2(cards cd) {
    Cardtype ans = UNKNOWNCARD;
    for (Cardtype i = NINE; i<=JOKER; i=Cardtype(i+1)) {
        if (((cd >> i*4) & FULL_MASK) == TWO_MASK) 
            ans = i;
    }
    return ans;
}

inline Cardtype find_3(cards cd) {
    Cardtype ans = UNKNOWNCARD;
    for (Cardtype i = NINE; i<=TWO; i=Cardtype(i+1)) {
        if (((cd >> i*4) & FULL_MASK) == THREE_MASK) 
            ans = i;
    }
    return ans;
}

inline Cardtype find_4(cards cd) {
    Cardtype ans = UNKNOWNCARD;
    for (Cardtype i = NINE; i<=JOKER; i=Cardtype(i+1)) {
        if (((cd >> i*4) & FULL_MASK) == FOUR_MASK) 
            ans = i;
    }
    return ans;
}

inline Cardtype cardTypeOf(int c){
    return Cardtype((c >> 2) + int(bool(c & 1) && (c >= 54-2)) - 6);
}

// 
void Hand::type_check() {
    int cards_num = Cards::cards_sum(action);
    int card_type_num = Cards::card_types(action);
    switch (cards_num) {
    case 0:
        type = PASS;
        append_type = NONE;
        break;
    case 1:
        type = SINGLE;
        append_type = NONE;
        mainCard = find_1(action);
        break;
    case 2:
        if (card_type_num == 2 ) {
            if ((action & Rocket) == Rocket) {
                type = ROCKET;
                append_type = NONE;
                mainCard = JOKER;     
                break;           
            } else {
                append_type = WITHTWOSINGLE;
                break;
            }
        } else {
            type = PAIR;
            append_type = NONE;
            mainCard = find_2(action);
            break;
        }
        
    case 3:
        type = THREECARDS;
        append_type = NONE;
        mainCard = find_3(action);
        break;
    case 4:
        if (card_type_num == 1){
            type = BOMB;
            append_type = NONE;
            mainCard = find_4(action);
            break;
        } else {
            type = THREECARDS;
            append_type = WITHSINGLE; 
            mainCard = find_3(action);
            break;            
        }

    case 5:
        // 单顺子
        if (card_type_num == 5) {
            type = LINE_1_5;
            append_type = NONE;
            mainCard = find_1(action);
            break;
        }
        // 四带一 || 三带一对
        if (card_type_num == 2){
            for (int i=0;i<BomBs_num;++i) {
                if ((action & BomBs[i]) == BomBs[i]) {
                    type = JUSTfFOUR;
                    append_type = WITHSINGLE;
                    mainCard = find_4(action);
                    break;                    
                }
            }
            if (type == UNKNOWN) {
                type = THREECARDS;
                append_type = WITHPAIR;
                mainCard = find_3(action);
            }
            break;
        }
        break;
    case 6:
        if (card_type_num == 6 && action == Line_1_6){
            type = LINE_1_6;
            append_type = NONE;
            mainCard = find_1(action);
            break;
        }
        if (card_type_num == 3){
            // 四带两张 or 3*2
            for (int i=0;i<BomBs_num;++i) {
                if ((action & BomBs[i]) == BomBs[i]) {
                    type = JUSTfFOUR;
                    append_type = WITHTWOSINGLE;
                    mainCard = find_4(action);
                    break;
                }
            }
            if (type==UNKNOWN) {
                type = LINE_2_3;
                append_type = NONE;
                mainCard = find_2(action);
            } 
            break;
        }
        // 四张带一对 || 2*3
        for (int i=0;i<BomBs_num;++i) {
            if ((action & BomBs[i]) == BomBs[i]) {
                type = JUSTfFOUR;
                append_type = WITHPAIR;
                mainCard = find_4(action);
                break;
            }
        }
        if (type==UNKNOWN && card_type_num==2) {
            type = PLANE_2;
            append_type = NONE;            
            mainCard = find_3(action);
        }

        break;
    case 8:
    // 航天飞机 || 2*3飞机带小翼 || 四张连对 || 四带两对
        if (card_type_num == 4) {
            for (int i=0;i<Plane_2_num;++i) {
                if ((action & Plane_2[i]) == Plane_2[i]) {
                    type = PLANE_2;
                    append_type = WITHTWOSINGLE;
                    mainCard = find_3(action);
                    break;
                }
            }
            if (type==UNKNOWN) {
                type = LINE_2_4;
                append_type = NONE;                
                mainCard = find_2(action);
            }
            break;
        } else if (card_type_num == 3) {
            type = JUSTfFOUR;
            append_type = WITHTWOPAIR;
            mainCard = find_4(action);
            break;
        } else {
            type = PLANE_SUPER;
            append_type = NONE;            
            mainCard = find_4(action);
        }

        break;
    case 9:
    // 3*3飞机 
        type = PLANE_3;
        append_type = NONE;
        mainCard = find_3(action);
        break;
    case 10:
    // 航天飞机带小翼 || 五连对 || 飞机带大翼
        if (card_type_num == 4) {
            for (int i=0;i<Plane_super_num;++i) {
                if ((action & Plane_super[i]) == Plane_super[i]) {
                    type = PLANE_SUPER;
                    append_type = WITHTWOSINGLE;
                    mainCard = find_4(action);
                    break;                    
                }
            }
            if (type==UNKNOWN) {
                type = PLANE_2;
                append_type = WITHTWOPAIR;
                mainCard = find_3(action);
                break;                
            }
        } else if ( card_type_num==5){
            type = LINE_2_5;
            append_type = NONE;            
            mainCard = find_2(action);
        }

        break;
    case 12:
    // 六连对 || 航天飞机带大翼
        if (card_type_num == 6) {
            type = LINE_2_6;
            append_type = NONE;
            mainCard = find_2(action);
            break;
        } else if (card_type_num == 4 ){
            type = PLANE_SUPER;
            append_type = WITHTWOPAIR;            
            mainCard = find_4(action);
        }
        break;
    default:
        type = ERRORHAND;
        break;
    }

}

// 在findAppend()中对顺子进行额外检查
// check_card是已经减去顺子的cards
void findAppend_extra(set<Hand>& append_hands, const cards check_card, 
    const cards action, int each_num) {
    // 从minline开始
    for (Cardtype i=NINE; i!=TWO;i=Cardtype(i+1)) {
        if ((action >> i*4) & FULL_MASK) {
            if (numof(check_card,i)==1) 
                append_hands.insert(Hand((1<<4*i),WITHSINGLE));
            else if (numof(check_card,i)==2 && each_num == 1) {
                append_hands.insert(Hand(2<<4*i,WITHPAIR));
            }
        }
    }
}


// 找到合适的副牌,不考虑类型，把四种能找到的全找到
// set内部不会重复
// 原则：把所有不会干扰到顺子，飞机，炸弹，三张的单牌和对子拿出来
// 原则：不拆炸弹
// 对顺子的情况进行额外的检查
// set里面可能会有多种相同类型副牌，在函数最后提供一个取最优的过程
void findAppend(vector<Hand>& append_vector, cards mycard) {
    set<cards> inMid_cards;
    set<Hand> append_hands;
    int mycard_num = Cards::cards_sum(mycard);

// 炸弹
    for (int i=0;i<BomBs_num;i++) 
        if (include(mycard, BomBs[i])) {
            mycard -= BomBs[i];
        } 

// 顺子
    if (include(mycard,Line_1_6)) {
        inMid_cards.insert(mycard - Line_1_6);
        findAppend_extra(append_hands, mycard - Line_1_6, Line_1_6, 1);
    } 
    else if (include(mycard, Line_1_5[0])) {
        inMid_cards.insert(mycard - Line_1_5[0]);    
        findAppend_extra(append_hands, mycard - Line_1_5[0], Line_1_5[0], 1);  
    }
    else if (include(mycard, Line_1_5[1])) {
        inMid_cards.insert(mycard - Line_1_5[1]);
        findAppend_extra(append_hands, mycard - Line_1_5[1], Line_1_5[1], 1);
    }
    // 连对
    if (mycard_num >= 6) {
        for (int i=0;i<Line_2_3_num;i++) 
            if (include(mycard, Line_2_3[i])) {
                inMid_cards.insert(mycard - Line_2_3[i]);  
                findAppend_extra(append_hands, mycard - Line_2_3[i], Line_2_3[i], 2);             
            }

        if (mycard_num >= 8) {
            for (int i=0;i<Line_2_4_num;i++) 
                if (include(mycard, Line_2_4[i])) {
                    inMid_cards.insert(mycard - Line_2_4[i]); 
                    findAppend_extra(append_hands, mycard - Line_2_4[i], Line_2_4[i], 2);
                }
        }                
    }


// 三张
    // 需要include_strict，避免将炸弹给拆了
    // 需要避免把飞机考虑进去
    for (int i=0;i<threeCards_num-1;i++)
        if (include_strict(mycard, threeCards[i]) & !include_strict(mycard, threeCards[i+1])) 
            inMid_cards.insert(mycard - threeCards[i]);
    if (include_strict(mycard, threeCards[threeCards_num-1]) ) // 需要include_strict
            inMid_cards.insert(mycard - threeCards[threeCards_num-1]);

// 飞机
    if (mycard_num >= 6) {
        for (int i=0;i<Plane_2_num;i++) 
            if (include(mycard, Plane_2[i]))   
                inMid_cards.insert(mycard - Plane_2[i]);
    }

    int midcard_num = inMid_cards.size();
    cards all_addup = 0ull;

    for (set<cards>::iterator i = inMid_cards.begin();i!=inMid_cards.end();i++) 
        { all_addup += *i; }


    for (Cardtype i=NINE;i<=JOKER;i=Cardtype(i+1)) {
        // 只考虑两张及以下而且在inMid_cards次次都出现
        if (numof(mycard, i) == 1 && numof(all_addup, i) == midcard_num) {
                append_hands.insert(Hand((1ull << i*4),WITHSINGLE));
        } else if (numof(mycard, i) == 2 && numof(all_addup, i) == midcard_num*2) {
                append_hands.insert(Hand((2ull << i*4), WITHPAIR));
        }
    }
         
    // } else { //直接把最小的拿出来

    // }        
// 处理set
    int findSingle = 0;
    int findPair = 0;
    Hand findsingle1,findsingle2,findpair1,findpair2;
    for (set<Hand>::iterator i=append_hands.begin();i!=append_hands.end();i++) {
        if (i->append_type == WITHSINGLE) {
            if (findSingle == 0) {
                findSingle++;
                findsingle1 = *i;
            } else if (findSingle == 1) {
                findSingle ++;
                if (*i < findsingle1) {findsingle2 = findsingle1; findsingle1=*i;}
                else {findsingle2 = *i;}
            } else {
                if (*i < findsingle1) {findsingle2 = findsingle1; findsingle1=*i;}
                else if (*i < findsingle2) {findsingle2 = *i;}
            }
        } else {
            if (findPair == 0) {
                findPair++;
                findpair1 = *i;
            } else if (findPair == 1) {
                findPair++;
                if (*i < findpair1) {findpair2 = findpair1; findpair1 = *i;}
                else {findpair2 = *i;}
            } else {
                if (*i < findpair1) {findpair2 = findpair1; findpair1 = *i;}
                else if (*i < findpair2) {findpair2 = *i;}
            }
        }
    }

    if (findSingle == 0) {append_vector.push_back(PASS_HAND); append_vector.push_back(PASS_HAND);}
    else if (findSingle == 1) {append_vector.push_back(findsingle1); append_vector.push_back(PASS_HAND);}
    else {append_vector.push_back(findsingle1); 
        append_vector.push_back(Hand(findsingle1.action+findsingle2.action, WITHTWOSINGLE)); }
    
    if (findPair == 0) {append_vector.push_back(PASS_HAND); append_vector.push_back(PASS_HAND);}
    else if (findPair == 1) {append_vector.push_back(findpair1); append_vector.push_back(PASS_HAND); }
    else { append_vector.push_back(findpair1);
        append_vector.push_back(Hand(findpair1.action+findpair2.action,WITHTWOPAIR));
    }
    
}



// last_action在传入时就应当已经被check过type和maincard了
// 在这个函数里面仅仅找出所有可以反制对手的牌
// 副牌在此会考虑，在拆牌后会加上
void State::anti_action(vector<Hand>& actions) {
    vector<Hand> append_hands;
    find_rocket(actions);
    actions.push_back(PASS_HAND);
    
    if (last_action.type == ROCKET) return ;
    // 帮助队友
    else if (last_action.type == BOMB || last_action.type == JUSTfFOUR) 
        find_bomb(actions, last_action.mainCard);
    else {
        find_bomb(actions);
        switch (last_action.type) {
        case SINGLE:
            find_single(actions, last_action.mainCard);
            break;
        case PAIR:
            find_pair(actions, last_action.mainCard);
            break;
        case THREECARDS:
            find_three_cards(actions, last_action.mainCard);
            break;
        case LINE_1_5:
            find_line_1_5(actions, last_action.mainCard);
            break;
        case LINE_2_3:
            find_line_2_3(actions, last_action.mainCard); 
            break;
        case PLANE_2:
            find_plane_2(actions, last_action.mainCard);           
            break;
        case LINE_1_6:
            break;
        case LINE_2_4:
            find_line_2_4(actions, last_action.mainCard);
            break;
        case LINE_2_5:
            find_line_2_5(actions, last_action.mainCard);
            break;
        case PLANE_3:
            find_plane_3(actions, last_action.mainCard);
            break;
        case PLANE_SUPER:
            find_plane_super(actions, last_action.mainCard);
            break;
        default:
            break;
        }        
    }
    if (last_action.append_type == NONE) return;
    findAppend(append_hands, cards.mycards);
    // int vector_size = actions.size();
    // for (int i=0;i<vector_size;i++) {
    //     if (actions[i].type != BOMB && actions[i].type != ROCKET && actions[i].type != PASS) {
    //         if (append_hands[last_action.append_type] == PASS || 
    //       /  !cards.has(actions[i].action+append_hands[last_action.append_type].action)) {
    //             actions.erase(i);
    //         }
    //     }
    // }

    vector<Hand>::iterator i=actions.begin();
    while (i!=actions.end()) {
        if (i->type != BOMB && i->type != ROCKET && i->type != PASS) {
            if (append_hands[last_action.append_type] == PASS || 
                !cards.has(i->action+append_hands[last_action.append_type].action)) {
                i = actions.erase(i);
            } else {
                (i->action) += append_hands[last_action.append_type].action;
                i->type_check();
                i++;
            }
        } else {
            i++;
        }
    }

}

// 在一个vector<action>里面统计所有牌参与到了多少种handtype
// 暂时没被用到
void actions_type_sum(vector<Hand>& actions, set<Handtype>& action_types) {
    for (vector<Hand>::iterator i=actions.begin();i!=actions.end();++i) {
        if (i->type == BOMB) action_types.insert(JUSTfFOUR);
        else action_types.insert(i->type);
    }
}



// 列出自由出牌时的所有可出的牌
// 在此不考虑副牌
// 原则：不拆炸弹
void State::free_action(vector<Hand>& actions) {
    int card_num = Cards::cards_sum(cards.mycards);
    ull all_bomb = 0ull;
    find_bomb(actions);
    for (vector<Hand>::iterator i=actions.begin();i!=actions.end();i++) {
        all_bomb += i->action; }
    cards.mycards -= all_bomb;
    find_rocket(actions);
    find_line_1_5(actions);
    find_single(actions);
    find_pair(actions);
    find_three_cards(actions);

    if (card_num>=6 ) {
        find_line_2_3(actions);     
        find_line_1_6(actions);
        find_plane_2(actions);
    } 
    if (card_num >= 8) {
        find_plane_super(actions);
        find_plane_3(actions);
        find_line_2_5(actions);
        find_line_2_4(actions);
    }
    cards.mycards += all_bomb;
}

void State::rand_action(vector<Hand>& actions) {
    if (last_action == PASS) {
        free_action(actions);
        // 减少单牌的几率
        if (actions.size() > 8) {
            vector<Hand>::iterator i = actions.begin();
            while (i != actions.end()) {
                if (i->type == SINGLE && rand_int(2)) i = actions.erase(i);
                else i++;
            }
        }
        Cardtype choosed_single=FAKECARD,choosed_pair=FAKECARD;
        for (Cardtype i = NINE;i<=KING;i = Cardtype(i+1)) {
            if ((numof(cards.mycards, i) == 1) ) {
                choosed_single = i;
            } else if ((numof(cards.mycards, i) == 2)) {
                choosed_pair = i;
            }
        }
        int actions_size = actions.size();
        if (choosed_single != FAKECARD) {
            for (int i=0;i<actions_size;i++) {
                if (actions[i].type == THREECARDS) {
                    actions.push_back(
                        Hand(actions[i].action+(1ull << choosed_single*4),
                        THREECARDS,WITHSINGLE,actions[i].mainCard));
                }
            }            
        }
        if (choosed_pair != FAKECARD) {
            for (int i=0;i<actions_size;i++) {
                if (actions[i].type == THREECARDS) {
                    actions.push_back(
                        Hand(actions[i].action+(2ull << choosed_pair*4),THREECARDS,WITHPAIR,actions[i].mainCard));
                }
            }            
        }

    } else {
        anti_action(actions);
    }

}

void State::member_sum(vector<Hand>& actions, set<Handtype> *members ) {
    for (vector<Hand>::iterator i = actions.begin(); i!=actions.end(); ++i) {
        if (i->type==SINGLE || i->type == PAIR || i->type == THREECARDS || i->type == BOMB) 
            members[i->mainCard].insert(i->type);
        else {
            switch (i->type) {
                case LINE_1_5:
                    for (int j=0;j<5;++j) 
                        members[(i->mainCard)-j].insert(LINE_1_5);
                    break;
                case ROCKET:
                    members[7].insert(ROCKET);
                    members[8].insert(ROCKET);
                    break;
                case LINE_2_3:
                    for (int j=0;j<3;++j) 
                        members[(i->mainCard)-j].insert(LINE_2_3);
                    break;
                case LINE_1_6:
                    for (int j=0;j<6;++j) 
                        members[(i->mainCard)-j].insert(LINE_1_6);
                    break;   
                case LINE_2_4:
                    for (int j=0;j<4;++j) 
                        members[(i->mainCard)-j].insert(LINE_2_4);
                    break;     
                case LINE_2_5:
                    for (int j=0;j<5;++j) 
                        members[(i->mainCard)-j].insert(LINE_2_5);
                    break;  
                case PLANE_2:
                    for (int j=0;j<2;++j) 
                        members[(i->mainCard)-j].insert(PLANE_2);
                    break;  
                case PLANE_3:
                    for (int j=0;j<3;++j) 
                        members[(i->mainCard)-j].insert(PLANE_3);
                    break;    
                case PLANE_SUPER:
                    for (int j=0;j<2;++j) 
                        members[(i->mainCard)-j].insert(PLANE_SUPER);
                    break;                         
                default:
                    break;
            }            
        }
    }
}



// 仅当可以自由出牌时才会使用，通过拆牌减少无效搜索
// 找出有搜索价值的主牌
// 包括大牌，散牌
// 大牌为了使得下两家接不住，散牌为了方便把散牌打掉
// // 一些奇怪的理论：找出出手的
// 副牌处理思路，如果有单牌仅仅只能单打的，如果只有一个，试着把它用三带一打掉
// 如果有两个或以上，把小的那个立刻打掉，剩下的看情况
// 
void State::apart_cards(vector<Hand>& actions_) {
    Cards mycard_cp = cards.mycards;
    vector<Hand> focusHand;
    set<Handtype> members[9];
    vector<Hand> actions;
    vector<Hand> append_hands; //格式 single twosingle pair twopair
    free_action(actions);
    member_sum(actions, members);
    findAppend(append_hands, cards.mycards);

    focusHand.push_back(Hand(append_hands[0].action,SINGLE));
    focusHand[0].type_check();
    focusHand.push_back(Hand(append_hands[2].action,PAIR));
    focusHand[1].type_check();


    for (vector<Hand>::iterator i=actions.begin();i!=actions.end();++i) {
        if (i->type == PASS) {}
        else if(
            i->type!=SINGLE &&
            i->type!=PAIR ) {

            if (i->type == THREECARDS) {
                if ((cards.mycards & (FULL_MASK << 4*i->mainCard)) 
                    == FOUR_MASK<<(4*i->mainCard)) continue;
                focusHand.push_back(*i);
                focusHand.push_back((*i)+append_hands[0]);
                focusHand.push_back((*i)+append_hands[2]);
            

            } else if (i->type == BOMB || i->type == JUSTfFOUR) {
                focusHand.push_back(*i);
                focusHand.push_back((*i)+append_hands[1]);
                focusHand.push_back((*i)+append_hands[3]);

            } 
            // else if (i->type == PLANE_2 || i->type == PLANE_SUPER) {
            //     focusHand.push_back(*i);
            //     if (append_hands[1] != PASS)
            //         focusHand.push_back((*i)+append_hands[1]);
            //     if (append_hands[3] != PASS)
            //         focusHand.push_back((*i)+append_hands[3]);
            // } 
            else{
                focusHand.push_back(*i);
            }
        }
    }

    // 若存在一次打完所有手牌的机会，就只设这一个行动方案
    int cards_num_ = cards.cards_sum(cards.mycards);
    for (vector<Hand>::iterator i=actions.begin();i!=actions.end();++i) {
        if (!cards.has(i->action)) *i = ERROR_HAND;
        else {i->type_check();}
        if (i->cardsum() == cards_num_) {
            actions_.clear();
            actions_.push_back(*i);
            return ;
        }
    }
    for (vector<Hand>::iterator i=focusHand.begin();i!=focusHand.end();++i) {
        if (!cards.has(i->action)) *i = ERROR_HAND;
        if (i->cardsum() == cards_num_) {
            actions_.clear();
            actions_.push_back(*i);
            return ;
        }
    }
    if (focusHand.size()==0) actions_= actions;
    else actions_ = focusHand;

    set<Hand>action_set (actions_.begin(),actions_.end());
    actions_.assign(action_set.begin(),action_set.end());
}


// 逻辑枢纽
void State::make_decision(vector<Hand>& actions) {
    if (last_action == PASS) {
        apart_cards(actions);
    } else {
        vector<Hand> main_action;
        anti_action(main_action);

        actions = main_action;
    }
}





User rand_game(Node node ) {
    int actionID;
    User user = node.user;
    Srand();
    // //   debug
    // int times = 0;
    // //   debug
    while (1) {
        vector<Hand> actions;
        if (node.states[user].cards.mycards == EMPTY_CARDS) break;
        node.states[user].rand_action(actions);

        Hand thisaction = actions[rand_int(actions.size())];
        // // debug
        // const char card_name[] = {'9','T','J','Q','K','A','2','w','W'};
        // vector<int>action = Cards::decode(thisaction.action);
        // cout << "user" <<user<<": ";
        // if (thisaction == PASS) cout << "pass ";
        // for (vector<int>::iterator j = action.begin();j!=action.end();j++)
        //     cout << card_name[*j] <<" ";
        // if (user == USER2) cout << endl; 
        // // system("pause");
        // // debug
        node.states[user].cards.mycards -= thisaction.action;
        node.states[user].myaction = thisaction;
        if (thisaction != PASS) node.states[next(user)].last_action = thisaction;
        else if (node.states[last(user)].myaction==PASS)
            node.states[next(user)].last_action = PASS_HAND;
        else node.states[next(user)].last_action = node.states[user].last_action;
        user = next(user);
        // // debug  
        // times++;
        // if (times >= 10000) {cout <<"loop error!"; system("pause");}
        // // debug 
    }
    // // debug
    // cout << endl << "winner: " << user<<endl<<endl;
    // // debug
    return user;
}

int all_cards[30]={0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 8};
const char card_name[] = {'9','T','J','Q','K','A','2','w','W'};
