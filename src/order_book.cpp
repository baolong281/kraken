#include "order_book.h"
#include "feed.h"
#include <algorithm>
#include <memory>

void OrderBook::add(const KrakenOrder &order) {
  LevelVec &levels = order.side_ == OrderSide::bid ? bids_ : asks_;

  auto it = std::lower_bound(levels.begin(), levels.end(), order.price_,
                             [&](const LevelPtr lvl, Price p) {
                               return order.side_ == OrderSide::bid
                                          ? lvl->price_ > p
                                          : lvl->price_ < p;
                             });

  LevelPtr lvlPtr;

  if (it != levels.end() && (*it)->price_ == order.price_) {
    (*it)->qty_ += order.qty_;
    lvlPtr = *it;
  } else {
    LevelPtr lvl = std::make_shared<Level>(order.price_, order.qty_);
    auto new_it = levels.insert(it, lvl);
    lvlPtr = *new_it;
  }

  orders_[order.id_] = std::make_unique<Order>(order.qty_, lvlPtr);
}

void OrderBook::modify(const KrakenOrder &order, bool remove) {
  if (orders_.contains(order.id_)) {
    OrderPtr &order_ptr = orders_[order.id_];
    Level &level = *(order_ptr->level_);
    level.qty_ -= order.qty_;
    if (remove)
      orders_.erase(order.id_);
  } else {
    std::cerr << "Order ID not found: " << order.id_ << "\n";
  }
}
