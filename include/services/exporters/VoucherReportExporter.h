#pragma once

#include "CsvExporter.h"
#include <string>
#include <vector>

#ifndef VOUCHER_ITEM_DEFINED
#define VOUCHER_ITEM_DEFINED
struct VoucherItem {
    std::string code;
    std::wstring subtitle;
    int type = 2;
    int value = 20;
    int minBill = 0;
    int recipientCount = 145;
    std::wstring tag = L"";
    bool isChecked = false;
};

struct RecipientItem {
    std::string email;
    std::wstring fullName;
    std::string receivedDate;
    int quantity = 1;
    bool isUsed = false;
};
#endif

/**
 * @brief Lớp con xuất báo cáo hệ thống voucher và khách hàng nhận voucher CINECORE.
 * Kế thừa từ CsvExporter và triển khai Template Method.
 */
class VoucherReportExporter : public CsvExporter {
private:
    const std::vector<VoucherItem>& vouchers;
    const std::vector<RecipientItem>& recipients;
    std::string currentVoucherCode;

    static std::string formatPrice(int p) {
        std::string s = std::to_string(p);
        int n = static_cast<int>(s.length()) - 3;
        while (n > 0) {
            s.insert(static_cast<size_t>(n), ".");
            n -= 3;
        }
        return s;
    }

protected:
    void writeReportHeader(std::ostream& out) override {
        out << "BÁO CÁO CHI TIẾT HỆ THỐNG VOUCHER & KHÁCH HÀNG NHẬN\n";
        out << "Thời gian kết xuất," << escapeCsv(getCurrentTimestamp()) << "\n";
        out << "Tổng số chiến dịch voucher," << vouchers.size() << "\n\n";
    }

    void writeData(std::ostream& out) override {
        out << "MÃ VOUCHER,TÊN CHIẾN DỊCH,LOẠI VOUCHER,GIÁ TRỊ GIẢM,ĐƠN TỐI THIỂU (VNĐ),SỐ NGƯỜI NHẬN,TRẠNG THÁI\n";
        for (const auto& v : vouchers) {
            std::string typeStr = (v.type == 2) ? "Giảm theo %" : "Giảm tiền trực tiếp";
            std::string valStr = (v.type == 2) ? (std::to_string(v.value) + "%") : (formatPrice(v.value) + " đ");
            out << escapeCsv(v.code) << ","
                << escapeCsv(v.subtitle) << ","
                << escapeCsv(typeStr) << ","
                << escapeCsv(valStr) << ","
                << v.minBill << ","
                << v.recipientCount << ","
                << "\"Đang phát hành\"\n";
        }

        out << "\n\n";
        std::string curCode = currentVoucherCode.empty() ? "WELCOME" : currentVoucherCode;
        out << "DANH SÁCH CHI TIẾT KHÁCH HÀNG NHẬN VOUCHER (" << curCode << ")\n";
        out << "STT,EMAIL KHÁCH HÀNG,HỌ VÀ TÊN,SỐ LƯỢNG,NGÀY CẤP,TRẠNG THÁI\n";

        int stt = 1;
        for (const auto& r : recipients) {
            out << stt++ << ","
                << escapeCsv(r.email) << ","
                << escapeCsv(r.fullName) << ","
                << r.quantity << ","
                << escapeCsv(r.receivedDate) << ","
                << (r.isUsed ? "\"Đã dùng\"" : "\"Chưa dùng\"") << "\n";
        }
    }

public:
    VoucherReportExporter(const std::vector<VoucherItem>& vList,
                          const std::vector<RecipientItem>& rList,
                          const std::string& selVoucherCode)
        : vouchers(vList), recipients(rList), currentVoucherCode(selVoucherCode) {}
};

