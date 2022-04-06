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

// 第二种是编码方式，用一个unsighed long long int 储存一手牌
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
// 将会被废弃 ！！！
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
    NONE = 0,
    WITHSINGLE = 1,
    WITHTWOSINGLE = 2,
    WITHPAIR = 3,
    WITHTWOPAIR = 4
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

// uint64_t myHash[3] = {16668552215174154829ull, 15684088468973760346ull, 14458935525009338918ull };
uint64_t myHash[64][3] = {16668552215174154829ull, 15684088468973760346ull, 14458935525009338918ull, 17069087732856008244ull, 4665249168328654237ull, 2506651028494935006ull, 4142044020440757338ull, 1838224231312793316ull, 407446862418391519ull, 12651567891252036807ull, 12065738086055235367ull, 17863737910281200644ull, 14818848823590580721ull, 2449328130808507269ull, 3703610699633700225ull, 1247449263731090769ull, 18415012460386301039ull, 10440343481692447988ull, 6575942454424886470ull, 7079388388391540156ull, 1056903953815343530ull, 1302778975914571106ull, 8568378854837631303ull, 8061970819115803990ull, 14325212211899040538ull, 6039537587684889327ull, 8634611165135681786ull, 2373693396233091769ull, 6762593013204091542ull, 3239491292076930509ull, 7851954291664117057ull, 6603879558530891700ull, 18097968031487201671ull, 6237602200615785004ull, 6938166570731171152ull, 10839878235714550601ull, 12558588575245590780ull, 9786719252293315549ull, 14478642443199040409ull, 18318865194440743847ull, 2188075877604972879ull, 13853404042204719420ull, 15790617741847716028ull, 13169103606201132884ull, 7237405842780369298ull, 7319255577297890057ull, 14596021816324768057ull, 3118979015609147186ull, 14964815200706063351ull, 396921024258579884ull, 2784098543079198318ull, 13296736921928741747ull, 1534159419210227095ull, 7210014343715497891ull, 5991736446831530586ull, 930875805330555743ull, 9553559469292047716ull, 3148300683452202202ull, 9343209496481317499ull, 15107591555134261881ull, 12181804936447203596ull, 13210087611768757141ull, 6719066631683055734ull, 3325834051997066068ull, 6391080986949358585ull, 3730782868825969264ull, 17610115084337192332ull, 2147649610319799244ull, 543740767602230636ull, 15812277687345229439ull, 7307517328862940893ull, 14523330419253597995ull, 10034960855476612066ull, 2604432104628846312ull, 13232655076468385361ull, 2989775395494227098ull, 6841544787939108138ull, 17306053772534944427ull, 4839212232905313649ull, 17649931098039478771ull, 4755815132978931169ull, 10990491964609973746ull, 10257606170776784773ull, 12450307457559505408ull, 7462781503201666376ull, 6254484232527537735ull, 17802935049721257336ull, 5765981541309584671ull, 13819469070950802349ull, 12964283009726431281ull, 6364857706746111599ull, 9122691174139123072ull, 11826040702937168613ull, 5579891280180689308ull, 978139813813459485ull, 3414956089563965541ull, 14252933494671180514ull, 8987413421681391908ull, 6937048715218753305ull, 9470313117237644623ull, 4117480305406080765ull, 10387439639194965371ull, 1613004714374024769ull, 11572702902014344759ull, 6925818282466670550ull, 11350684147439743729ull, 13582731901001220562ull, 7096229272808839574ull, 11656602685177029023ull, 12035177389252383273ull, 9601544388911144461ull, 10168261857442669966ull, 1826722813000732362ull, 18126645333604486346ull, 4291537724591403329ull, 17406284394822311124ull, 14982781505061244384ull, 2531783010219297890ull, 8827857058156438233ull, 134902996294667272ull, 6401569462100436384ull, 2177462498408934342ull, 9633905724068725921ull, 8333086144174031401ull, 3225494044143205884ull, 9799683909359747457ull, 5156793947141607110ull, 5765849991854444689ull, 15223857081901539984ull, 12410231686593393582ull, 15590102858949883800ull, 17174959797926922664ull, 787058988844657537ull, 2685506887356308045ull, 14676255597535192704ull, 6647175297046901846ull, 6919052294627046984ull, 13793245618590807689ull, 1145879464798453614ull, 11804454393526935237ull, 1495015275024927761ull, 10764473511429947328ull, 13206476334160523820ull, 13162668047424105333ull, 14535206418010801661ull, 15654880054427191241ull, 10141424475846849666ull, 11140920606112196563ull, 11431128457954766101ull, 7867330980549331256ull, 1922837694387100693ull, 11416179472953463490ull, 12401513342097832696ull, 13218371223108052012ull, 4000949356437243011ull, 7884158610990408253ull, 10707286997474214536ull, 7756363583350225618ull, 3310450739409847266ull, 3345053689537336103ull, 2309171409701299267ull, 13135666138001156036ull, 12463842187737695209ull, 16464897576421676144ull, 7995630649052900248ull, 4454930779258814632ull, 10610981062443131921ull, 6920552190775002977ull, 578760989450696976ull, 15925583449999100184ull, 95889443336856401ull, 17037024464907497596ull, 3860267974905203268ull, 7516036215066551880ull, 4277058413668603281ull, 9786670671884617487ull, 3437603049371422301ull, 13895326348634672006ull, 7724158672266640722ull, 6734435917875600107ull, 4591755121552383383ull, 7039133793920971384ull, 18430580342807362499ull, 16192480689637956038ull, 12598719717892771667ull, 1665625432242099795ull, 17091277853226821782ull, 7927492083339397439ull, 13107747356566013913ull, 3821972150321199266ull, 9537590639044331541ull, 17436595555697649537ull};

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
};

