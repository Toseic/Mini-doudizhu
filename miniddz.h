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
#include "jsoncpp/json.h"

using namespace std;
// 编码方式
//  9  10 J  Q  K  A  2  Joker JOKER
//  0  1  2  3  4  5  6    7    8
// 按照每四位编码一张牌
//  0x000000004  -> 9*4
//  0x110000000  -> 王炸
// 
// 合法性检查范围：
//  仅仅在拆牌和压上家的牌时才会使用，
// 若添加人机交互功能，则仅仅限于合法调试
// 
// 

typedef unsigned long long ull;
typedef unsigned long long Hash;
typedef unsigned long long cards;

typedef enum {
    NINE = 0, TEN = 1,
    JACK = 2, QUEEN = 3,
    KING = 4, ACE = 5,
    TWO = 6, Joker = 7, 
    JOKER = 8, UNKNOWNCARD = -1,
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
    FOURCARDS = 15,
    BOMB = 16,
    JUSTfFOUR = 17,
    ROCKET = 18,

} Handtype;


typedef enum {
    NONE = 0,
    WITHSINGLE = 1,
    WITHTWOSINGLE = 2,
    WITHPAIR = 3,
    WITHTWOPAIR = 4
} Appendtype;

typedef enum {
    USER0 = 0,
    USER1 = 1,
    USER2 = 2,
    LORD = 0,

} User;

typedef enum {
    BLIND,
    FULL,
} TransType;

User next(User user) {if (user==USER2) return USER0; return User(user+1);}
User last(User user) {if (user==USER0) return USER2; return User(user-1);}

const cards EMPTY_CARDS = 0ull;
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

uint64_t myHash[3] = {16668552215174154829ull, 15684088468973760346ull, 14458935525009338918ull };

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

