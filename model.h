#pragma once
#include <string>

namespace model {

using entity_id = int;

struct session_meta_t {
    entity_id id;
	std::string key;
	std::string value;
};

struct session_name_t {
	entity_id id;
	std::string name;
};

} // namespace model
