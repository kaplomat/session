#include <iostream>
#include <memory>
#include <algorithm>
#include <map>
#include <sstream>

#include <boost/format.hpp>
#include <boost/program_options.hpp>

#include <cstring>
#include <cassert>

#include <sqlite_orm/sqlite_orm.h>

#include "db.h"
#include "model.h"

using namespace std;

enum class validity {
    valid,
    invalid,
    empty
};

auto open_database(std::string filename) {
    using namespace sqlite_orm;
    using namespace model;

    return make_storage(filename,
        make_table(SESSION_META_INFO,
            make_column("id", &session_meta_t::id,
                        autoincrement(), primary_key()),
            make_column("key", &session_meta_t::key, unique()),
            make_column("value", &session_meta_t::value)),
        make_table(SESSION_SESSION_INFO,
            make_column("id", &session_name_t::id,
                        autoincrement(), primary_key()),
            make_column("name", &session_name_t::name, unique())));
}

using storage_instance = decltype(open_database(""));

std::string sync_result_str(sqlite_orm::sync_schema_result res) {
    using namespace sqlite_orm;
    switch (res) {
        case sync_schema_result::new_table_created:
            return "create table";
        case sync_schema_result::already_in_sync:
            return "leave unchanged";
        case sync_schema_result::old_columns_removed:
            return "remove old columns";
        case sync_schema_result::new_columns_added:
            return "add new columns";
        case sync_schema_result::new_columns_added_and_old_columns_removed:
            return "remove old and add new columns";
        case sync_schema_result::dropped_and_recreated:
            return "drop and create new table";
    }

    return (boost::format("%1%???") % res).str();
}

template<class Table>
bool ensure_empty_table(storage_instance& storage) {
    bool result = storage.count<Table>() == 0;
    assert (result);
    return result;
}

validity validate_database(storage_instance& storage) {
    using namespace model;
    using namespace sqlite_orm;

    bool possibly_empty = false;
    bool truly_empty = true;
    std::string version;

    vector<session_meta_t> entries = storage.get_all<session_meta_t>(
        where(is_equal(&session_meta_t::key, SESSION_META_VERSION))
    );

    size_t count = entries.size();
    assert(count <= 1);

    if (count == 0) {
        cerr << "No version entry - will check the database" << endl;
        possibly_empty = true;
    }
    else {
        version = entries[0].value;
    }

    if (possibly_empty) {
        if (!ensure_empty_table<session_meta_t>(storage)) {
            cerr << "meta table is not empty" << endl;
            truly_empty = false;
        }

        if (!ensure_empty_table<session_name_t>(storage)) {
            cerr << "info table is not empty" << endl;
            truly_empty = false;
        }
    }

    if (possibly_empty) {
        if (truly_empty) {
            return validity::empty;
        }
        return validity::invalid;
    }

    if (version != DB_VERSION) {
        cerr << "invalid version " << version << "; won't proceed" << endl;
        return validity::invalid;
    }

    return validity::valid;
}

bool initialize_database(storage_instance storage) {
    model::session_meta_t entry{-1, SESSION_META_VERSION, DB_VERSION};
    auto id = storage.insert(entry);

    return true;
}

template<class ...Ts>
bool prepare_database(sqlite_orm::storage_t<Ts...>& storage) {
    using namespace sqlite_orm;
    auto result = storage.sync_schema_simulate(false);

    cerr << "preparing database. i will:" << endl;
    for (auto it = result.begin(); it != result.end(); it++) {
        cerr << "    "
             << sync_result_str(it->second) << ": "
             << it->first << endl;
    }

    storage.sync_schema(false);

    validity v = validate_database(storage);
    switch (v) {
    case validity::valid:
        cerr << "database valid" << endl;
        return true;
    case validity::invalid:
        cerr << "database is CORRUPTED" << endl;
        return false;
    case validity::empty:
        cerr << "will initialize empty database" << endl;
        return initialize_database(storage);
    }

    assert(false);
    return false;
}

model::entity_id get_session_id(storage_instance& db, std::string name) {
    using namespace model;
    using namespace sqlite_orm;

	vector<session_name_t> entries =
        db.get_all<session_name_t>(
            where(is_equal(&session_name_t::name, name))
        );

    size_t count = entries.size();

	if (count == 1) {
        return entries[0].id;
	}
    else if (count == 0) {
        return db.insert(session_name_t{-1, name});
    }

    assert(count > 1);
    throw std::runtime_error("more than one session with id");
}

void usage(boost::program_options::options_description desc) {
    cerr << desc << endl;
}

enum class operation_t {
    get,
    set
};

int main(int argc, char** argv) {
    namespace po = boost::program_options;

    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("id", po::value<string>(), "set session label")
            ("set", po::value<string>(), "specify variable to set")
            ("get", po::value<string>(), "specify variable to get")
            ("value", po::value<string>(), "specify value to set to variable")
            ("db", po::value<string>(), "database file to use [default available in the future]");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        usage(desc);
        return 0;
    }

    string db_file, id, name, value;
    operation_t op;

    if (vm.count("db")) {
        db_file = vm["db"].as<string>();
        cerr << "Will use database " << db_file << endl;
    }
    else {
        cerr << "No database specified." << endl
             << "   This development version doesn't have a default database." << endl
             << "   Please use --db <file> option to specify one." << endl;
        return 1;
    }

    if (vm.count("id")) {
        id = vm["id"].as<string>();
    } else {
        cerr << "Id not set." << endl;
        return 1;
    }

    if (vm.count("get")) {
        if (vm.count("set")) {
            cerr << "--get and --set options set in the same time!" << endl;
            return 1;
        }

        name = vm["get"].as<string>();
        op = operation_t::get;
    }

    if (vm.count("set")) {
        assert(vm.count("get") == 0);
        name = vm["set"].as<string>();
        op = operation_t::set;

        if (vm.count("value")) {
            value = vm["value"].as<string>();
        }
        else {
            cerr << "--set specified, but no --value flag" << endl;
            return 1;
        }
    }

    cerr << "Operation: "
         << name << '@' << id;

    if (op == operation_t::set) {
        cerr << " = " << value;
    }

    cerr << endl;

    bool result;
    auto db = open_database(db_file);

    result = prepare_database(db);
    if (!result) {
        cerr << "error preparing the database" << endl;
        exit(1);
    }

    model::entity_id session_id = get_session_id(db, id);
    cerr << "got session id " << session_id << endl;

    return 1;
}
