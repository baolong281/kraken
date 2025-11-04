#include "consumer.h"
#include "feed.h"
#include "logger.h"
#include "order_book.h"
#include <Poco/Exception.h>
#include <array>
#include <boost/lockfree/spsc_queue.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

std::string fmt_double(double val, int precision = 2) {
  std::ostringstream ss;
  ss << std::fixed << std::setprecision(precision) << val;
  return ss.str();
}

// Execute command and capture output
std::string exec(const std::string &cmd) {
  std::array<char, 128> buffer;
  std::string result;

  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"),
                                                pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }

  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }

  return result;
}

std::string getToken() {
  try {
    Logger::instance().info("Retrieving token for data feed");

    std::string token = exec("python3 get_kraken_token.py");

    if (!token.empty() && token.back() == '\n') {
      token.pop_back();
    }

    Logger::instance().info("Token for data feed retrieved.");

    return token;
  } catch (const std::exception &e) {
    std::cerr << "Error executing Python script: " << e.what() << std::endl;
    return "";
  }
}

int main() {
  try {

    std::string token = getToken();

    SubscribeParams params{
        "level3", {"BTC/USD", "ETH/USD", "XRP/USD"}, true, token};

    FeedConfig config{"ws-auth.kraken.com", "/v2", 443, params};

    OrderBook book{};

    Consumer<KrakenOrder> consumer{
        1024, [&book](KrakenOrder order) {
          if (order.type_ == OrderType::add) {
            book.add(order);
          } else {
            book.modify(order, order.type_ == OrderType::remove ? true : false);
          }
        }};

    FeedCallback feedCb = [&consumer](const KrakenOrder &order) {
      consumer.eat(order);
    };

    SnapshotCallback snapshotCb =
        [&book](const std::vector<KrakenOrder> &initialOrders) {
          Logger::instance().info("Updating initial book state with orders: ");
          for (auto &o : initialOrders) {
            book.add(o);
          }
          Logger::instance().info("Book initial snapshot constructed.");
        };

    Feed dataFeed{config};
    dataFeed.start(feedCb, snapshotCb);

    book.printBooks();

    while (true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

  } catch (Poco::Exception &exc) {
    std::cerr << "WebSocket error: " << exc.what() << std::endl;
  } catch (const std::exception &e) {
    // Log error or notify callback
    std::cerr << "Feed thread exception: " << e.what() << "\n";
  }

  return 0;
}
