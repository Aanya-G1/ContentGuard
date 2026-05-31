#pragma once
#include <string>

/**
 * ContentGuard – Database Configuration (template)
 *
 * Copy this file to db_config.hpp and fill in your Neon/PostgreSQL connection string:
 *   cp database/db_config.example.hpp database/db_config.hpp
 *
 * db_config.hpp is gitignored — never commit real credentials.
 */
namespace DB {
    inline const std::string CONNECTION_STRING =
        "host=YOUR_NEON_HOST.neon.tech "
        "user=YOUR_USER "
        "password=YOUR_PASSWORD "
        "dbname=contentGuard "
        "sslmode=require";
}
