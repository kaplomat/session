#include <iostream>
#include <memory>
#include <algorithm>
#include <map>
#include <sstream>

#include <boost/program_options.hpp>

#include <cstring>
#include <cassert>

#include <sqlite3.h>

using namespace std;

using sqlite3_conn =
        std::shared_ptr<sqlite3>;

using sqlite3_handle =
        std::weak_ptr<sqlite3>;

using sqlite3_stmt_ptr =
        std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

sqlite3_conn make_conn(sqlite3* ptr) {
    return sqlite3_conn(ptr, &sqlite3_close);
}

sqlite3_stmt_ptr make_ptr(sqlite3_stmt* ptr) {
    return sqlite3_stmt_ptr(ptr, &sqlite3_finalize);
}

namespace _stmts {

const std::string prepare = "\
PRAGMA foreign_keys = off;                                \n\
BEGIN TRANSACTION;                                        \n\
                                                          \n\
-- Table: session.meta                                    \n\
CREATE TABLE IF NOT EXISTS                                \n\
    \"session.meta\" (                                    \n\
        name VARCHAR (32) UNIQUE PRIMARY KEY NOT NULL,    \n\
        value VARCHAR (100));                             \n\
                                                          \n\
-- Table: session.name                                    \n\
CREATE TABLE IF NOT EXISTS                                \n\
    \"session.name\" (                                    \n\
        id INT PRIMARY KEY,                               \n\
        name VARCHAR (100) UNIQUE NOT NULL);              \n\
                                                          \n\
-- Inserting data                                         \n\
INSERT INTO \"session.meta\" VALUES ('version', '0.1');   \n\
                                                          \n\
COMMIT TRANSACTION;                                       \n\
PRAGMA foreign_keys = on;                                 \n\
";

const std::string get_version = "\
SELECT value FROM \"session.meta\" WHERE name = 'version';\
";

}

enum class validity {
    valid,
    invalid,
    empty
};

class database;

class sqlite_exception : public std::exception {
private:
    int m_code;
    std::string m_what;

public:
    sqlite_exception(int sqlite_code);

    int code() const { return m_code; }

    virtual const char* what() const noexcept override;
};


class database {
public:
    using id_t = unsigned int;
    friend class statement;

private:
    sqlite3_conn m_conn;

public:
    static database open(std::string filename);
    database(database&& other) = default;

    database(const database& other) = delete;
    void operator =(const database& other) = delete;

    bool prepare();

    std::string describe_error();

    template<typename T>
    void insert(T entity);

    template<typename T>
    T find_by_id(id_t id);

private:
    explicit database(sqlite3_conn conn);

    validity check_if_valid();
    bool initialize_database();
};

static std::string _sqlite_what(int sqlite_code) {
    std::stringstream ss;
    ss << sqlite_code << " " << sqlite3_errstr(sqlite_code);

    return ss.str();
}

sqlite_exception::sqlite_exception(int sqlite_code) :
    m_code(sqlite_code),
    m_what(_sqlite_what(sqlite_code))
{ }

const char* sqlite_exception::what() const noexcept {
    return m_what.c_str();
}

database database::open(std::string filename) {
    sqlite3* ppDb;
    int result;

    cerr << "opening database " << filename << "..." << endl;

    result = sqlite3_open_v2(
            filename.c_str(), &ppDb,
            SQLITE_OPEN_READWRITE,
            nullptr);

    if (result != SQLITE_OK) {
        cerr << "could not open database: " << _sqlite_what(result) << endl;
        cerr << "will try creating a new one" << endl;
    }

    result = sqlite3_open_v2(
            filename.c_str(), &ppDb,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
            nullptr);

    if (result != SQLITE_OK) {
        throw sqlite_exception(result);
    }

    return database(make_conn(ppDb));
}

database::database(sqlite3_conn conn) :
    m_conn(conn)
{ }

bool database::prepare() {
    validity v = this->check_if_valid();
    switch (v) {
    case validity::valid:
        cerr << "database valid" << endl;
        return true;
    case validity::invalid:
        cerr << "database is CORRUPTED" << endl;
        return false;
    case validity::empty:
        cerr << "will initialize empty database" << endl;
        return this->initialize_database();
    }

    assert(false);
}

std::string database::describe_error() {
    const char* msg = sqlite3_errmsg(m_conn.get());
    return std::string(msg);
}

static int _handle_get_version(void* sink, int a, char** vals, char** names) {
    std::string* value = reinterpret_cast<std::string*>(sink);
    assert(value != nullptr);
    assert(a == 1);

    *value = std::string(*vals);
    return SQLITE_OK;
}

validity database::check_if_valid() {
    char* errmsg;
    std::unique_ptr<std::string> version(new std::string);
    *version = "";

    int result = sqlite3_exec(m_conn.get(),
            _stmts::get_version.c_str(),
            _handle_get_version,
            version.get(),
            &errmsg);

    if (errmsg != nullptr) {
        std::string msg = errmsg;
        sqlite3_free(errmsg);

        if (msg == "no such table: session.meta") {
            cerr << "database seems empty - no session.meta table" << endl;
            return validity::empty;
        }
        else {
            cerr << "There was an error: " << msg << endl;
            return validity::invalid;
        }
    }

    assert (result == SQLITE_OK);

    if (*version == "") {
        cerr << "No valid version found - database corrupted?" << endl;
        return validity::invalid;
    }

    cout << "version=" << *version << endl;
    if (*version != "0.1") {
        cerr << "unknown version; cannot proceed" << endl;
        return validity::invalid;
    }

    return validity::valid;
}

static int _handle_prepare(void* isnull, int a, char** c1, char** c2) {
    cerr << a << endl;
    return SQLITE_OK;
}

bool database::initialize_database() {
    char* errmsg;
    sqlite3_exec(m_conn.get(),
            _stmts::prepare.c_str(),
            _handle_prepare,
            nullptr,
            &errmsg);

    if (errmsg != nullptr) {
        std::string msg = errmsg;
        sqlite3_free(errmsg);

        cerr << "Could not initialize database: " << msg << endl;
        return false;
    }

    return true;
}

namespace model {

struct session_meta_t {
    std::string key;
    std::string value;
};

struct session_name_t {
    database::id_t id;
    std::string name;
};

} // namespace model

void usage(boost::program_options::options_description desc) {
    cerr << desc << endl;
}

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

    string db_file, id, get, set, value;

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
        cerr << "Id is " << id << endl;
    } else {
        cerr << "Id not set." << endl;
        return 1;
    }

    if (vm.count("get")) {
        if (vm.count("set")) {
            cerr << "--get and --set options set in the same time!" << endl;
            return 1;
        }

        get = vm["get"].as<string>();
        cerr << "Will get " << get << endl;
    }

    if (vm.count("set")) {
        assert(vm.count("get") == 0);

        set = vm["set"].as<string>();
        cerr << "Will set " << set << endl;

        if (vm.count("value")) {
            value = vm["value"].as<string>();
            cerr << "To value " << value << endl;
        }
        else {
            cerr << "--set specified, but no --value flag" << endl;
            return 1;
        }
    }

    bool result;
    database db = database::open(db_file);

    try {
        result = db.prepare();
    }
    catch (sqlite_exception& exc) {
        cerr << "got an error: " << exc.what() << endl;
        cerr << db.describe_error() << endl;
    }

    if (!result) {
        cerr << "error preparing the database" << endl;
        exit(1);
    }

    return 1;
}
