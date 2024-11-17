#include "cookie.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <stdexcept>

namespace cookie_retriever {

nlohmann::json Cookie::to_json() const {
    try {
        nlohmann::json j;
        j["domain"] = domain;
        j["name"] = name;
        j["path"] = path;
        j["value"] = value;
        j["secure"] = secure;
        j["httpOnly"] = http_only;
        j["hostOnly"] = host_only;
        j["session"] = session;
        j["sameSite"] = same_site.empty() ? nullptr : same_site;
        j["storeId"] = store_id.empty() ? nullptr : store_id;
        
        if (expires_utc > 0) {
            // Convert Windows FILETIME to Unix timestamp
            std::time_t expires = expires_utc / 1000000 - 11644473600;
            std::stringstream ss;
            ss << std::put_time(std::gmtime(&expires), "%Y-%m-%d %H:%M:%S UTC");
            j["expires"] = ss.str();
        }
        
        return j;
    } catch (const std::exception& e) {
        std::cerr << "Failed to convert cookie to JSON: " << e.what() << std::endl;
        throw;
    }
}

} // namespace cookie_retriever
