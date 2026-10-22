#include "BBSClient.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include <curl/curl.h>

#include <iostream>
#include <fstream>

using namespace std;

static size_t write_file_callback(void *ptr, size_t size, size_t nmemb, void *stream) {
    FILE* file = (FILE*)stream;
    return fwrite(ptr, size, nmemb, file);
}

string BBSClient::GetCartUrlFromLid(const string& lid) {
    if (lid.empty()) return "";
    
    // Check if purely numeric
    bool isNumeric = true;
    for (char c : lid) {
        if (!isdigit(c)) {
            isNumeric = false;
            break;
        }
    }

    string prefix;
    if (isNumeric) {
        long idVal = atol(lid.c_str());
        prefix = to_string(idVal / 10000);
    } else {
        prefix = lid.substr(0, 2);
    }

    return "https://www.lexaloffle.com/bbs/cposts/" + prefix + "/" + lid + ".p8.png";
}

bool BBSClient::DownloadCart(const string& url, const string& destinationPath) {
    Logger_Write("BBSClient: Downloading %s to %s\n", url.c_str(), destinationPath.c_str());

    CURL* curl = curl_easy_init();
    if (!curl) {
        Logger_Write("BBSClient: curl_easy_init() failed\n");
        return false;
    }

    FILE* fp = fopen(destinationPath.c_str(), "wb");
    if (!fp) {
        Logger_Write("BBSClient: Cannot open destination file %s\n", destinationPath.c_str());
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Orbis8/1.0 (PS4 Homebrew by birth1mark)");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // Allow connection on homebrew runtimes

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    fclose(fp);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || http_code != 200) {
        Logger_Write("BBSClient: Download failed with CURL error: %s (HTTP %ld)\n", curl_easy_strerror(res), http_code);
        unlink(destinationPath.c_str()); // delete incomplete file
        return false;
    }

    Logger_Write("BBSClient: Download successful (%s)\n", destinationPath.c_str());
    return true;
}

bool BBSClient::DownloadCartByLid(const string& lid, const string& targetFilename, const string& cartsDirectory) {
    string url = GetCartUrlFromLid(lid);
    string fullPath = cartsDirectory + "/" + targetFilename;
    return DownloadCart(url, fullPath);
}

vector<BBSCartItem> BBSClient::GetFeaturedCarts(const string& cartsDirectory) {
    // Curated high-reputation community games from Lexaloffle BBS
    struct PredefinedCart {
        const char* title;
        const char* author;
        const char* category;
        const char* lid;
        const char* filename;
    };

    static const PredefinedCart list[] = {
        {"Celeste Classic", "Maddy Thorson & Noel Berry", "Platformer", "11925", "celeste_bbs.p8.png"},
        {"Slipways", "krajzeg", "Strategy", "57404", "slipways.p8.png"},
        {"Digable Planetoids", "zep", "Arcade / Action", "digable_planetoids-0", "digable_planetoids.p8.png"}
    };

    vector<BBSCartItem> result;
    for (const auto& item : list) {
        string fullPath = cartsDirectory + "/" + item.filename;
        struct stat st;
        bool exists = (stat(fullPath.c_str(), &st) == 0 && st.st_size > 0);

        result.push_back({
            item.title,
            item.author,
            item.category,
            item.filename,
            GetCartUrlFromLid(item.lid),
            exists
        });
    }

    return result;
}
