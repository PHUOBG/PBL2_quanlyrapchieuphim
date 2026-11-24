#pragma once

#include "CsvExporter.h"
#include <string>
#include <vector>

#ifndef MOVIE_STAT_ITEM_DEFINED
#define MOVIE_STAT_ITEM_DEFINED
struct MovieStatItem {
    std::string title;
    std::wstring displayTitle;
    std::wstring shortTitle;
    int tickets = 0;
    long long revenue = 0;
};

struct MonthStatItem {
    std::string month;
    std::wstring displayMonth;
    long long revenue = 0;
    std::wstring changeBadge;
    sf::Color badgeBg;
    sf::Color badgeText;
    bool isRecord = false;
};
#endif

/**
 * @brief Lớp con xuất báo cáo tổng quan doanh thu & vận hành rạp CINECORE.
 * Kế thừa từ CsvExporter và triển khai Template Method.
 */
class OverviewReportExporter : public CsvExporter {
private:
    std::wstring periodStr;
    std::wstring roomStr;
    std::wstring queryStr;
    long long totalRevenue;
    long long movieRevenue;
    long long comboRevenue;
    int totalTickets;
    int standardTickets;
    int vipTickets;
    int newCustomers;
    const std::vector<MovieStatItem>& topMoviesList;
    const std::vector<MonthStatItem>& monthlyStatsList;

protected:
    void writeReportHeader(std::ostream& out) override {
        out << "BÁO CÁO TỔNG QUAN DOANH THU & VẬN HÀNH RẠP CINECORE\n";
        out << "Thời gian kết xuất," << escapeCsv(getCurrentTimestamp()) << "\n";
        out << "Kỳ báo cáo," << escapeCsv(periodStr.empty() ? L"Toàn kỳ" : periodStr) << "\n";
        out << "Phòng chiếu," << escapeCsv(roomStr.empty() ? L"Tất cả phòng" : roomStr) << "\n";
        out << "Từ khóa lọc," << escapeCsv(queryStr.empty() ? L"Không có" : queryStr) << "\n\n";

        out << "CHỈ SỐ KPI,GIÁ TRỊ\n";
        out << "Tổng doanh thu kỳ chọn," << totalRevenue << " VNĐ\n";
        out << "Doanh thu vé phim," << movieRevenue << " VNĐ\n";
        out << "Doanh thu combo F&B," << comboRevenue << " VNĐ\n";
        out << "Tổng số vé bán ra," << totalTickets << " vé (Standard: " << standardTickets << " - VIP: " << vipTickets << ")\n";
        out << "Khách hàng mới đăng ký," << newCustomers << " khách hàng\n\n";
    }

    void writeData(std::ostream& out) override {
        out << "HẠNG,TÊN PHIM,SỐ VÉ BÁN,DOANH THU PHIM (VNĐ)\n";
        for (size_t i = 0; i < topMoviesList.size(); ++i) {
            out << (i + 1) << ","
                << escapeCsv(topMoviesList[i].title) << ","
                << topMoviesList[i].tickets << ","
                << topMoviesList[i].revenue << "\n";
        }

        out << "\nTHÁNG,BIẾN ĐỘNG,DOANH THU (VNĐ)\n";
        for (const auto& m : monthlyStatsList) {
            out << escapeCsv(m.month) << ","
                << escapeCsv(m.changeBadge) << ","
                << m.revenue << "\n";
        }
    }

public:
    OverviewReportExporter(const std::wstring& period,
                           const std::wstring& room,
                           const std::wstring& query,
                           long long totRev, long long movRev, long long comRev,
                           int totTickets, int stdTickets, int vipTicks, int newCust,
                           const std::vector<MovieStatItem>& topMovies,
                           const std::vector<MonthStatItem>& monthlyStats)
        : periodStr(period), roomStr(room), queryStr(query),
          totalRevenue(totRev), movieRevenue(movRev), comboRevenue(comRev),
          totalTickets(totTickets), standardTickets(stdTickets), vipTickets(vipTicks),
          newCustomers(newCust), topMoviesList(topMovies), monthlyStatsList(monthlyStats) {}
};

