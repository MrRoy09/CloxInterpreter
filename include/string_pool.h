#pragma once
#include <unordered_set>
#include <string>
#include <string_view>

class StringPool {
private:
    std::unordered_set<std::string> pool;
    
public:
    std::string_view intern(std::string_view str) {
        auto result = pool.insert(std::string(str));
        return *result.first;
    }
    
    std::string_view intern(const std::string& str) {
        auto result = pool.insert(str);
        return *result.first;
    }
    
    bool contains(std::string_view str) const {
        return pool.find(std::string(str)) != pool.end();
    }
    
    size_t size() const {
        return pool.size();
    }
};