#pragma once

enum class AppState {
    HOME,
    LOGIN,
    REGISTER,
    FORGOT_PASSWORD,
    BOOKING,
    MOVIE_DETAILS,
    ACCOUNT,
    
    ADMIN_DASHBOARD,        // Thống kê tổng quan
    ADMIN_MOVIES,           // Quản lý phim
    ADMIN_ROOMS,            // Quản lý phòng chiếu
    ADMIN_SHOWTIMES,        // Quản lý suất chiếu
    ADMIN_TICKETS,          // Quản lý vé
    ADMIN_COMBOS,           // Quản lý thực đơn combo bắp nước
    ADMIN_VOUCHERS,         // Quản lý voucher khuyến mãi
    ADMIN_CUSTOMERS,        // Quản lý khách hàng
    ADMIN_REVENUE,          // Thống kê tổng quan doanh thu
    ADMIN_SOLD_TICKETS,     // Thống kê doanh thu theo phim & báo cáo
    ADMIN_CHANGE_PASSWORD,  // Quản lý tài khoản Admin (Profile/Sửa info/Đổi MK/Xóa acc)
};