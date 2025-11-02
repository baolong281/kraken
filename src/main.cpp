#include "feed.h"
#include <Poco/Exception.h>
#include <iostream>
#include <string>
#include <array>
#include <memory>
#include <stdexcept>
#include <cstdlib>

// Execute command and capture output
std::string exec(const std::string& cmd) {
    std::array<char, 128> buffer;
    std::string result;
    
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
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

        std::string token = exec("python3 get_kraken_token.py");
        
        if (!token.empty() && token.back() == '\n') {
            token.pop_back();
        }
        
        return token;
    } catch (const std::exception& e) {
        std::cerr << "Error executing Python script: " << e.what() << std::endl;
        return "";
    }
}

int main() {
    try {

        FeedConfig config {};
        config.host = "ws-auth.kraken.com";
        config.path = "/v2";
        config.port = 443;
        config.token = getToken();

        Feed dataFeed {config} ;
        dataFeed.start();

        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    catch (Poco::Exception& exc) {
        std::cerr << "WebSocket error: " << exc.what() << std::endl;
    } catch (const std::exception& e) {
        // Log error or notify callback
        std::cerr << "Feed thread exception: " << e.what() << "\n";
    }

    return 0;
}
