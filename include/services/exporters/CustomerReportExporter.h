#pragma once

#include "CsvExporter.h"
#include <string>
#include <vector>

#ifndef CUSTOMER_TIER_DEFINED
#define CUSTOMER_TIER_DEFINED
enum class CustomerTier {
    STANDARD,
    SILVER,
    GOLD
};

struct CustomerItem {
    std::string email;
    std::string fullName;
    std::wstring displayFullName;
    std::string birthDate;
    std::string phone;
    std::string registeredDate;
    time_t registeredAt = 0;
    std::string uid;
    std::wstring initials;
    sf::Color avatarBg;
    CustomerTier tier = CustomerTier::STANDARD;
    int ticketsBought = 0;
    long long totalSpent = 0;
    bool selected = false;
};
#endif

/**
 * @brief Lớp con xuất báo cáo danh sách khách hàng & hội viên CINECORE.
 * Kế thừa từ CsvExporter và triển khai Template Method.
 */
class CustomerReportExporter : public CsvExporter {
private:
    const std::vector<CustomerItem>& customers;

protected:
    void writeReportHeader(std::ostream& out) override {
        out << "DANH SÁCH KHÁCH HÀNG & HỘI VIÊN RẠP CINECORE\n";
        out << "Thời gian kết xuất," << escapeCsv(getCurrentTimestamp()) << "\n";
        out << "Tổng số khách hàng," << customers.size() << "\n\n";
    }

    void writeData(std::ostream& out) override {
        out << "STT,Mã khách hàng,Họ và tên,Email,Số điện thoại,Hạng thẻ,Ngày đăng ký,Số vé đã mua,Tổng chi tiêu (VNĐ)\n";
        for (size_t i = 0; i < customers.size(); ++i) {
            std::string tName = (customers[i].tier == CustomerTier::GOLD) ? "Hạng Vàng" :
                                (customers[i].tier == CustomerTier::SILVER ? "Hạng Bạc" : "Tiêu chuẩn");
            out << (i + 1) << ","
                << escapeCsv(customers[i].uid) << ","
                << escapeCsv(customers[i].fullName) << ","
                << escapeCsv(customers[i].email) << ","
                << escapeCsv(customers[i].phone) << ","
                << escapeCsv(tName) << ","
                << escapeCsv(customers[i].registeredDate) << ","
                << customers[i].ticketsBought << ","
                << customers[i].totalSpent << "\n";
        }
    }

    void writeSummary(std::ostream& out) override {
        long long totalSpending = 0;
        int totalTickets = 0;
        for (const auto& c : customers) {
            totalSpending += c.totalSpent;
            totalTickets += c.ticketsBought;
        }
        out << "\nTỔNG KẾT HỘI VIÊN\n";
        out << "Tổng số hội viên," << customers.size() << "\n";
        out << "Tổng số vé hội viên đã mua," << totalTickets << "\n";
        out << "Tổng chi tiêu của hội viên," << totalSpending << " VNĐ\n";
    }

public:
    explicit CustomerReportExporter(const std::vector<CustomerItem>& custList)
        : customers(custList) {}
};

