#pragma once
#include <string>

/**
 * ContentGuard – Database Configuration
 * Replace CONNECTION_STRING with your actual Neon connection string.
 * Format: postgresql://user:password@ep-xxx.region.aws.neon.tech/ContentGuard?sslmode=require
 */
namespace DB {
    inline const std::string CONNECTION_STRING =
        "postgresql://neondb_owner:npg_2TJFqNlzPp5Y@ep-calm-heart-a1fxv4n4-pooler.ap-southeast-1.aws.neon.tech/contentGuard?sslmode=require&channel_binding=require";
}
