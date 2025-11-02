#pragma once

#include <Poco/Net/WebSocket.h>
#include <Poco/Net/Context.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/InvalidCertificateHandler.h>
#include <thread>

struct FeedConfig {
    std::string host;
    std::string path;
    std::string token;
    int port;
};

class Feed {
public:
    Feed(const FeedConfig& cfg);           // initialize websocket (not start thread)
    ~Feed();                            // clean up (stop thread if running)
    
    void start();   // starts background thread
    void stop();                        // signals thread to exit and joins
private:
    void run();                         // internal loop: read frames, assemble messages, call callback
    std::unique_ptr<Poco::Net::WebSocket> socket_;
    std::unique_ptr<Poco::Net::HTTPSClientSession> session_;     // keep alive
    Poco::Net::Context::Ptr context_;                            // keep alive
    Poco::SharedPtr<Poco::Net::InvalidCertificateHandler> certHandler_;
    std::thread worker_;
    std::atomic<bool> running_;
    FeedConfig config;
};
