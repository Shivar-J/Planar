#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <filesystem>
#include <Windows.h>
#include "minizip/unzip.h"
#include "curl/curl.h"
#include <fstream>
#include "json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

class Updater {
public:
    Updater(const std::string& owner, const std::string& repo, const std::string& exe)
        : repo_owner(owner), repo_name(repo), exe_path(exe) {}

    void check_for_updates() {
        std::string curr_version;
        if (read_version_info(curr_version)) {
            std::cout << "Current version: " << curr_version << "\n";
            auto [latest_version, download_url] = get_latest();

            if (latest_version.empty() || download_url.empty()) {
                std::cout << "Failed to get the latest version or download URL.\n";
                return;
            }

            std::cout << "Latest version: " << latest_version << "\n";
            std::cout << "Download URL: " << download_url << "\n";

            if (curr_version != latest_version) {
                std::cout << "New version available: " << latest_version << "\n";
                download_apply_update(download_url, latest_version);
            }
            else {
                std::cout << "You are already on the latest version.\n";
            }
        }
        else {
            std::cout << "Failed to read current version info.\n";
        }
    }

private:
    std::string repo_owner, repo_name, exe_path;
    const std::string temp_update_path = "update.zip";
    const std::string plan_dir = "Planar";

    void download_apply_update(const std::string& download_url, const std::string& latest_version) {
        std::cout << "Downloading the update...\n";
        if (download_update(download_url)) {
            std::cout << "Download complete.\n";
            std::cout << "Unzipping the update...\n";
            unzip_update();
            std::cout << "Replacing files...\n";
            replace_files(plan_dir, fs::current_path().string());
            write_version_info(latest_version);
            std::cout << "Cleaning up old files...\n";
            cleanup_old_files(plan_dir, fs::current_path().string());
            std::cout << "Restarting the application...\n";
            replace_and_restart(latest_version);
        }
        else {
            std::cout << "Failed to download the update.\n";
        }
    }

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

