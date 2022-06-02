#ifndef NET_BLOCKS_PACKET_H
#define NET_BLOCKS_PACKET_H
#include "builder/dyn_var.h"
#include "core/runtime.h"
#include <assert.h>
#include <map>

#define MTU_SIZE (1024)

namespace net_blocks {
using packet_t = builder::dyn_var<unsigned char*>;
using bytes = builder::dyn_var<unsigned char*>;

static const int byte_size = 8;
static int align_to(int v, int a = byte_size) {
	return ((v + a - 1) / a) * a;
}

struct dynamic_layout;

// Abstract class for members in programmable layouts
struct dynamic_member {
	
	// Members that are set while registering members
	int m_order;
	struct dynamic_layout* m_parent = nullptr;
	// Quick helper to get prev and next member
	struct dynamic_member * m_prev = nullptr;
	struct dynamic_member * m_next = nullptr;
	
	// Enable dynamic inheritance
	virtual ~dynamic_member();
	virtual int get_offset(void) = 0;
	virtual int get_size(void) = 0;
	virtual bytes get_addr(bytes) = 0;
	virtual void set_integer(bytes, builder::dyn_var<unsigned long long>);
	virtual builder::dyn_var<unsigned long long> get_integer(bytes);
};

enum class member_flags {
	aligned = 1
};

template <typename T>
struct generic_integer_member: public dynamic_member {
	int m_flags = 0;

	generic_integer_member(int flags): m_flags(flags) {}
	
	int get_size(void) {
		return sizeof(T) * byte_size;	
	}
	int get_offset(void) {
		if (m_prev == nullptr)
			return 0;
		if (m_flags & (int)member_flags::aligned)
			return align_to(m_prev->get_offset() + m_prev->get_size(), byte_size);
		else
			return m_prev->get_offset() + m_prev->get_size();
	}

	bytes get_addr(bytes base) {
		assert(get_offset() % byte_size == 0 && "Cannot obtain address for non aligned members");
		return base + get_offset() / byte_size;
	}
	
	builder::dyn_var<unsigned long long> get_integer(bytes base) {
		assert(get_offset() % byte_size == 0 && "Currently not supporting unaligned accesses");	
		builder::dyn_var<T*> addr = runtime::to_void_ptr(get_addr(base));
		return addr[0];
	}	
	
	void set_integer(bytes base, builder::dyn_var<unsigned long long> val) {
		assert(get_offset() % byte_size == 0 && "Currently not supporting unaligned accesses");	
		builder::dyn_var<T*> addr = runtime::to_void_ptr(get_addr(base));
		addr[0] = val;
	}	
};

template <int N>
struct byte_array_member: public dynamic_member {
	int m_flags = 0;

	byte_array_member(int flags): m_flags(flags) {}

	int get_size(void) {
		return N * byte_size;
	}
	int get_offset(void) {
		if (m_prev == nullptr)
			return 0;
		if (m_flags & (int) member_flags::aligned) 
			return align_to(m_prev->get_offset() + m_prev->get_size(), byte_size);
		else
			return m_prev->get_offset() + m_prev->get_size();
	}
	bytes get_addr(bytes base) {
		assert(get_offset() % byte_size == 0 && "Cannot obtain address for non aligned members");
		return base + get_offset() / byte_size;
	}
};

struct dynamic_layout {
	int m_total_members = 0;
	
	std::vector<std::vector<std::string>> m_registered_members;
	std::map<std::string, dynamic_member*> m_members_map;
		
	void add_member(std::string name, dynamic_member* m, int group = 0);
	dynamic_member* operator[] (std::string name);
	
	int get_total_size(void);
	void fix_layout(void);
	void print_layout(std::ostream& oss);			

	int group_start_offset(int i) {
		auto m = m_members_map[m_registered_members[i][0]];
		return m->get_offset() / byte_size;
	}
};

}

#endif
