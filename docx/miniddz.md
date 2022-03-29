###  *Minidoudizhu*迷你斗地主

#### 逻辑过程

```cpp
void user0(参数 A, 参数 B, ...) {
    //地主的逻辑
}
void user1(参数 A, 参数 B, ...) {
    //农民1
}
void user2(参数 A, 参数 B, ...) {
    //农民2
}


void game() {
	//不需要进行角色的分配，直接调用角色的函数即可。
    for (int pos=0;i<3;i++) {
        json_sender();
        json_analyze();
        user0(...);/user1(...);/user2(...);
        json_reveive();
        game_move();
        game_over_check();
    }
    game_end();
    
}

int main() {
    game();
}
```

#### user0 （地主）

```cpp
void user0(...) {
    V1: 我们现在有的牌
    V2: 历史出牌
    V3: 我们的角色
    V4: 猜测的user1的牌
    V5: 猜测的user2的牌
    V6: 计划
    funcition1: guess(V1, V2, V3) -> V4,V5 根据历史猜测出剩下方的剩下的牌的组成
    mcts(V1, V3, V4, V5) -> V6
    return V6; 
}
V6 mcts(V1, V4, V5) {
	Node used = nodefind();
	fina = node_check(...);
	data_roll_back(fina);
		
}
int node_check(V1, V4, V5) {
	V4 -> V4_1, V4_2 可能性最高的的两种情况
	V5 -> V5_1, V5_2 同上
	int wintimes=0;
	for (i=0; i<GameNum; i++) {
		wintimes += rand(V1, V4_1, V5_2);
		wintimes += rand(V1, V4_2, V5_1);
			or 
		wintimes += rand(V1, V4_1, V5_1);
		wintimes += rand(V1, V4_2, V5_2);		
	}
}
int rand(V1, V4_n, V5_n) {
	<...>
	return winner_is_me;
}
```



















