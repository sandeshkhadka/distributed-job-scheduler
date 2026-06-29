#include "pg_admin_db.hpp"
#include "pg_db.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

namespace {

static std::string random_hex_string(int bytes) {
    std::ifstream urandom("/dev/urandom", std::ios::binary);
    if (!urandom)
        throw std::runtime_error("cannot open /dev/urandom");
    std::vector<unsigned char> buf(bytes);
    urandom.read(reinterpret_cast<char*>(buf.data()), bytes);
    std::ostringstream hex;
    for (auto b : buf)
        hex << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    return hex.str();
}

int token_list_cb(void* data, int argc, char** argv, char**) {
    auto* records = static_cast<std::vector<TokenRecord>*>(data);
    TokenRecord r;
    r.id = std::stoi(argv[0]);
    if (argc > 1)
        r.token = argv[1] ? argv[1] : "";
    if (argc > 2)
        r.description = argv[2] ? argv[2] : "";
    if (argc > 3)
        r.token_type = argv[3] ? argv[3] : "client";
    if (argc > 4)
        r.active = std::stoi(argv[4]) != 0;
    if (argc > 5)
        r.created_at = argv[5] ? argv[5] : "";
    records->push_back(r);
    return 0;
}

int usage_list_cb(void* data, int argc, char** argv, char**) {
    auto* records = static_cast<std::vector<UsageRecord>*>(data);
    UsageRecord r;
    r.client_id = std::stoi(argv[0]);
    r.token_id = std::stoi(argv[1]);
    r.token_short = argv[2] ? std::string(argv[2]).substr(0, 12) + "..." : "";
    if (argc > 3)
        r.last_used_at = argv[3] ? argv[3] : "";
    records->push_back(r);
    return 0;
}

} // anonymous namespace

PgAdminDatabase::PgAdminDatabase() {
    PgDatabase::init();
    PgDatabase::instance().apply_migrations("migrations/001_init.sql");
}

std::string PgAdminDatabase::generate_token(const std::string& description,
                                            const std::string& token_type) {
    std::string token = random_hex_string(32);
    std::string q = "INSERT INTO auth_tokens (token, description, token_type) VALUES ('" + token +
                    "', '" + description + "', '" + token_type + "')";
    PgDatabase::instance().execute(q);
    return token;
}

bool PgAdminDatabase::revoke_token(const std::string& id_or_token) {
    std::string q;
    if (id_or_token.find_first_not_of("0123456789") == std::string::npos) {
        q = "UPDATE auth_tokens SET active = 0 WHERE id = " + id_or_token;
    } else {
        q = "UPDATE auth_tokens SET active = 0 WHERE token = '" + id_or_token + "'";
    }
    try {
        PgDatabase::instance().execute(q);
        return true;
    } catch (...) {
        return false;
    }
}

std::vector<TokenRecord> PgAdminDatabase::list_tokens() {
    std::vector<TokenRecord> records;
    PgDatabase::instance().execute("SELECT id, token, description, token_type, active, created_at "
                                   "FROM auth_tokens ORDER BY id DESC",
                                   token_list_cb,
                                   &records);
    return records;
}

std::vector<UsageRecord> PgAdminDatabase::list_usage() {
    std::vector<UsageRecord> records;
    PgDatabase::instance().execute(
        "SELECT u.client_id, u.token_id, t.token, u.last_used_at "
        "FROM client_token_usage u JOIN auth_tokens t ON u.token_id = t.id "
        "ORDER BY u.last_used_at DESC",
        usage_list_cb,
        &records);
    return records;
}
