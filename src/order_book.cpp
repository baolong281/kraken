#include "order_book.h"
#include "feed.h"
#include <algorithm>
#include <fmt/core.h>
#include <glaze/glaze.hpp>
#include <memory>

void OrderBook::add(const KrakenOrder &order) {
  if (books_.contains(order.symbol_)) {
    books_[order.symbol_]->add(order);
  } else {
    books_[order.symbol_] = std::make_unique<Book>(order.symbol_);
    books_[order.symbol_]->add(order);
  }
}

void OrderBook::modify(const KrakenOrder &order, bool remove) {
  if (books_.contains(order.symbol_)) {
    books_[order.symbol_]->modify(order, remove);
  } else {
    std::cerr << "Book not found: " << order.symbol_ << "\n";
  }
}

struct BookStruct {
  std::string symbol{};
  std::vector<Level> bid{};
  std::vector<Level> asks{};
};

struct OrderBookStruct {
  std::vector<BookStruct> books{};
};

std::string OrderBook::serialize() const {
  OrderBookStruct ob;

  for (auto &[symbol, book] : books_) {
    BookStruct bs{};
    bs.symbol = symbol;

    std::vector<Level> bid{};

    for (auto &lvl : book->getBids()) {
      bid.push_back(*lvl);
    }

    std::vector<Level> ask{};
    for (auto &lvl : book->getAsks()) {
      ask.push_back(*lvl);
    }

    bs.bid = bid;
    bs.asks = ask;

    ob.books.push_back(bs);
  }

  return glz::write_json(ob).value_or("error");
}

void Book::add(const KrakenOrder &order) {
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

  if (levels.size() > 10)
    levels.pop_back();

  orders_[order.id_] = std::make_unique<Order>(order.qty_, lvlPtr);
}

void Book::modify(const KrakenOrder &order, bool remove) {
  if (orders_.contains(order.id_)) {
    OrderPtr &order_ptr = orders_[order.id_];
    Level &level = *(order_ptr->level_);
    level.qty_ -= order.qty_;

    if (remove)
      orders_.erase(order.id_);

    constexpr double EPSILON = 1e-9;
    if (level.qty_ <= EPSILON) {
      LevelVec &levels = (order.side_ == OrderSide::bid) ? bids_ : asks_;

      // find the matching level pointer
      auto lit =
          std::find_if(levels.begin(), levels.end(), [&](const LevelPtr &lvl) {
            return lvl.get() == &level;
          });

      if (lit != levels.end())
        levels.erase(lit);
    }

  } else {
    std::cerr << "Order ID not found: " << order.id_ << "\n";
  }
}
