#pragma once

#include "CsvExporter.h"
#include <string>
#include <vector>

#ifndef MOVIE_REV_ITEM_DEFINED
#define MOVIE_REV_ITEM_DEFINED
struct MovieRevItem {
    std::string id;
    std::wstring title;
    std::wstring shortTitle;
    std::wstring genre;
    int showtimes = 0;
    int tickets = 0;
    long long revenue = 0;
    float share = 0.f;
    bool isHot = false;
};

struct MonthRevItem {
    std::string month;
    std::wstring displayMonth;
    long long ticketRev = 0;
    long long comboRev = 0;
    long long totalRev = 0;
    std::wstring growth;
    float comboShare = 0.f;
    bool isRecord = false;
};

struct ComboRevItem {
    std::string code;
    std::wstring name;
    int unitPrice = 0;
    int quantity = 0;
    long long revenue = 0;
    float share = 0.f;
    bool isTop1 = false;
};
#endif

/**
 * @brief Lớp con xuất báo cáo doanh thu & quyết toán tài chính CINECORE.
 * Kế thừa từ CsvExporter và triển khai Template Method.
 */
class RevenueReportExporter : public CsvExporter {
private:
    const std::vector<MovieRevItem>& moviesList;
    const std::vector<MonthRevItem>& monthsList;
    const std::vector<ComboRevItem>& combosList;
    long long totalRevenue;
    long long movieRevenue;
    long long comboRevenue;
    int totalTickets;
    int totalCombos;

protected:
    void writeReportHeader(std::ostream& out) override {
        double moviePct = totalRevenue > 0 ? (static_cast<double>(movieRevenue) * 100.0 / static_cast<double>(totalRevenue)) : 0.0;
        double comboPct = totalRevenue > 0 ? (static_cast<double>(comboRevenue) * 100.0 / static_cast<double>(totalRevenue)) : 0.0;

        out << "BÁO CÁO DOANH THU & QUYẾT TOÁN TÀI CHÍNH RẠP CINECORE\n";
        out << "Thời gian kết xuất," << escapeCsv(getCurrentTimestamp()) << "\n\n";

        out << "CHỈ SỐ TỔNG QUAN,GIÁ TRỊ\n";
        out << "Tổng doanh thu toàn rạp," << totalRevenue << " VNĐ\n";
        out << std::fixed << std::setprecision(1);
        out << "Doanh thu vé phim," << movieRevenue << " VNĐ (Chiếm " << moviePct << "%)\n";
        out << "Doanh thu combo F&B," << comboRevenue << " VNĐ (Chiếm " << comboPct << "%)\n";
        out << "Tổng số vé đã xuất," << totalTickets << " vé\n";
        out << "Tổng sản phẩm combo," << totalCombos << " combo\n\n";
    }

    void writeData(std::ostream& out) override {
        out << "DOANH THU CHI TIẾT THEO PHIM\n";
        out << "Hạng,Mã phim,Tên phim,Thể loại,Suất chiếu,Số vé bán,Doanh thu (VNĐ),Tỷ trọng\n";
        for (size_t i = 0; i < moviesList.size(); ++i) {
            out << (i + 1) << ","
                << escapeCsv(moviesList[i].id) << ","
                << escapeCsv(moviesList[i].title) << ","
                << escapeCsv(moviesList[i].genre) << ","
                << moviesList[i].showtimes << ","
                << moviesList[i].tickets << ","
                << moviesList[i].revenue << ","
                << moviesList[i].share << "%\n";
        }

        out << "\nTĂNG TRƯỞNG THEO THÁNG\n";
        out << "Tháng,Doanh thu vé (VNĐ),Doanh thu combo (VNĐ),Tổng doanh thu (VNĐ),Biến động\n";
        for (const auto& m : monthsList) {
            out << escapeCsv(m.month) << ","
                << m.ticketRev << ","
                << m.comboRev << ","
                << m.totalRev << ","
                << escapeCsv(m.growth) << "\n";
        }

        out << "\nTOP COMBO F&B BÁN CHẠY\n";
        out << "Mã combo,Tên combo,Đơn giá (VNĐ),Số lượng,Doanh thu (VNĐ),Tỷ trọng\n";
        for (const auto& c : combosList) {
            out << escapeCsv(c.code) << ","
                << escapeCsv(c.name) << ","
                << c.unitPrice << ","
                << c.quantity << ","
                << c.revenue << ","
                << c.share << "%\n";
        }
    }

public:
    RevenueReportExporter(const std::vector<MovieRevItem>& movies,
                          const std::vector<MonthRevItem>& months,
                          const std::vector<ComboRevItem>& combos,
                          long long totRev, long long movRev, long long comRev,
                          int totTickets, int totCombos)
        : moviesList(movies), monthsList(months), combosList(combos),
          totalRevenue(totRev), movieRevenue(movRev), comboRevenue(comRev),
          totalTickets(totTickets), totalCombos(totCombos) {}
};

