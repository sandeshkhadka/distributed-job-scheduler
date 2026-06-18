#pragma once
#include <string>
#include <vector>

struct TokenRecord {
    int id;
    std::string token;
    std::string description;
    std::string token_type;
    bool active;
    std::string created_at;
};

struct UsageRecord {
    int client_id;
    int token_id;
    std::string token_short;
    std::string last_used_at;
};

class AdminDatabase {
  public:
    AdminDatabase();

    std::string generate_token(const std::string& description, const std::string& token_type);
    bool revoke_token(const std::string& id_or_token);
    std::vector<TokenRecord> list_tokens();
    std::vector<UsageRecord> list_usage();
};
