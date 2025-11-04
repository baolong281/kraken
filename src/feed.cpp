#include "feed.h"
#include "logger.h"
#include <Poco/Exception.h>
#include <Poco/Net/AcceptCertificateHandler.h>
#include <Poco/Net/Context.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/InvalidCertificateHandler.h>
#include <Poco/Net/SSLException.h>
#include <Poco/Net/SSLManager.h>
#include <Poco/Net/WebSocket.h>
#include <Poco/SharedPtr.h>
#include <glaze/glaze.hpp>

std::string getResponseType(const char *buffer, size_t n) {

  std::string response_type = "";

  try {
    KrakenMessage msg;
    std::string json_str(buffer, n);
    auto err = glz::read_json(msg, json_str);
    response_type = msg.type;
  } catch (const std::exception &e) {
    // handle parsing errors
    Logger::instance().error("Failed to parse Kraken Level3 JSON: {}");
  }

  return response_type;
}

inline std::vector<KrakenOrder> parseKrakenLevel3Buffer(const char *buffer,
                                                        size_t n) {
  std::vector<KrakenOrder> orders;

  try {
    KrakenMessage msg;
    std::string json_str(buffer, n);
    auto err = glz::read_json(msg, json_str);

    for (const auto &update : msg.data) {
      auto convert = [&](const OrderEvent &event, OrderSide side) {
        KrakenOrder o;
        o.id_ = event.order_id;
        o.side_ = side;
        if (event.event == "add")
          o.type_ = OrderType::add;
        else if (event.event == "delete")
          o.type_ = OrderType::remove;
        else
          o.type_ = OrderType::update;
        o.price_ = event.limit_price;
        o.qty_ = event.order_qty;
        o.symbol_ = update.symbol;
        return o;
      };

      for (const auto &ev : update.bids)
        orders.push_back(convert(ev, OrderSide::bid));
      for (const auto &ev : update.asks)
        orders.push_back(convert(ev, OrderSide::ask));
    }

  } catch (const std::exception &e) {
    // handle parsing errors
    Logger::instance().error("Failed to parse Kraken Level3 JSON: {}");
  }

  return orders;
}

void Feed::start(FeedCallback feed_cb, SnapshotCallback snapshot_cb) {
  try {

    callback_ = feed_cb;
    snapshot_callback_ = snapshot_cb;

    Poco::Net::initializeSSL();
    certHandler_ = new Poco::Net::AcceptCertificateHandler(false);

    Poco::Net::SSLManager::instance().initializeClient(0, certHandler_, 0);
    context_ =
        new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "",
                               Poco::Net::Context::VERIFY_RELAXED, 9, true);

    session_ = std::make_unique<Poco::Net::HTTPSClientSession>(
        config.host, config.port, context_);

    Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET,
                                   this->config.path);
    Poco::Net::HTTPResponse response;

    socket_ =
        std::make_unique<Poco::Net::WebSocket>(*session_, request, response);
    auto placeholder = "Connection sucessfull: ";
    Logger::instance().info(placeholder + response.getStatus());

    SubscribeMessage msg_s{"subscribe", config.params};
    std::string msg = glz::write_json(msg_s).value_or("error");

    socket_->sendFrame(msg.data(), msg.size(),
                       Poco::Net::WebSocket::FRAME_TEXT);
    Logger::instance().info("Sending subscribe request: " + msg);

    // getting intial snapshot message
    // we may get other updates first which we put into a backlog to apply after
    // TODO: apply this backlog
    char buffer[16384];
    int flags;

    int snapshots_remaining = config.params.symbol.size();
    int n;

    std::vector<KrakenOrder> order_backlog;

    while (snapshots_remaining > 0) {
      n = socket_->receiveFrame(buffer, sizeof(buffer), flags);

      auto response_type = getResponseType(buffer, n);

      if (response_type == "snapshot") {
        auto orders = parseKrakenLevel3Buffer(buffer, n);
        snapshot_callback_(orders);
        snapshots_remaining--;
      } else if (response_type == "update") {
        auto orders = parseKrakenLevel3Buffer(buffer, n);
        order_backlog.insert(order_backlog.end(), orders.begin(), orders.end());
      }
    }

    Logger::instance().info("Processing backlog...");
    Logger::instance().info("Backlog size: " +
                            std::to_string(order_backlog.size()));

    for (auto &o : order_backlog)
      callback_(o);

    running_.store(true, std::memory_order_release);

    // start network thread
    worker_ = std::thread(&Feed::run, this);
  } catch (const std::exception &e) {
    running_.store(false, std::memory_order_release);
    throw;
  }
}

// the network thread will read off the wire and parse json messages into orders
void Feed::run() {
  Logger::instance().info("Listening to feed in background...");
  try {
    char buffer[8192];
    int flags;
    while (running_) {
      int n = socket_->receiveFrame(buffer, sizeof(buffer), flags);

      auto orders = parseKrakenLevel3Buffer(buffer, n);
      for (auto &o : orders) {

        callback_(o);
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "Feed thread exception: " << e.what() << "\n";
  } catch (...) {
    std::cerr << "Feed thread unknown exception\n";
  }
};

Feed::~Feed() { stop(); };

void Feed::stop() {

  bool expected = true;
  if (running_.compare_exchange_strong(expected, false)) {
    Logger::instance().info("Stopping feed...");
  }

  if (socket_) {
    try {
      socket_->shutdown();
    } catch (...) {
    }
    try {
      socket_->close();
    } catch (...) {
    }
  }

  if (worker_.joinable()) {
    worker_.join();
  }

  socket_.reset();
  session_.reset();
  context_.reset();
  certHandler_.reset();
};
