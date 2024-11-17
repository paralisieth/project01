#ifndef COOKIE_RETRIEVER_COOKIE_HPP
#define COOKIE_RETRIEVER_COOKIE_HPP

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace cookie_retriever {

/**
 * @brief Represents a browser cookie with its properties
 */
struct Cookie {
    std::string domain;
    std::string name;
    std::string path;
    std::string value;
    int64_t expires_utc = 0;
    bool secure = false;
    bool http_only = false;
    bool session = false;
    bool host_only = false;
    std::string same_site;
    std::string store_id;
    std::vector<uint8_t> encrypted_value;

    /**
     * @brief Converts the cookie to JSON format
     * @return JSON object representing the cookie
     */
    nlohmann::json to_json() const;
};

} // namespace cookie_retriever

#endif // COOKIE_RETRIEVER_COOKIE_HPP
