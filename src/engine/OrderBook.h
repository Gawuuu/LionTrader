#pragma once

#include <map>
#include <unordered_map>
#include <functional>
#include <iostream>

#include "Types.h"
#include "MemoryPool.h"

// 订单节点：链表节点
struct OrderNode
{
    Order order;
    OrderNode *next; // 指向同价格的下一个订单

    // 构造函数，方便 ObjectPool 调用
    OrderNode(const Order &o) : order(o), next(nullptr) {}
};

// 价格档位：管理同一个价格下的所有订单队列
struct LimitLevel
{
    double price;
    OrderNode *head; // 队头 (最早的订单，先成交)
    OrderNode *tail; // 队尾 (最新的订单)

    LimitLevel(double p) : price(p), head(nullptr), tail(nullptr) {}

    // 加单 (时间优先，加到尾部)
    void append(OrderNode *node)
    {
        if (!head)
        {
            head = tail = node;
        }
        else
        {
            tail->next = node;
            tail = node;
        }
        node->next = nullptr;
    }

    // 弹出一个订单 (成交时使用)
    // 注意：内存回收由外部负责
    OrderNode *pop()
    {
        if (!head)
            return nullptr;
        OrderNode *node = head;
        head = head->next;
        if (!head)
            tail = nullptr;
        return node;
    }

    bool empty() const { return head == nullptr; }
};

class OrderBook
{
public:
    // 回调函数定义：当成交发生时调用
    // 参数：买单ID, 卖单ID, 成交价, 成交量
    using MatchCallback = std::function<void(int64_t, int64_t, double, int32_t)>;

    OrderBook(MatchCallback cb) : on_match_(cb), pool_(1000000) { }

    // 处理新订单
    void addOrder(const Order &order)
    {
        // 1. 尝试撮合
        int32_t leaves_qty = match(order);

        // 2. 如果还有剩余量，挂单
        if (leaves_qty > 0)
        {
            Order new_order = order;
            new_order.volume = leaves_qty;

            // 从内存池分配节点
            OrderNode *node = pool_.allocate(new_order);

            // 加入对应的价格档位
            if (order.direction == 'B')
            {
                insert_to_book(bids_, node);
            }
            else
            {
                insert_to_book(asks_, node);
            }

            // 建立索引，方便撤单
            // order_index_[order.order_ref] = node; // 暂时先不写撤单，专注撮合
        }
    }

private:
    // 撮合核心逻辑：返回剩余未成交数量
    int32_t match(const Order &incoming)
    {
        int32_t leaves_qty = incoming.volume;

        if (incoming.direction == 'B')
        {
            // 买单来了，去卖盘(Asks)找最便宜的
            // Asks 是升序 map (less)，begin() 就是最低价
            while (leaves_qty > 0 && !asks_.empty())
            {
                auto it = asks_.begin();
                LimitLevel &level = it->second;

                // 价格不合适：买价 < 卖一价，无法成交
                if (incoming.limit_price < level.price)
                    break;

                // 开始吃这一层的单子
                match_level(level, incoming, leaves_qty);

                // 如果这一层吃光了，删掉这个价格档位
                if (level.empty())
                {
                    asks_.erase(it);
                }
            }
        }
        else
        {
            // 卖单来了，去买盘(Bids)找最贵的
            // Bids 是降序 map (greater)，begin() 就是最高价
            while (leaves_qty > 0 && !bids_.empty())
            {
                auto it = bids_.begin();
                LimitLevel &level = it->second;

                // 价格不合适：卖价 > 买一价
                if (incoming.limit_price > level.price)
                    break;

                match_level(level, incoming, leaves_qty);

                if (level.empty())
                {
                    bids_.erase(it);
                }
            }
        }
        return leaves_qty;
    }

    // 在某一个价格档位上进行撮合
    void match_level(LimitLevel &level, const Order &incoming, int32_t &leaves_qty)
    {
        while (leaves_qty > 0 && !level.empty())
        {
            OrderNode *book_order = level.head; // 队头订单

            // 计算成交量：取两者较小值
            int32_t trade_qty = std::min(leaves_qty, book_order->order.volume);

            // 触发回调
            on_match_(incoming.order_ref, book_order->order.order_ref, level.price, trade_qty);

            // 更新数量
            leaves_qty -= trade_qty;
            book_order->order.volume -= trade_qty;

            // 如果盘口订单吃完了，回收内存
            if (book_order->order.volume == 0)
            {
                level.pop();                  // 从链表移除
                pool_.deallocate(book_order); // 还给内存池
            }
        }
    }

    // 辅助：插入订单簿
    template <typename MapType>
    void insert_to_book(MapType &book, OrderNode *node)
    {
        double price = node->order.limit_price;
        // 如果这个价格档位不存在，emplace 会自动创建
        // map 的 value 是 LimitLevel，它有构造函数 LimitLevel(price)
        // 所以这里 map[price] 会调用默认构造，或者我们需要稍微改一下写法

        auto it = book.find(price);
        if (it == book.end())
        {
            // 完美转发构造 LimitLevel
            book.emplace(price, LimitLevel(price));
            it = book.find(price);
        }
        it->second.append(node);
    }

private:
    // 买盘：价格从高到低 (greater)
    std::map<double, LimitLevel, std::greater<double>> bids_;

    // 卖盘：价格从低到高 (less)
    std::map<double, LimitLevel, std::less<double>> asks_;

    // 成交回调
    MatchCallback on_match_;

    // 内存池
    ObjectPool<OrderNode> pool_;
};