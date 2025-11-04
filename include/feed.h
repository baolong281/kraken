#pragma once

#include <Poco/Net/Context.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/InvalidCertificateHandler.h>
#include <Poco/Net/WebSocket.h>
#include <functional>
#include <thread>

enum class OrderSide { bid, ask };
enum class OrderType { add, update, remove };

using Price = double;
using Quantity = double;

struct KrakenOrder {
  std::string id_;
  OrderSide side_;
  OrderType type_;
  Price price_;
  Quantity qty_;
  std::string symbol_;

  std::string toString() const {
    std::ostringstream oss;

    // Convert enums to string
    std::string sideStr = (side_ == OrderSide::bid) ? "BID" : "ASK";
    std::string typeStr;
    switch (type_) {
    case OrderType::add:
      typeStr = "ADD";
      break;
    case OrderType::update:
      typeStr = "UPDATE";
      break;
    case OrderType::remove:
      typeStr = "REMOVE";
      break;
    }

    // Format the output
    oss << "OrderID: " << id_ << ", Side: " << sideStr << ", Type: " << typeStr
        << ", Price: " << price_ << ", Qty: " << qty_ << ", Symbol: " << symbol_
        << "\n";

    return oss.str();
  }
};

struct OrderEvent {
  std::string event; // "add" or "delete"
  std::string order_id;
  Price limit_price;
  Quantity order_qty;
  std::string timestamp;
};

struct Level3Update {
  uint64_t checksum;
  std::string symbol;
  std::string timestamp;
  std::vector<OrderEvent> bids;
  std::vector<OrderEvent> asks;
};

struct KrakenMessage {
  std::string channel;
  std::string type;
  std::vector<Level3Update> data;
};

struct SubscribeParams {
  std::string channel{"level3"};
  std::vector<std::string> symbol{"BTC/USD"};
  bool snapshot{true};
  std::string token{};
};

struct FeedConfig {
  std::string host{};
  std::string path{};
  int port{};
  SubscribeParams params{};
};

struct SubscribeMessage {
  std::string method{"subscribe"};
  SubscribeParams params;
};

using FeedCallback = std::function<void(const KrakenOrder &)>;
using SnapshotCallback = std::function<void(const std::vector<KrakenOrder> &)>;

class Feed {
public:
  Feed(const FeedConfig &cfg)
      : socket_(), worker_(), running_(false), config(cfg) {};
  ~Feed(); // clean up (stop thread if running)

  void start(FeedCallback feed_cb,
             SnapshotCallback snapshot_cb); // starts background thread
  void stop();                              // signals thread to exit and joins
private:
  void run();
  FeedCallback callback_;
  SnapshotCallback snapshot_callback_;
  std::unique_ptr<Poco::Net::WebSocket> socket_;
  std::unique_ptr<Poco::Net::HTTPSClientSession> session_; // keep alive
  Poco::Net::Context::Ptr context_;                        // keep alive
  Poco::SharedPtr<Poco::Net::InvalidCertificateHandler> certHandler_;
  std::thread worker_;
  std::atomic<bool> running_;
  FeedConfig config;
};
