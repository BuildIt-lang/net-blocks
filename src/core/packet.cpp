#include "core/packet.h"
#include "builder/static_var.h"
#include <assert.h>
namespace net_blocks {

	dynamic_member::~dynamic_member() {}

	void dynamic_layout::add_member(std::string name, dynamic_member* m) {
		m->m_order = m_total_members;
		m->m_parent = this;

		if (m_total_members > 0) {
			m->m_prev = m_members_map[m_registered_members[m_total_members-1]];
			m->m_prev->m_next = m;
		} else
			m->m_prev = nullptr;

		m->m_next = nullptr;
		m_registered_members.push_back(name);
		m_members_map[name] = m;
		m_total_members++;
	}
	dynamic_member* dynamic_layout::operator[] (std::string name) {
		return m_members_map[name];
	}
	
	int dynamic_layout::get_total_size(void) {
		dynamic_member* last = m_members_map[m_registered_members[m_total_members-1]];
		return align_to(last->get_offset() + last->get_size(), byte_size) / byte_size;
	}
		
}
