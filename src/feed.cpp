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

Feed::Feed(const FeedConfig &cfg)
    : socket_(), worker_(), running_(false), config(cfg) {};

void Feed::start() {
  try {
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
    Logger::instance().info("Connection sucessfull: " + response.getStatus());

    SubscribeMessage subMessage{};
    subMessage.params.token = config.token;
    std::string msg = glz::write_json(subMessage).value_or("error");

    socket_->sendFrame(msg.data(), msg.size(),
                       Poco::Net::WebSocket::FRAME_TEXT);
    Logger::instance().info("Sending subscribe request: " + msg);

    char buffer[1024];
    int flags;
    int n = socket_->receiveFrame(buffer, sizeof(buffer), flags);
    Logger::instance().info("Successfully subscribed to feed!" +
                            std::string(buffer, n));

    running_.store(true, std::memory_order_release);
    worker_ = std::thread(&Feed::run, this);
  } catch (const std::exception &e) {
    running_.store(false, std::memory_order_release);
    throw;
  }
}

void Feed::run() {
  Logger::instance().info("Listening to feed in background...");
  try {
    char buffer[8192];
    int flags;
    while (running_) {
      int n = socket_->receiveFrame(buffer, sizeof(buffer), flags);
      Logger::instance().info(std::string(buffer, n));
    }
  } catch (const std::exception &e) {
    // Log error or notify callback
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
