#include "core/packet.h"
#include "builder/static_var.h"
#include <assert.h>
namespace net_blocks {

dynamic_member::~dynamic_member() {}

void dynamic_layout::add_member(std::string name, dynamic_member* m, int group) {
	m->m_order = m_total_members;
	m->m_parent = this;

	m->m_next = nullptr;
	for (size_t g = m_registered_members.size(); g <= (size_t) group; g++) {
		m_registered_members.emplace_back();
	}
	m_registered_members[group].push_back(name);
	m_members_map[name] = m;
	m_total_members++;
}

void dynamic_layout::fix_layout(void) {
	dynamic_member * last = nullptr;
	for (size_t g = 0; g < m_registered_members.size(); g++) {
		for (size_t m = 0; m < m_registered_members[g].size(); m++) {
			std::string m_name = m_registered_members[g][m];
			m_members_map[m_name]->m_prev = last;
			last = m_members_map[m_name];
		}
	}
}
void dynamic_layout::print_layout(std::ostream &oss) {
	for (size_t g = 0; g < m_registered_members.size(); g++) {
		for (size_t m = 0; m < m_registered_members[g].size(); m++) {
			std::string m_name = m_registered_members[g][m];
			oss << m_name << "\t:" << m_members_map[m_name]->get_size() << std::endl;
		}
		oss << "----------------" << std::endl;
	}
}
dynamic_member* dynamic_layout::operator[] (std::string name) {
	assert(m_members_map[name] != nullptr && "Requested member not found in layout");
	return m_members_map[name];
}

int dynamic_layout::get_total_size(void) {
	dynamic_member* last = m_members_map[m_registered_members.back().back()];
	return align_to(last->get_offset() + last->get_size(), byte_size) / byte_size;
}

void dynamic_member::set_integer(bytes, builder::dyn_var<unsigned long long>) {
	assert(false && "Set integer not implemented for dynamic member type\n");
}
builder::dyn_var<unsigned long long> dynamic_member::get_integer(bytes) {
	assert(false && "Get integer not implemented for dynamic member type\n");
	return 0;
}

}
