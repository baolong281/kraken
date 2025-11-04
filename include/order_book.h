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

class Book {
public:
  Book(std::string ticker) : ticker_(ticker), orders_(), bids_(), asks_() {}

  void add(const KrakenOrder &order);
  void modify(const KrakenOrder &order, bool remove);

  const std::string &ticker() const { return ticker_; }
  const LevelVec &getBids() const { return bids_; }
  const LevelVec &getAsks() const { return asks_; }

  void printBook() const {
    std::cout << "Order Book: " << ticker_ << "\n";

    std::cout << "\nBIDS (price descending):\n";
    for (const auto &lvl : bids_) {
      std::cout << "Price: " << std::fixed << std::setprecision(2)
                << lvl->price_ << " | Qty: " << lvl->qty_ << " | Volume: $"
                << ((lvl->qty_) * (lvl->price_)) << "\n ";
    }

    std::cout << "\nASKS (price ascending):\n";
    for (const auto &lvl : asks_) {
      std::cout << "Price: " << std::fixed << std::setprecision(5)
                << lvl->price_ << " | Qty: " << lvl->qty_ << " | Volume: $"
                << ((lvl->qty_) * (lvl->price_)) << "\n ";
    }

    std::cout << "----------------------\n";
  }

private:
  std::string ticker_;
  std::unordered_map<std::string, OrderPtr> orders_;
  LevelVec bids_;
  LevelVec asks_;
};

using BookPtr = std::unique_ptr<Book>;

class OrderBook {
public:
  OrderBook() {}
  ~OrderBook() {}

  void add(const KrakenOrder &order);
  void modify(const KrakenOrder &order, bool remove);

  void printBooks() const {
    for (auto &book : books_) {
      book.second->printBook();
    }
  }

  std::string serialize() const;

private:
  std::unordered_map<std::string, BookPtr> books_;
};
