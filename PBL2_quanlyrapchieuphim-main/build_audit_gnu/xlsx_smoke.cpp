#include "utils/XlsxWriter.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
int main() {
    std::ifstream file("data/tickets.txt");
    std::string line; std::vector<std::vector<std::string>> rows;
    std::getline(file,line); std::stringstream head(line); std::string cell; std::vector<std::string> headers;
    while(std::getline(head,cell,'|')) headers.push_back(cell); rows.push_back(headers);
    while(std::getline(file,line)) { std::stringstream ss(line); std::vector<std::string> row; while(std::getline(ss,cell,'|')) row.push_back(cell); if(row.size()>=13) rows.push_back({row[3],row[0],row[1],row[2],row[5],row[6],row[8],row[7],row[9]}); }
    return XlsxWriter::write("build_audit_gnu/revenue_smoke.xlsx", rows) ? 0 : 1;
}
