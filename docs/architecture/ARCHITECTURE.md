# Kiến trúc Hệ thống CiNeXiNe / CineCore

## 1. Tổng quan kiến trúc phân tầng (Layered Architecture)
Hệ thống được thiết kế theo mô hình kiến trúc phân tầng chuẩn mực trong lập trình hướng đối tượng (OOP):

1. **Presentation Layer (`include/ui/` & `src/ui/`):**
   - Quản lý giao diện đồ họa xây dựng trên thư viện SFML 3.
   - Các màn hình (`screens/`), các bảng chức năng (`panels/admin/`, `panels/customer/`), và các thành phần trực quan (`components/`).
2. **Business Logic Layer (`include/services/` & `src/services/`):**
   - Xử lý nghiệp vụ chính: Đặt vé, xác thực tài khoản, quản lý voucher, tìm kiếm phim, tính toán doanh thu.
3. **Data Access Layer (`include/repositories/` & `src/repositories/`):**
   - Đóng gói logic đọc/ghi và đồng bộ dữ liệu với hệ thống file text (`data/`).
4. **Data Structures Layer (`include/data_structures/` & `src/data_structures/`):**
   - Triển khai các cấu trúc dữ liệu cốt lõi tự xây dựng: Danh sách liên kết đôi (DLL), Bảng băm (HashTable), Cây B-Tree, Cây tiền tố (Trie), Vector, Stack, Queue.
5. **Exporters Module (`include/exporters/` & `src/exporters/`):**
   - Áp dụng Template Method Pattern xuất báo cáo CSV vào thư mục `exports/`.
6. **Cross-Cutting Utilities (`include/utils/` & `src/utils/`):**
   - Các tiện ích dùng chung: Xử lý file, xử lý chuỗi, định dạng ngày giờ, hash mật khẩu, kiểm tra hợp lệ.

