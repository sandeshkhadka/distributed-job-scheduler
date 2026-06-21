#include "argparse.hpp"
#if defined(USE_PG) && USE_PG
#include "pg_admin_db.hpp"
using AdminDb = PgAdminDatabase;
#else
#include "admin_db.hpp"
using AdminDb = AdminDatabase;
#endif
#include <iostream>

int main(int argc, char* argv[]) {
    argparse::ArgumentParser program("djs-admin");

    argparse::ArgumentParser gen_cmd("gen-token");
    gen_cmd.add_description("Generate a new auth token");
    gen_cmd.add_argument("--desc", "-d").required().help("Description for the token");
    gen_cmd.add_argument("--type", "-t")
        .default_value(std::string("client"))
        .help("Token type: client or worker");

    argparse::ArgumentParser list_cmd("list-tokens");
    list_cmd.add_description("List all auth tokens");

    argparse::ArgumentParser revoke_cmd("revoke-token");
    revoke_cmd.add_description("Revoke a token by id or token string");
    revoke_cmd.add_argument("id_or_token").help("Token id or full token string");

    argparse::ArgumentParser usage_cmd("usage");
    usage_cmd.add_description("Show client token usage");

    program.add_subparser(gen_cmd);
    program.add_subparser(list_cmd);
    program.add_subparser(revoke_cmd);
    program.add_subparser(usage_cmd);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    try {
        AdminDb db;

        if (program.is_subcommand_used(gen_cmd)) {
            auto desc = gen_cmd.get<std::string>("--desc");
            auto type = gen_cmd.get<std::string>("--type");
            std::string token = db.generate_token(desc, type);
            std::cout << token << std::endl;
        } else if (program.is_subcommand_used(list_cmd)) {
            auto tokens = db.list_tokens();
            std::cout << "ID  | Token (truncated)        | Description       | Type   | Active | "
                         "Created\n";
            std::cout << "----+---------------------------+-------------------+--------+--------+--"
                         "----------------------\n";
            for (const auto& t : tokens) {
                auto short_token = t.token.substr(0, 16) + "...";
                printf("%-3d | %-25s | %-17s | %-6s | %-6s | %s\n",
                       t.id,
                       short_token.c_str(),
                       t.description.c_str(),
                       t.token_type.c_str(),
                       t.active ? "yes" : "no",
                       t.created_at.c_str());
            }
        } else if (program.is_subcommand_used(revoke_cmd)) {
            auto val = revoke_cmd.get<std::string>("id_or_token");
            if (db.revoke_token(val)) {
                std::cout << "Token revoked." << std::endl;
            } else {
                std::cerr << "Failed to revoke token." << std::endl;
                return 1;
            }
        } else if (program.is_subcommand_used(usage_cmd)) {
            auto records = db.list_usage();
            std::cout << "Client ID | Token ID | Token           | Last Used\n";
            std::cout << "----------+----------+-----------------+------------------------\n";
            for (const auto& r : records) {
                printf("%-9d | %-8d | %-15s | %s\n",
                       r.client_id,
                       r.token_id,
                       r.token_short.c_str(),
                       r.last_used_at.c_str());
            }
        } else {
            std::cerr << program;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