const Hand PASS_HAND(EMPTY_CARDS,PASS);

// 某个玩家的某个时间的状态
struct State {
    Cards cards;
    User user;
    Hand last_action;
    Hand myaction;

    State(Cards card0, User user_):
        cards(card0), user(user_) {}
    State(User user_): user(user_) {}

    void anti_action(vector<Hand>& );
    void apart_cards(vector<Hand>& );
    void free_action(vector<Hand>& );
    void rand_action(vector<Hand>& );
    void member_sum(vector<Hand>& , set<Handtype> * );
    void make_decision(vector<Hand>&);

    void find_rocket(vector<Hand>& vh) {
        if (cards.mycards & Rocket == ROCKET) 
            vh.push_back(Hand(Rocket,ROCKET,JOKER));
    }
    void find_bomb(vector<Hand>& vh, Cardtype minline = FAKECARD) {
        for (Cardtype i = Cardtype(minline+1); i<=TWO; i=Cardtype(i+1)) {
            if ((cards.mycards & Bombs[i]) == Bombs[i]) 
                vh.push_back(Hand(Bombs[i],BOMB,i));
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

bool has(cards mcd,cards cd) {
    for (int i=0;i<CARD_TYPES;++i) {
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
            if (action & Rocket == Rocket) {
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
            for (int i=0;i<Bombs_num;++i) {
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
            for (int i=0;i<Bombs_num;++i) {
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
        for (int i=0;i<Bombs_num;++i) {
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


// last_action在传入时就应当已经被check过type和maincard了
// 在这个函数里面仅仅找出所有可以反制对手的牌
// 副牌在此不考虑，在拆牌后再加上
void State::anti_action(vector<Hand>& actions) {
    find_rocket(actions);
    actions.push_back(PASS_HAND);
    if (last_action.type == ROCKET) return ;
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
void State::free_action(vector<Hand>& actions) {
    int card_num = Cards::cards_sum(cards.mycards);
    find_rocket(actions);
    find_bomb(actions);
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

}

void State::rand_action(vector<Hand>& actions) {
    if (last_action == PASS) {
        free_action(actions);
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
    // set<Handtype> all_action_types;
    vector<Hand> append_single;
    vector<Hand> append_pair;
    free_action(actions);
    member_sum(actions, members);
    


    for (Cardtype i = NINE; i<=JOKER; i=Cardtype(i+1)) {
        if (members[i].size()==1) {
            Cards appendact = (1ull << i*4);
            append_single.push_back(Hand(appendact.mycards,WITHSINGLE));
            focusHand.push_back(Hand(appendact.mycards,SINGLE,i));
        } else if (members[i].size()==2) {
            if (members[i].find(PAIR)!=members[i].end()) {
                Cards appendact = (2ull << i*4);
                append_pair.push_back(Hand(appendact.mycards,WITHPAIR));
                focusHand.push_back(Hand(appendact.mycards,PAIR,i));
            }
        }
    }

    for (vector<Hand>::iterator i=actions.begin();i!=actions.end();++i) {
        if (
            i->type!=PASS &&
            i->type!=SINGLE &&
            i->type!=PAIR ) {
            if (i->type == THREECARDS) {
                if ((cards.mycards & (FULL_MASK << 4*i->mainCard)) 
                    == FOUR_MASK<<(4*i->mainCard)) continue;
                focusHand.push_back(*i);
                for (vector<Hand>::iterator j=append_single.begin();j!=append_single.end();++j) 
                        focusHand.push_back((*i)+(*j));
                for (vector<Hand>::iterator j=append_pair.begin();j!=append_pair.end();++j) 
                        focusHand.push_back((*i)+(*j));                
            } else if (i->type == BOMB || i->type == JUSTfFOUR) {
                focusHand.push_back(*i);
                int pair_num =append_pair.size();
                if (pair_num==1){
                    focusHand.push_back((*i)+append_pair[0]);
                } else {
                    for (int i1=0;i1<pair_num-1;++i1) {
                        for (int i2=i1+1;i2<pair_num;i2++) {
                            focusHand.push_back(((*i)+append_pair[i1])+append_pair[i2]);
                        }
                    }
                }
            } else {
                focusHand.push_back(*i);
            }
        }
    }

    int cards_num_ = cards.cards_sum(cards.mycards);
    for (vector<Hand>::iterator i=actions.begin();i!=actions.end();++i) {
        if (i->cardsum() == cards_num_) {
            actions_.clear();
            actions_.push_back(*i);
            return ;
        }
    }
    for (vector<Hand>::iterator i=focusHand.begin();i!=focusHand.end();++i) {
        if (i->cardsum() == cards_num_) {
            actions_.clear();
            actions_.push_back(*i);
            return ;
        }
    }
    if (focusHand.size()==0) actions_= actions;
    else actions_ = focusHand;
}


// 逻辑枢纽，牌型反制时的副牌的查找
void State::make_decision(vector<Hand>& actions) {
    if (last_action == PASS) {
        apart_cards(actions);
    } else {
        vector<Hand> main_action;
        anti_action(main_action);
        // if (last_action.append_type == WITHSINGLE) {
            
        // } else if (last_action.append_type == WITHPAIR) {

        // } else if (last_action.append_type == WITHTWOSINGLE) {

        // } else if (last_action.append_type == WITHTWOPAIR) {

        // }
        // if (last_action.type == THREECARDS) {
        //     if (last_action.append_type == WITHSINGLE) {
        //         for (vector<Hand>::iterator i=actions.begin();i!=actions.end();++i) {
        //             // if (i->type == THREECARDS)
        //         }                
        //     }
        // }
        actions = main_action;
    }
}





User rand_game(Node node ) {
    int actionID;
    User user = node.user;
    Srand();
    while (1) {
        vector<Hand> actions;
        if (node.states[user].cards.mycards == EMPTY_CARDS) break;
        node.states[user].rand_action(actions);
        Hand thisaction = actions[rand_int(actions.size())];
        node.states[user].cards.mycards -= thisaction.action;
        node.states[user].myaction = thisaction;
        if (thisaction != PASS) node.states[next(user)].last_action = thisaction;
        else if (node.states[last(user)].myaction==PASS)
            node.states[next(user)].last_action = PASS_HAND;
        else node.states[next(user)].last_action = node.states[user].last_action;
        user = next(user);
    }
    return user;
}

int all_cards[30]={0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 8};
const char card_name[] = {'9','T','J','Q','K','A','2','w','W'};
