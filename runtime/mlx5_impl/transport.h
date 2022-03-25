#ifndef _TRANSPORT_H
#define _TRANSPORT_H

#include <infiniband/verbs.h>
#include <iostream>
#include "halloc.h"
#include <arpa/inet.h>
#include "mlx5_defs.h"
//#include "mlx5.h"

//#define IBV_FRAME_SIZE_LOG (13ull)
#define IBV_FRAME_SIZE_LOG (10ull)
#define IBV_FRAME_SIZE (1ull << IBV_FRAME_SIZE_LOG)
#define MAX_INLINE_DATA_SIZE (972)
struct msgbuffer {
	char* buffer;
	int length;
	int lkey;
};

struct cqe_snapshot_t {
	int wqe_id;
	int wqe_counter;
};

static inline cqe_snapshot_t grab_snapshot(volatile mlx5_cqe64* cqe_ref) {
	// Since these DMAs can happen anytime, we need to grad a consistent snapshot
	cqe_snapshot_t to_ret, another;
	while (true) {
		to_ret.wqe_id = cqe_ref->wqe_id;
		to_ret.wqe_counter = cqe_ref->wqe_counter;
		
		memory_barrier();
		another.wqe_id = cqe_ref->wqe_id;
		another.wqe_counter = cqe_ref->wqe_counter;
		
		if (to_ret.wqe_id == another.wqe_id && to_ret.wqe_counter == another.wqe_counter) {
			to_ret.wqe_id = ntohs(to_ret.wqe_id);
			to_ret.wqe_counter = ntohs(to_ret.wqe_counter);
			return to_ret;
		}
	}
}

class Transport {	
public:
	struct ibv_context* resolved_context = nullptr;
	int resolved_dev_id = -1;
	int resolved_dev_port_id = -1;
	unsigned char resolved_mac[6];

	struct ibv_pd* pd;
	struct ibv_cq* send_cq;
	struct ibv_cq* recv_cq;
	struct ibv_cq* recv_cq_simple;
	struct ibv_qp* qp;

	struct ibv_exp_wq *wq;
	struct ibv_exp_wq_family *wq_family;
	struct ibv_exp_rwq_ind_table *ind_tbl;
	struct ibv_qp *mp_recv_qp;
	struct ibv_exp_flow *recv_flow;	
	
	struct ibv_sge mp_recv_sge[8];

	// Instead of using an array of send_wr's we will allocate send_wr locally	
	//struct ibv_send_wr send_wr[512 + 1];
	//struct ibv_sge send_sgl[512][2]; // We will use 2 sges per wr IF required

// This needs to be passed to the driver
public:
	volatile mlx5_cqe64 *recv_cqe_arr = nullptr;	
	int cqe_idx = 0;
	cqe_snapshot_t prev_snapshot;	
	// We will need a lot of prepared send_buffers
	struct msgbuffer send_buffers[512];
	unsigned send_buffer_index;
	char* main_send_buffer;
	int send_lkey;

	unsigned char * recv_buffer;

	int recv_idx = 0;
	int recvs_to_post = 0;
	int recv_sge_idx = 0;
	
	Transport(short udp_port = 0);
	void send_message(struct msgbuffer*);
	int try_recv(void);
	int try_recv_old(void);
};
#endif
