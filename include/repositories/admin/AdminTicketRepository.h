#pragma once

#include <string>
#include "data-structures/DLL.h"
#include "repositories/booking/TicketRepository.h"

/**
 * @brief AdminTicketRepository provides ticket management operations for Admin panels.
 * Inherits all core CRUD and seat release capabilities from the unified TicketRepository.
 */
class AdminTicketRepository : public TicketRepository {
public:
    explicit AdminTicketRepository(const std::string& path);
};
