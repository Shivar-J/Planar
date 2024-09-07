#include <iostream>
#include <fstream>
#include <string>
#include "curl/curl.h"
#include "json.hpp"
#include <Windows.h>
#include <cstdio>
#include <filesystem>
#include "minizip/unzip.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

class Updater {
public:
    Updater(const std::string& owner, const std::string& repo)
        : repo_owner(owner), repo_name(repo) {}

    bool check_for_updates() {
        std::string curr_version;
        if (read_version_info(curr_version)) {
            auto [latest_version, download_url] = get_latest();

            if (latest_version.empty() || download_url.empty()) {
                return false;
            }

            if (curr_version != latest_version) {
                return true;
            }
            else {
                return false;
            }
        }
        else
            return false;
    }

private:
    std::string repo_owner, repo_name;

    static size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* s) {
        size_t total_size = size * nmemb;
        s->append((char*)contents, total_size);
        return total_size;
    }

    bool read_version_info(std::string& version, const std::string& filename = "version_info.txt") {
        std::ifstream file(filename);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                size_t pos = line.find("=");
                if (pos != std::string::npos) {
                    std::string key = line.substr(0, pos);
                    std::string value = line.substr(pos + 1);

                    if (key == "version")
                        version = value;
                }
            }
            file.close();
            return true;
        }
        return false;
    }

    std::pair<std::string, std::string> get_latest() {
        CURL* curl;
        std::string read_buf;

        curl = curl_easy_init();
        if (curl) {
            std::string url = "https://api.github.com/repos/" + repo_owner + "/" + repo_name + "/releases/latest";
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buf);

            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, "User-Agent: autoupdate");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);

            if (res != CURLE_OK)
                return { "", "" };

            auto res_json = json::parse(read_buf, nullptr, false);
            if (!res_json.is_discarded() && res_json.contains("tag_name") && res_json["tag_name"].is_string()) {
                return { res_json["tag_name"], res_json["assets"][0]["browser_download_url"] };
            }
        }
        return { "", "" };
    }
};