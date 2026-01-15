#include <iostream>
#include <vector>
#include <cassert>
#include "OrderBook.h"

using namespace std;

// 简单的打印回调
void print_trade(int64_t buy_id, int64_t sell_id, double price, int32_t qty) {
    cout << "[TRADE] Buy #" << buy_id << " + Sell #" << sell_id 
         << " @ " << price << " (Qty: " << qty << ")" << endl;
}

int main() {
    cout << "=== Starting Matching Engine Unit Test ===" << endl;

    // 1. 初始化引擎
    OrderBook book(print_trade);

    // 2. 挂单场景测试 (Limit Orders)
    
    // 场景 A: 卖方挂单 (提供流动性)
    // 卖一: 100.0 (Qty 10)
    // 卖二: 101.0 (Qty 5)
    cout << "\n--- Step 1: Injecting Asks (Sells) ---" << endl;
    Order o1 = {1, "AAPL", 100.0, 10, 'S', 'O'}; // ID=1, Price=100
    book.addOrder(o1);
    cout << "Sell #1 placed: 100.0 * 10" << endl;

    Order o2 = {2, "AAPL", 101.0, 5, 'S', 'O'};  // ID=2, Price=101
    book.addOrder(o2);
    cout << "Sell #2 placed: 101.0 * 5" << endl;

    // 场景 B: 买方吃单 (Taker)
    // 买入 100.5，数量 12
    // 预期结果：
    // 1. 应该先吃掉 Sell #1 (价格 100.0 < 100.5，划算)，成交 10 个。
    // 2. 此时买单还剩 2 个。
    // 3. 再看 Sell #2 (价格 101.0 > 100.5)，太贵了，买不起。
    // 4. 剩余的 2 个买单挂在 100.5 的位置。
    cout << "\n--- Step 2: Injecting Buy (Taker) ---" << endl;
    Order o3 = {3, "AAPL", 100.5, 12, 'B', 'O'};
    cout << "Buy #3 incoming: 100.5 * 12 (Expect match with #1)" << endl;
    book.addOrder(o3);

    // 场景 C: 再次买入，吃掉剩下的
    // 买入 102.0，数量 10
    // 预期结果：
    // 1. 先吃掉 Sell #2 (101.0)，成交 5 个。
    // 2. 剩余 5 个挂在 102.0。
    cout << "\n--- Step 3: Injecting Buy (Sweep) ---" << endl;
    Order o4 = {4, "AAPL", 102.0, 10, 'B', 'O'};
    cout << "Buy #4 incoming: 102.0 * 10 (Expect match with #2)" << endl;
    book.addOrder(o4);

    cout << "\n=== Test Finished ===" << endl;
    return 0;
}