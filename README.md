# PBL2 - HỆ THỐNG QUẢN LÝ RẠP CHIẾU PHIM & THỐNG KÊ DOANH THU (ROLE: QUẢN TRỊ VIÊN)

## 1. Giới thiệu dự án
Dự án được xây dựng phục vụ môn học **Đồ án Cơ sở Lập trình / Cấu trúc Dữ liệu & Giải thuật (PBL2)**.
Hệ thống tập trung chuyên biệt vào vai trò **Quản Trị Viên (Admin)**, phục vụ quản lý toàn diện các hoạt động vận hành rạp chiếu phim và phân tích, thống kê doanh thu đa chiều, không có role khách hàng.

---

## 2. Chi tiết các module chức năng

### 2.1. Quản lý tài khoản cá nhân (Quản trị viên)
- **Đăng ký Admin**: Tạo tài khoản quản trị viên mới với đầy đủ thông tin (Họ tên, Ngày sinh, Email, SĐT, Mật khẩu). Mặc định cấp quyền Admin.
- **Đăng nhập**: Xác thực tài khoản Admin bảo mật với thuật toán mã hóa mật khẩu Salted DJB2 Hash.
- **Quên mật khẩu**: Cơ chế đặt lại mật khẩu an toàn trực tiếp trên giao diện dành cho Admin.
- **Xem Profile / Chỉnh sửa thông tin cá nhân**: Xem thông tin chi tiết, sửa họ tên, ngày sinh, số điện thoại, đổi mật khẩu.
- **Xóa vĩnh viễn tài khoản Admin**: Có hộp thoại popup cảnh báo xác nhận *"Bạn có chắc muốn xóa vĩnh viễn tài khoản quản trị này? Hành động này không thể hoàn tác!"*. Sau khi xóa sẽ gỡ dữ liệu khỏi file và tự động đăng xuất.

### 2.2. Quản lý danh sách phim
- **Cơ sở dữ liệu**: ID phim, Tên phim, Phân loại, Quốc gia, Ngôn ngữ, Thể loại, Thời lượng, Ngày khởi chiếu, Ngày ngừng chiếu, Đạo diễn, Diễn viên, Tóm tắt, Poster, Trạng thái.
- **Xem danh sách phim**: Hiển thị bảng phim trực quan, chi tiết kèm poster và trạng thái.
- **Thêm phim mới**: Thêm đầy đủ thông tin phim và đường dẫn ảnh poster.
- **Sửa thông tin phim**: Cập nhật linh hoạt các trường dữ liệu phim.
- **Xóa phim**: Hộp thoại popup xác nhận trước khi xóa để tránh thao tác nhầm.
- **Reload**: Tải lại dữ liệu từ file tức thì sau khi thêm, sửa, xóa.

### 2.3. Quản lý suất chiếu
- **Lịch chiếu tổng quan**: Hiển thị toàn bộ suất chiếu trong ngày gồm: Giờ chiếu + Phòng + Phim + Giá vé.
- **Lọc thông minh**: Lọc danh sách theo ngày chiếu, lọc theo từng phòng chiếu.
- Tự động duy trì và dọn dẹp suất chiếu quá hạn.

### 2.4. Quản lý phòng chiếu
- **Theo dõi trạng thái phòng**: Hiển thị danh sách phòng (ID phòng + Tên phòng + Trạng thái phim đang chiếu / sắp chiếu).
- **Reload**: Làm mới trạng thái phòng chiếu.

### 2.5. Quản lý vé
- **Xem danh sách vé**: ID vé, ID suất chiếu, Email người đặt, Ngày đặt vé, Giờ đặt vé, Trạng thái vé.
- **Xử lý vé**: Hủy / Xóa vé có xác nhận.
- **Reload**: Cập nhật lại danh sách vé từ file lưu trữ.

### 2.6. Quản lý thực đơn combo bắp nước & Voucher khuyến mãi
- **Quản lý thực đơn combo**:
  - Xem danh sách combo bắp nước.
  - Thêm combo mới, chỉnh sửa thông tin, xóa combo.
  - Cập nhật đơn giá combo linh hoạt.
  - Reload danh sách thực đơn.
- **Quản lý Voucher khuyến mãi**:
  - Danh sách voucher: Thêm voucher mới, chỉnh sửa, xóa, phát hành đồng loạt cho khách hàng.
  - Chi tiết người nhận: Xem danh sách khách hàng sở hữu voucher, thêm người nhận thủ công, thu hồi voucher.

### 2.7. Quản lý khách hàng
- **Xem danh sách khách hàng**: Hiển thị đầy đủ danh sách tài khoản khách hàng đã đăng ký.
- **Xem chi tiết khách hàng**: Click vào khách hàng để xem lịch sử đặt vé và danh sách voucher mà khách hàng đang sở hữu.
- **Gỡ tài khoản khách hàng**: Có popup xác nhận: *"Bạn có chắc chắn muốn xóa tài khoản này không?"*.
- **Reload**: Làm mới bảng khách hàng.

