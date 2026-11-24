#pragma once

#include "CsvExporter.h"
#include <string>
#include <vector>

#ifndef COMBO_ITEM_DEFINED
#define COMBO_ITEM_DEFINED
struct ComboItem {
    std::string id;
    std::wstring name;
    std::wstring subtitle;
    std::wstring category;
    int price = 30000;
    bool isHot = false;
    bool isAvailable = true;
    bool isChecked = false;
};
#endif

/**
 * @brief Lớp con xuất báo cáo bảng giá F&B & combo bắp nước CINECORE.
 * Kế thừa từ CsvExporter và triển khai Template Method.
 */
class ComboReportExporter : public CsvExporter {
private:
    const std::vector<ComboItem>& combos;

protected:
    void writeReportHeader(std::ostream& out) override {
        out << "BẢNG GIÁ & DANH MỤC COMBO BẮP NƯỚC F&B CINECORE\n";
        out << "Thời gian kết xuất," << escapeCsv(getCurrentTimestamp()) << "\n";
        out << "Tổng số combo," << combos.size() << "\n\n";
    }

    void writeData(std::ostream& out) override {
        out << "Mã Combo,Tên Combo,Phân Loại,Mô Tả,Giá Bán (VNĐ),Trạng Thái\n";
        for (const auto& c : combos) {
            out << escapeCsv(c.id) << ","
                << escapeCsv(c.name) << ","
                << escapeCsv(c.category) << ","
                << escapeCsv(c.subtitle) << ","
                << c.price << ","
                << (c.isAvailable ? "Đang bán" : "Ngừng bán") << "\n";
        }
    }

public:
    explicit ComboReportExporter(const std::vector<ComboItem>& comboList)
        : combos(comboList) {}
};

