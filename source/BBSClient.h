#pragma once

#include <string>
#include <vector>

struct BBSCartItem {
    std::string title;
    std::string author;
    std::string category;
    std::string filename;
    std::string url;
    bool downloaded;
};

class BBSClient {
public:
    static std::string GetCartUrlFromLid(const std::string& lid);
    static bool DownloadCart(const std::string& url, const std::string& destinationPath);
    static std::vector<BBSCartItem> GetFeaturedCarts(const std::string& cartsDirectory);
    static bool DownloadCartByLid(const std::string& lid, const std::string& targetFilename, const std::string& cartsDirectory);
};