### 2.8. Thống kê, phân tích doanh thu & báo cáo
- **Thống kê tổng quan**: Doanh thu trong ngày, doanh thu trong tháng, số lượng khách hàng mới, tỷ lệ lấp đầy rạp.
- **Thống kê theo phim**: Doanh thu từng phim, doanh thu bắp nước (combo), số vé bán ra.
- **Báo cáo định kỳ**: Lọc và xuất dữ liệu báo cáo theo tháng, quý, năm với biểu đồ trực quan.

---

## 3. Kiến trúc kỹ thuật & Cấu trúc dữ liệu tự cài đặt

Dự án tuân thủ nghiêm ngặt các tiêu chuẩn đồ án:
1. **Lập trình Hướng đối tượng (OOP)**:
   - **Encapsulation**: Tất cả các thực thể (User, Movie, Room, Showtime, Ticket, Combo, Voucher) đều được đóng gói thành các Class với private fields, public getters/setters.
   - **Inheritance**: `BaseScreen` làm lớp cơ sở cho các màn hình, các panel quản lý kế thừa và mở rộng.
   - **Polymorphism**: Sử dụng phương thức ảo `virtual void handleEvent()`, `virtual void update()`, `virtual void render()`.
2. **Kỹ thuật lập trình C++ (KTLT)**:
   - Sử dụng con trỏ quản lý bộ nhớ (`unique_ptr`, `shared_ptr`, raw pointer khi cần tối ưu).
   - Template programming, Operator overloading, Exception handling (`try-catch`).
3. **Cấu trúc dữ liệu tự cài đặt (Hạn chế tối đa STL)**:
   - `MyVector<T>` (`include/data-structures/MyVector.h`): Mảng động tự quản lý bộ nhớ, hỗ trợ dynamic resize, push_back, pop_back, operator[], range-based iterator.
   - `DLL<T>` (`include/data-structures/DLL.h`): Danh sách liên kết đôi (Doubly Linked List).
   - `MyQueue<T>` (`include/data-structures/MyQueue.h`): Hàng đợi FIFO tự cài đặt bằng linked nodes.
   - `MyStack<T>` (`include/data-structures/MyStack.h`): Ngăn xếp LIFO tự cài đặt.
   - `HashTable<K, V>` (`include/data-structures/HashTable.h`): Bảng băm tra cứu O(1), giải quyết va chạm bằng separate chaining và hàm băm DJB2.
   - `BTree<T>` (`include/data-structures/BTree.h`): Cây B-Tree phục vụ quản lý và tra cứu khóa.
   - `Trie` (`include/data-structures/Trie.h`): Cây tiền tố hỗ trợ tìm kiếm nhanh xâu ký tự.
4. **Tương tác dữ liệu tệp (File I/O)**:
   - Toàn bộ dữ liệu được lưu trữ, đọc/ghi nhất quán trong thư mục `data/` (`users.txt`, `movies.txt`, `rooms.txt`, `showtimes.txt`, `tickets.txt`, `combo.txt`, `vouchers.txt`).
5. **Giao diện đồ họa (SFML 3)**:
   - Thiết kế giao diện đồ họa hiện đại chuẩn UI/UX, hỗ trợ tiếng Việt Unicode UTF-8, bảng điều khiển Sidebar, hiệu ứng tương tác chuột (hover, click, animation), hộp thoại popup xác nhận thao tác.

---

## 4. Hướng dẫn khởi chạy

### Cách 1: Build bản mới nhất và chạy ngay bằng PowerShell (Khuyên dùng)
Lệnh dưới đây sẽ tự động đóng `app.exe` cũ (nếu đang mở), biên dịch lại mã nguồn mới nhất và khởi chạy ứng dụng:

```powershell
# Chạy từ thư mục gốc dự án (PBL2phuong)
Stop-Process -Name app -Force -ErrorAction SilentlyContinue; cmake --build build --config Release --target app; .\build\app.exe
```

*(Nếu terminal đang đứng sẵn bên trong thư mục `build\`, chạy: `Stop-Process -Name app -Force -ErrorAction SilentlyContinue; cmake --build . --config Release --target app; .\app.exe`)*

### Cách 2: Cấu hình CMake từ đầu (Khi mới clone repo)
```powershell
# Cấu hình CMake với Ninja và SFML 3
cmake -B build -G Ninja

# Biên dịch toàn bộ dự án
cmake --build build --config Release

# Chạy ứng dụng
.\build\app.exe
```

### Cách 3: Chạy bộ kiểm thử tự động (Automated Tests)
```powershell
cmake --build build --config Release --target core_tests
ctest --test-dir build --output-on-failure
```

### Cách 4: Chạy file Batch nhanh
Nếu đã biên dịch sẵn `build/app.exe`, có thể nhấp đúp chuột vào file **`chay_pbl2.bat`** tại thư mục gốc để mở nhanh ứng dụng.

### Tài khoản Admin mặc định có sẵn:
- **Email:** `admin@cinexine.vn` (hoặc `admin@cinema.com`)
- **Mật khẩu:** `admin123`
*(Bạn cũng có thể bấm "Chưa có tài khoản? Đăng ký Admin ngay" trên màn hình đăng nhập để tạo tài khoản Admin mới).*