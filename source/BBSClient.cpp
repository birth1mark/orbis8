#include "BBSClient.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cctype>

#include <curl/curl.h>

#include <iostream>
#include <fstream>
#include <algorithm>

using namespace std;

static size_t write_file_callback(void *ptr, size_t size, size_t nmemb, void *stream) {
    FILE* file = (FILE*)stream;
    return fwrite(ptr, size, nmemb, file);
}

string BBSClient::CleanLid(const string& rawLid) {
    string cleaned = rawLid;
    // Trim leading whitespace and '#'
    size_t start = cleaned.find_first_not_of(" \t\r\n#");
    if (start != string::npos) {
        cleaned = cleaned.substr(start);
    } else {
        return "";
    }
    // Trim trailing whitespace
    size_t end = cleaned.find_last_not_of(" \t\r\n");
    if (end != string::npos) {
        cleaned = cleaned.substr(0, end + 1);
    }
    return cleaned;
}

string BBSClient::GetCartUrlFromLid(const string& rawLid) {
    string lid = CleanLid(rawLid);
    if (lid.empty()) return "";

    // If full URL was provided, return directly
    if (lid.rfind("http://", 0) == 0 || lid.rfind("https://", 0) == 0) {
        return lid;
    }
    
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
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

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

    // Verify file format header: must be PNG (\x89PNG) or text (.p8 header)
    FILE* checkFp = fopen(destinationPath.c_str(), "rb");
    if (checkFp) {
        unsigned char header[8] = {0};
        size_t bytesRead = fread(header, 1, 8, checkFp);
        fclose(checkFp);

        bool isPng = (bytesRead >= 4 && header[0] == 0x89 && header[1] == 'P' && header[2] == 'N' && header[3] == 'G');
        bool isP8 = (bytesRead >= 6 && memcmp(header, "pico-8", 6) == 0);

        if (!isPng && !isP8) {
            Logger_Write("BBSClient: Downloaded file is not a valid PICO-8 cartridge! Discarding.\n");
            unlink(destinationPath.c_str());
            return false;
        }
    }

    Logger_Write("BBSClient: Download successful (%s)\n", destinationPath.c_str());
    return true;
}

bool BBSClient::DownloadCartByLid(const string& lid, const string& targetFilename, const string& cartsDirectory) {
    string url = GetCartUrlFromLid(lid);
    string fullPath = cartsDirectory + "/" + targetFilename;
    return DownloadCart(url, fullPath);
}

bool BBSClient::IsCartDownloaded(const string& filename, const string& cartsDirectory) {
    string fullPath = cartsDirectory + "/" + filename;
    struct stat st;
    return (stat(fullPath.c_str(), &st) == 0 && st.st_size > 0);
}

vector<BBSCartItem> BBSClient::GetFeaturedCarts(const string& cartsDirectory) {
    struct PredefinedCart {
        const char* id;
        const char* title;
        const char* author;
        const char* category;
        const char* filename;
    };

    static const PredefinedCart list[] = {
        {"11925", "Celeste Classic", "Maddy & Noel", "Platformer", "bbs_celeste.p8.png"},
        {"celeste_classic_2-5", "Celeste 2", "Noel & Maddy", "Platformer", "bbs_celeste2.p8.png"},
        {"57404", "Slipways", "krajzeg", "Strategy", "bbs_slipways.p8.png"},
        {"53161", "Mistigri", "B. Soule", "Metroidvania", "bbs_mistigri.p8.png"},
        {"54521", "Mai-Chan Buns", "Krystman", "Puzzle", "bbs_maichan.p8.png"},
        {"beckon_the_hellspawn-2", "Hellspawn", "MarcG", "Survivors", "bbs_hellspawn.p8.png"},
        {"dank_tomb-0", "Dank Tomb", "lucatron", "Puzzle Plat", "bbs_danktomb.p8.png"},
        {"digable_planetoids-0", "Dig Planetoids", "zep", "Arcade Action", "bbs_planetoids.p8.png"},
        {"spoopy-0", "Low Mem Sky", "Liquidream", "Space Sim", "bbs_lowmemsky.p8.png"},
        {"midnight_blue-1", "Midnight Blue", "Community", "Action", "bbs_midnightblue.p8.png"},
        {"elf_cannon-1", "Elf Cannon", "Community", "Physics", "bbs_elfcannon.p8.png"},
        {"bnl-1", "Blue and Lefty", "Community", "1K Jam", "bbs_bluelefty.p8.png"},
        {"jarlife-1", "Jar Life", "Community", "Simulation", "bbs_jarlife.p8.png"},
        {"blorb-1", "Blorb", "Community", "Arcade", "bbs_blorb.p8.png"},
        {"elf_curling-2", "Elf Curling", "Community", "Sports", "bbs_elfcurling.p8.png"}
    };

    vector<BBSCartItem> result;
    for (const auto& item : list) {
        bool exists = IsCartDownloaded(item.filename, cartsDirectory);
        result.push_back({
            item.id,
            item.title,
            item.author,
            item.category,
            item.filename,
            GetCartUrlFromLid(item.id),
            exists
        });
    }

    return result;
}
