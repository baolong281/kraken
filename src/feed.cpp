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

struct SubscribeParams {
  std::string channel{"level3"};
  std::vector<std::string> symbol{"BTC/USD"};
  bool snapshot{true};
  std::string token; // to be filled dynamically

  template <typename T> static auto reflect(T &self) {
    return std::tie(self.channel, self.symbol, self.snapshot, self.token);
  }
};

struct SubscribeMessage {
  std::string method{"subscribe"};
  SubscribeParams params;

  template <typename T> static auto reflect(T &self) {
    return std::tie(self.method, self.params);
  }
};

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

    SubscribeMessage subMessage{};
    subMessage.params.token = config.token;
    std::string msg = glz::write_json(subMessage).value_or("error");

    socket_->sendFrame(msg.data(), msg.size(),
                       Poco::Net::WebSocket::FRAME_TEXT);
    Logger::instance().info("Sending subscribe request: " + msg);

    // getting intial snapshot message
    // we may get other updates first which we put into a backlog to apply after
    // TODO: apply this backlog
    char buffer[8192];
    int flags;

    bool snapshot_recieved = false;
    int n;

    std::vector<KrakenOrder> order_backlog;

    while (!snapshot_recieved) {
      n = socket_->receiveFrame(buffer, sizeof(buffer), flags);
      auto response_type = getResponseType(buffer, n);

      if (response_type == "snapshot") {
        snapshot_recieved = true;
      } else if (response_type == "update") {
        auto orders = parseKrakenLevel3Buffer(buffer, n);
        order_backlog.insert(order_backlog.end(), orders.begin(), orders.end());
      }
    }

    for (auto &o : order_backlog) {
      std::cout << o.toString() << std::endl;
    }

    auto orders = parseKrakenLevel3Buffer(buffer, n);
    snapshot_callback_(orders);
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
