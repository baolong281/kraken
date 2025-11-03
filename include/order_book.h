#pragma once

#include "feed.h"
#include <iomanip>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

struct Level {
  Price price_;
  Quantity qty_;
};

using LevelPtr = std::shared_ptr<Level>;

struct Order {
  Quantity qty_;
  LevelPtr level_;
};

using OrderPtr = std::unique_ptr<Order>;

using LevelVec = std::vector<LevelPtr>;

class OrderBook {
public:
  OrderBook(std::string ticker)
      : ticker_(ticker), orders_(), bids_(), asks_() {}

  void add(const KrakenOrder &order);
  void modify(const KrakenOrder &order, bool remove);

  void printBook() const {
    std::cout << "Order Book: " << ticker_ << "\n";

    std::cout << "\nBIDS (price descending):\n";
    for (const auto &lvl : bids_) {
      std::cout << "Price: " << std::fixed << std::setprecision(2)
                << lvl->price_ << " | Qty: " << lvl->qty_
                << " | Volume: " << ((lvl->qty_) * (lvl->price_)) << "\n ";
    }

    std::cout << "\nASKS (price ascending):\n";
    for (const auto &lvl : asks_) {
      std::cout << "Price: " << std::fixed << std::setprecision(5)
                << lvl->price_ << " | Qty: " << lvl->qty_
                << " | Volume: " << ((lvl->qty_) * (lvl->price_)) << "\n ";
    }

    std::cout << "----------------------\n";
  }

private:
  std::string ticker_;
  std::unordered_map<std::string, OrderPtr> orders_;
  LevelVec bids_;
  LevelVec asks_;
};