                    if (key == "version") {
                        version = value;
                        std::cout << "Read version from file: " << version << "\n";
                        file.close();
                        return true;
                    }
                }
            }
            file.close();
        }
        std::cout << "Failed to read version info from file.\n";
        return false;
    }

    void write_version_info(const std::string& version, const std::string& filename = "version_info.txt") {
        std::ofstream file(filename);
        if (file.is_open()) {
            file << "version=" << version << std::endl;
            file.close();
            std::cout << "Version info written to file: " << version << "\n";
        }
        else {
            std::cout << "Failed to write version info to file.\n";
        }
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

            if (res != CURLE_OK) {
                std::cout << "CURL error: " << curl_easy_strerror(res) << "\n";
                return { "", "" };
            }

            std::cout << "Received response from GitHub API.\n";

            auto res_json = json::parse(read_buf, nullptr, false);
            if (!res_json.is_discarded() && res_json.contains("tag_name") && res_json["tag_name"].is_string()) {
                std::string tag_name = res_json["tag_name"];
                std::string download_url = res_json["assets"][0]["browser_download_url"];
                std::cout << "Latest tag: " << tag_name << "\n";
                std::cout << "Download URL: " << download_url << "\n";
                return { tag_name, download_url };
            }
        }
        std::cout << "Failed to parse GitHub API response.\n";
        return { "", "" };
    }

    bool download_update(const std::string& download_url) {
        CURL* curl;
        FILE* fp;
        CURLcode res;

        curl = curl_easy_init();
        if (curl) {
            fp = fopen(temp_update_path.c_str(), "wb");
            if (!fp) {
                std::cout << "Failed to open file for writing: " << temp_update_path << "\n";
                return false;
            }

            curl_easy_setopt(curl, CURLOPT_URL, download_url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

            res = curl_easy_perform(curl);

            if (res != CURLE_OK) {
                std::cout << "CURL error: " << curl_easy_strerror(res) << "\n";
                fclose(fp);
                return false;
            }
            else {
                long http_code = 0;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
                if (http_code != 200) {
                    std::cout << "HTTP error code: " << http_code << "\n";
                    fclose(fp);
                    return false;
                }
            }

            fclose(fp);
            curl_easy_cleanup(curl);
            return true;
        }
        std::cout << "Failed to initialize CURL.\n";
        return false;
    }

    void unzip_update() {
        std::cout << "Opening zip file: " << temp_update_path << "\n";
        unzFile zipfile = unzOpen(temp_update_path.c_str());
        if (zipfile == NULL) {
            std::cout << "Failed to open zip file.\n";
            return;
        }

        if (unzGoToFirstFile(zipfile) != UNZ_OK) {
            std::cout << "Failed to go to first file in zip.\n";
            unzClose(zipfile);
            return;
        }

        char filename_inzip[256];
        unz_file_info file_info;

        do {
            if (unzGetCurrentFileInfo(zipfile, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0) != UNZ_OK) {
                std::cout << "Failed to get file info from zip.\n";
                break;
            }

            std::string fullpath = fs::current_path().string() + "/" + filename_inzip;
            std::cout << "Extracting: " << fullpath << "\n";

            if (filename_inzip[strlen(filename_inzip) - 1] == '/') {
                fs::create_directories(fullpath);
            }
            else {
                if (unzOpenCurrentFile(zipfile) != UNZ_OK) {
                    std::cout << "Failed to open file inside zip.\n";
                    break;
                }

                FILE* fout = fopen(fullpath.c_str(), "wb");
                if (fout == NULL) {
                    std::cout << "Failed to create file: " << fullpath << "\n";
                    unzCloseCurrentFile(zipfile);
                    break;
                }

                char buf[8192];
                int bytes_read = 0;
                while ((bytes_read = unzReadCurrentFile(zipfile, buf, sizeof(buf))) > 0) {
                    fwrite(buf, 1, bytes_read, fout);
                }

                fclose(fout);
                unzCloseCurrentFile(zipfile);

                if (bytes_read < 0) {
                    std::cout << "Error reading file from zip.\n";
                    break;
                }
            }
        } while (unzGoToNextFile(zipfile) == UNZ_OK);

        unzClose(zipfile);
        std::cout << "Unzipping complete.\n";
    }

    void replace_files(const std::string& src_dir, const std::string& dest_dir) {
        std::string old_file = exe_path + ".old";
        if (fs::exists(exe_path)) {
            fs::rename(exe_path, old_file);
            std::cout << "Renamed old executable to: " << old_file << "\n";
        }

        for (const auto& entry : fs::recursive_directory_iterator(src_dir)) {
            fs::path relative_path = fs::relative(entry.path(), src_dir);
            fs::path dest_path = fs::path(dest_dir) / relative_path;

            if (fs::is_directory(entry)) {
                fs::create_directories(dest_path);
            }
            else {
                fs::copy_file(entry.path(), dest_path, fs::copy_options::overwrite_existing);
                std::cout << "Copied file from: " << entry.path() << " to: " << dest_path << "\n";
            }
        }

        std::string new_exe = src_dir + "/Planar.exe";
        if (fs::exists(new_exe)) {
            fs::rename(new_exe, exe_path);
            std::cout << "Renamed new executable to: " << exe_path << "\n";
        }
    }

    void cleanup_old_files(const std::string& src_dir, const std::string& dest_dir) {
        for (const auto& entry : fs::recursive_directory_iterator(dest_dir)) {
            std::string relative_path = fs::relative(entry.path(), dest_dir).string();
            if (!fs::exists(src_dir + "/" + relative_path)) {
                fs::remove_all(entry);
            }
        }
    }

    void replace_and_restart(const std::string& new_version) {
        std::string new_exe = fs::current_path().string() + "/Planar.exe";

        if (fs::exists(exe_path)) {
            std::string old_exe = exe_path + ".old";
            if (fs::exists(old_exe)) {
                fs::remove(old_exe);
            }
        }

        if (fs::exists(new_exe)) {
            fs::rename(new_exe, exe_path);
            std::cout << "Replaced old executable with the new one: " << exe_path << "\n";
        }

        STARTUPINFO si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        if (CreateProcess(NULL, (LPWSTR)exe_path.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        else {
            std::cout << "Failed to restart the application.\n";
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Executable path not provided.\n";
        return 1;
    }
    std::cout << "Starting updater with executable path: " << argv[1] << "\n";
    Updater updater("Shivar-J", "planar", argv[1]);
    updater.check_for_updates();

    return 0;
}
