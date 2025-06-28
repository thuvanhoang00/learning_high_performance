#ifndef TYPES_H
#define TYPES_H
#include <cstdint>
#include <limits>
#include "macros.h"

namespace thu{

// OrderId
typedef uint64_t OrderId;
constexpr auto OrderId_INVALID = std::numeric_limits<OrderId>::max();
inline auto orderIdToString(OrderId order_id)->std::string{
    if(UNLIKELY(order_id == OrderId_INVALID)){
        return "INVALID";
    }
    return std::to_string(order_id);
}

// TicketId
typedef uint32_t TicketId;
constexpr auto TicketId_INVALID = std::numeric_limits<TicketId>::max();
inline auto ticketIdToString(TicketId ticket_id)->std::string{
    if(UNLIKELY(ticket_id == TicketId_INVALID)){
        return "INVALID";
    }
    return std::to_string(ticket_id);
}

// ClientId
typedef uint32_t ClientId;
constexpr auto ClientId_INVALID = std::numeric_limits<ClientId>::max();
inline auto clientIdToString(ClientId client_id)->std::string{
    if(UNLIKELY(client_id == ClientId_INVALID)){
        return "INVALID";
    }
    return std::to_string(client_id);
}

// Price
typedef uint64_t Price;
constexpr auto Price_INVALID = std::numeric_limits<Price>::max();
inline auto priceToString(Price price)->std::string{
    if(UNLIKELY(price == Price_INVALID)){
        return "INVALID";
    }
    return std::to_string(price);
}

// Quantity
typedef uint32_t Qty;
constexpr auto Qty_INVALID = std::numeric_limits<Qty>::max();
inline auto qtyToString(Qty qty)->std::string{
    if(UNLIKELY(qty == Qty_INVALID)){
        return "INVALID";
    }
    return std::to_string(qty);
}

// Priority
typedef uint64_t Priority;
constexpr auto Priority_INVALID = std::numeric_limits<Priority>::max();
inline auto priorityToString(Priority priority)->std::string{
    if(UNLIKELY(priority == Priority_INVALID)){
        return "INVALID";
    }
    return std::to_string(priority);
}

enum class Side : int8_t {
    INVALID = 0,
    BUY = 1,
    SELL = -1,
};
inline auto sideToString(Side side)->std::string{
    switch (side)
    {
    case Side::BUY:
        return "BUY";
    case Side::SELL:
        return "SELL";
    case Side::INVALID:
        return "INVALID";
    default:
        return "UNKNOWN";
    }
}


// Limits and Constraints
constexpr size_t LOG_QUEUE_SIZE = 8 * 1024 * 1024;
constexpr size_t ME_MAX_TICKETS = 8;
constexpr size_t ME_MAX_CLIENT_UPDATES = 256 * 1024;
constexpr size_t ME_MAX_MARKET_UPDATES = 256 * 1024;
constexpr size_t ME_MAX_NUM_CLIENTS = 256;
constexpr size_t ME_MAX_ORDER_IDS = 1024 * 1024;
constexpr size_t ME_MAX_PRICE_LEVELS = 256;
}

#endif