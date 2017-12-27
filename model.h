#pragma once
#include <string>

namespace model {

using entity_id = int;

struct session_meta_t {
	std::string key;
	std::string value;
};

struct session_name_t {
	entity_id id;
	std::string name;
};

struct session_data_t {
    entity_id session_id;
    std::string name;
    std::string value;
};

} // namespace model
