#include "halloc.h"
#include <cstdlib>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <iostream>

template <typename T>
static constexpr inline bool is_power_of_two(T x) {
  return x && ((x & T(x - 1)) == 0);
}

template <uint64_t power_of_two_number, typename T>
static constexpr inline T round_up(T x) {
  static_assert(is_power_of_two(power_of_two_number),
                "PowerOfTwoNumber must be a power of 2");
  return ((x) + T(power_of_two_number - 1)) & (~T(power_of_two_number - 1));
}


#define kHugePageSize (2 * 1024 * 1024)

char * alloc_shared(int size, int * shm_key_ret, struct ibv_pd *pd) {
	size = round_up<kHugePageSize>(size);
	int shm_key, shm_id;
	while(true) {
		shm_key = static_cast<int>(rand());
		shm_key = std::abs(shm_key);
		errno = 0;
		shm_id = shmget(shm_key, size, IPC_CREAT | IPC_EXCL | 0666 | SHM_HUGETLB);
		
		if (shm_id == -1) {
			switch(errno) {
				case EEXIST:
					continue;
				case EACCES:
				case EINVAL:
				case ENOMEM:
				default:
					rt_assert(false, "Allocation failed with shm");
			}	
		} else {
			break;
		}
	}	
	char* shm_buf = static_cast<char*> (shmat(shm_id, nullptr, 0));
	rt_assert(shm_buf != nullptr, "Allocation failed at shmat");
	

	// This makes sure the shared memory segment is destroyed when the process exits	
	shmctl(shm_id, IPC_RMID, nullptr);
	
	// The memory also needs to be bound to the numa node, but we only have one numa node

	// We will need to register the memory otherwise we will see a completion error
	struct ibv_mr *mr = ibv_reg_mr(pd, shm_buf, size, IBV_ACCESS_LOCAL_WRITE);
	rt_assert(mr != nullptr, "Failed to register memory");
	std::cout << "Registered " << size / (1024 * 1024) << " MB (lkey = " << mr->lkey << ")" << std::endl;
	
	//*shm_key_ret = shm_key;

	*shm_key_ret = mr->lkey;
	return shm_buf;

}
