#pragma once
#include <cstdint>
#include <cstring>
#include <type_traits>

#pragma pack(push, 1)

struct MarketData
{
    char instrument_id[16];
    double last_price;  //最新价
    int32_t volume; // 成交量
    double bid_price1;  //买一价
    int32_t bid_volume1;    // 买一量
    double ask_price1;  //卖一价
    int32_t ask_volume1;    //卖一量；
    int64_t timestamp;  //交易所时间戳（纳秒）
    int64_t local_timestamp;    //本地时间戳
};

struct Order
{
    int64_t order_ref;  //本地报单引用
    char instrument_id[16];
    double limit_price;
    int32_t volume;
    char direction; //'B'uy or 'S'ell
    char offset_flag;   //'O'pen or 'C'lose
};

enum class MsgType : uint8_t
{
    CMD_MD_SUBSCRIBE,   //订阅行情
    CMD_ORDER_INSERT,   //报单
    MSG_MD_RIN, //行情回报
    MSG_ORDER_RTN   //报单回报
};

struct FrameHeader
{
    MsgType msg_type;
    uint32_t lenght;
    int64_t nano_time;  //写入队列时间
};

#pragma pack(pop)

static_assert(std::is_trivially_copyable<MarketData>::value, "MarketData must be POD");
static_assert(std::is_trivially_copyable<Order>::value, "Order must be POD");