const int Bombs_num = 7;
const cards Bombs[7] = {
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

struct Cards {
    cards mycards;
    Cards(): mycards(EMPTY_CARDS) {}
    Cards(cards cd): mycards(cd) {}
    Cards(vector<int>& cards_vector):
        mycards(encode(cards_vector)) {}
    
    bool has(cards cd) {
        for (int i=0;i<CARD_TYPES;i++) {
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
            for (int j=0;j<cardnum;j++) 
                decoded_cards.push_back(i);
        }
        return decoded_cards;
    }

    static vector<int> decode(cards cd) {
        vector<int> decoded_cards;
        for (Cardtype i=NINE;i<=JOKER;i=Cardtype(i+1)) {
            int cardnum = (int)(FULL_MASK & (cd>>i*4));
            for (int j=0;j<cardnum;j++) 
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
            if (cardnum) type_sum++;
        }
        return type_sum;   
    }

    static cards encode(vector<int>& cards_vector) {
        cards cd = EMPTY_CARDS;
        for (vector<int>::iterator i = cards_vector.begin();
            i != cards_vector.end(); i++) {
            add(cd, Cardtype(*i), 1);
        }
        return cd;
    }
    
    // void operator-=(Hand hd) {
    //     mycards -= hd.action;
    // }


};

struct Hand {
    cards action;
    Handtype type;
    Appendtype append_type;
    Cardtype mainCard;
    Hand(cards act, Handtype mytype):
        action(act),type(mytype),append_type(NONE),mainCard(UNKNOWNCARD) {} 
    Hand():action(EMPTY_CARDS),type(UNKNOWN),mainCard(UNKNOWNCARD) {}
    Hand(cards action_): action(action_),type(UNKNOWN),mainCard(UNKNOWNCARD) {}
    Hand(vector<int>& card_vector): action(Cards::encode(card_vector)),
        mainCard(UNKNOWNCARD),type(UNKNOWN) {}

    void type_check();
    bool operator==(Handtype hd) { return (type==hd); }
    bool operator!=(Handtype hd) { return (type!=hd); }
};

const Hand PASS_HAND(EMPTY_CARDS,PASS);

struct State {
    Cards cards;
    User user;
    Hand last_action;
    Hand myaction;

    State(Cards card0, User user_):
        cards(card0), user(user_) {}
    State(User user_): user(user_) {}

};

struct Node {
    State states[3];
    User user;
    Hash hash;
    Hash parent,children[50];
    int childNum;
    Node():states{USER0, USER1, USER2} {}
    Node(User user_):states{USER0, USER1, USER2},user(user_) {}
    // Node(State state0, State state1, State state2, User user_):
    //     states{state0, state1, state2},user(user_) {}
    Node(State state0, State state1, State state2, User user_, TransType type):
        states{state0, state1, state2},user(user_) {
            if (type==BLIND) {
                states[next(user)].cards.mycards = EMPTY_CARDS;                
                states[last(user)].cards.mycards = EMPTY_CARDS;                
            }
        }
    void nodehash() {
        hash = myHash[user] ^ states[0].cards.mycards ^ 
            states[1].cards.mycards ^ states[2].cards.mycards ; 
    }
    double eval();
    void newchild(Hand hd);

};

bool has(cards mcd,cards cd) {
    for (int i=0;i<CARD_TYPES;i++) {
        if ((mcd<<(i*4)) < (cd<<(i*4))) return false;
    }
    return true;
}  

bool is(cards mcd, cards cd) {
    return (mcd == cd);
}

void remove(cards mcd, Cardtype type, int num) {
    mcd -= ((ull)num << type*4);
}

void remove(cards mcd, cards cd) {
    mcd -= cd;
}

inline Cardtype find_1(cards cd) {
    Cardtype ans = UNKNOWNCARD;
    for (Cardtype i = NINE; i<=JOKER; i=Cardtype(i+1)) {
        if (((cd >> i*4) & ONE_MASK) && !((cd >> i*4) & EXP_ONE_MASK))
            ans = i; 
    }
    return ans;
}

inline Cardtype find_2(cards cd) {
    Cardtype ans = UNKNOWNCARD;
    for (Cardtype i = NINE; i<=JOKER; i=Cardtype(i+1)) {
        if (((cd >> i*4) & TWO_MASK) && !((cd >> i*4) & EXP_TWO_MASK)) 
            ans = i;
    }
    return ans;
}

inline Cardtype find_3(cards cd) {
    Cardtype ans = UNKNOWNCARD;
    for (Cardtype i = NINE; i<=JOKER; i=Cardtype(i+1)) {
        if (((cd >> i*4) & THREE_MASK) && !((cd >> i*4) & EXP_THREE_MASK)) 
            ans = i;
    }
    return ans;
}

inline Cardtype find_4(cards cd) {
    Cardtype ans = UNKNOWNCARD;
    for (Cardtype i = NINE; i<=JOKER; i=Cardtype(i+1)) {
        if (((cd >> i*4) & FOUR_MASK) && !((cd >> i*4) & EXP_FOUR_MASK)) 
            ans = i;
    }
    return ans;
}

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
        if (card_type_num == 2) {
            type = ROCKET;
            append_type = NONE;
            mainCard = JOKER;
        } else {
            type = PAIR;
            append_type = NONE;
            mainCard = find_2(action);
        }
        break;
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
            for (int i=0;i<Bombs_num;i++) {
                if ((action & Bombs[i]) == Bombs[i]) {
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
            for (int i=0;i<Bombs_num;i++) {
                if ((action & Bombs[i]) == Bombs[i]) {
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
        for (int i=0;i<Bombs_num;i++) {
            if ((action & Bombs[i]) == Bombs[i]) {
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
            for (int i=0;i<Plane_2_num;i++) {
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
    case 10:
    // 航天飞机带小翼 || 五连对 || 飞机带大翼
        if (card_type_num == 4) {
            for (int i=0;i<Plane_super_num;i++) {
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

// void allActionCheck(cards cd, vector<cards>& all_actions) {
// 
// }



// vector<vector<cards> > apartCards(cards cd,vector<Hand>& actions) {
//     cards mcd = cd;
//     vector<cards> actions;
//     vector<Hand> all_actions;
//     for (int i=0;i>Bombs_num;i++) {
//         if (has(cd, Bombs[i])) {
//             all_actions.push_back(Hand(Bombs[i],BOMB));
//         }
//     }

//     for (int i=0;i<Line_2_num;i++) {
//         if (has(cd, Line_2[i])) {
//             all_actions.push_back(Hand(Line_2[i],LINE_2));
//         }
//     }

//     for (int i=0;i<Line_1_num;i++) {
//         if (has(cd, Line_1[i])) {
//             all_actions.push_back(Hand(Line_1[i],LINE_1));
//         }
//     }


// }