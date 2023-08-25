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

template <typename T, size_t BITS = sizeof(T) * byte_size>
struct generic_integer_member: public dynamic_member {
	int m_flags = 0;
	int m_align = byte_size;
	int m_bit_size = BITS;
	T m_range_base = 0;


	generic_integer_member(int flags): m_flags(flags) {
		m_bit_size = BITS;
	}
	

	void set_custom_range(T range_base, T range_cap) {
		T range_size = range_cap - range_base;
		assert(range_size != 0 && "Range should be non zero size");
		T r = range_size - 1;
		m_bit_size = 0;
		while (r) {
			r = r >> 1;
			m_bit_size++;
		}
		m_range_base = range_base;
	}

	int get_size(void) {
		return m_bit_size;
	}
	int get_offset(void) {
		if (m_prev == nullptr)
			return 0;
		if (m_flags & (int)member_flags::aligned)
			return align_to(m_prev->get_offset() + m_prev->get_size(), m_align);
		else
			return m_prev->get_offset() + m_prev->get_size();
	}

	bytes get_addr(bytes base) {
		assert(get_offset() % byte_size == 0 && "Cannot obtain address for non aligned members");
		return base + get_offset() / byte_size;
	}
	
	unsigned long long get_value_mask(int n_bits) {
		return (1ull << (n_bits)) - 1;
	}

	// Bits lower extracts upto a single byte
	// the offsets can be anywhere in the byte range
	builder::dyn_var<unsigned char> extract_bits_lower(bytes base, int bit_start, int bit_end) {
		assert((bit_end - bit_start <= byte_size) && "Extract bits lower can extract atmost a single byte");
	
		int byte_offset = bit_start / byte_size;
		int num_bits = bit_end - bit_start;
		int bit_shift = bit_start % byte_size;

		builder::dyn_var<unsigned char*> addr = runtime::to_void_ptr(base + byte_offset);
		
		// At this point bit_shift should not be 0
		builder::dyn_var<unsigned char> value = addr[0] >> bit_shift;
		if ((bit_shift + num_bits) != byte_size) {
			return value & (unsigned char) get_value_mask(num_bits);
		} else {
			return value;
		}	
	}
	// Bits upper can extract larger number of bits
	// But bit_start % byte_size should be 0
	
	builder::dyn_var<unsigned long long> extract_bits_upper(bytes base, int bit_start, int bit_end) {
		assert((bit_start % byte_size == 0) && "Extract bits upper can only extract aligned bits");
		int num_bits = bit_end - bit_start;	
		int byte_offset = bit_start / byte_size;
		builder::dyn_var<T*> addr = runtime::to_void_ptr(base + byte_offset);
		if (num_bits == sizeof(T) * byte_size) {
			return addr[0];
		} else {
			return addr[0] & get_value_mask(num_bits);
		}
	}

	// Similar to extract_bits_lower can only set sizes < byte_size, but any offset
	void set_bits_lower(bytes base, int bit_start, int bit_end, builder::dyn_var<unsigned char> val) {
		assert((bit_end - bit_start <= byte_size) && "Set bits lower can set atmost a single byte");	
		int byte_offset = bit_start / byte_size;
		int num_bits = bit_end = bit_start;
		int bit_shift = bit_start % byte_size;
		builder::dyn_var<unsigned char*> addr = runtime::to_void_ptr(base + byte_offset);
		// clear the bits	
		addr[0] = ~((unsigned char)get_value_mask(num_bits) << bit_shift) & addr[0];
		addr[0] = addr[0] | (val << bit_shift);
	}
	// Similar to extract_bits_upper can only set aligned bits
	void set_bits_upper(bytes base, int bit_start, int bit_end, builder::dyn_var<unsigned long long> val) {
		assert((bit_start % byte_size == 0) && "Set bits upper can only set aligned bits");
		int byte_offset = bit_start / byte_size;
		int num_bits = bit_end - bit_start;
		builder::dyn_var<T*> addr = runtime::to_void_ptr(base + byte_offset);
		if (num_bits == sizeof(T) * byte_size) {
			addr[0] = val;
		} else {
			addr[0] = (addr[0] & ~((T)get_value_mask(num_bits))) | val;
		}
	}

	builder::dyn_var<unsigned long long> get_integer(bytes base) {
		builder::dyn_var<unsigned long long> value;
		int bit_start = get_offset();
		int bit_end = bit_start + m_bit_size;
		if (bit_start % 8 != 0) {
			int lbs = bit_start;
			int lb = byte_size - lbs % byte_size;
			if (lb > m_bit_size) lb = m_bit_size;

			int lbe = lbs + lb;
			int ubs = lbe;
			int ube = bit_end;
			if (ube - ubs != 0) 
				value = extract_bits_lower(base, lbs, lbe) | extract_bits_upper(base, ubs, ube) << lb;
			else 
				value = extract_bits_lower(base, lbs, lbe);
		} else {
			value = extract_bits_upper(base, bit_start, bit_end);
		}

		if (m_range_base != 0) 
			return value + m_range_base;
		else 
			return value;		
	}
	void set_integer(bytes base, builder::dyn_var<unsigned long long> val) {
		if (m_range_base != 0) 
			val = val - m_range_base;

		int bit_start = get_offset();
		int bit_end = bit_start + m_bit_size;
		if (bit_start % 8 != 0) {
			int lbs = bit_start;
			int lb = byte_size - lbs % byte_size;
			if (lb > m_bit_size) lb = m_bit_size;

			int lbe = lbs + lb;
			int ubs = lbe;
			int ube = bit_end;

			set_bits_lower(base, lbs, lbe, val & get_value_mask(lb));
			if (ube - ubs != 0) {
				set_bits_upper(base, ubs, ube, val >> lb);
			}
		} else {
			set_bits_upper(base, bit_start, bit_end, val);
		}
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
