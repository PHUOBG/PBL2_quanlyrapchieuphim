#pragma once
#include <string>

/**
 * @brief Tiện ích định dạng tiền tệ và số liệu tài chính.
 */
class CurrencyUtils {
public:
    static std::string formatCurrency(long long amount) {
        std::string s = std::to_string(amount);
        int n = static_cast<int>(s.length()) - 3;
        while (n > 0) {
            s.insert(static_cast<size_t>(n), ".");
            n -= 3;
        }
        return s + " đ";
    }

    static std::string formatCurrencyVnd(long long amount) {
        std::string s = std::to_string(amount);
        int n = static_cast<int>(s.length()) - 3;
        while (n > 0) {
            s.insert(static_cast<size_t>(n), ".");
            n -= 3;
        }
        return s + " VNĐ";
    }
};

