#ifndef TRANSPORT_MLX5_H
#define TRANSPORT_MLX5_H

#include <infiniband/verbs.h>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include "mlx5.h"
#include "utils.h"
#include "halloc.h"

#include <infiniband/mlx5dv.h>
// This has been found by trial and error for our devices
// In the future this can be figured out at transport creation time
#define MAX_INLINE_DATA_SIZE (972)
#define IBV_FRAME_SIZE_LOG (10ull)
#define IBV_FRAME_SIZE (1ull << IBV_FRAME_SIZE_LOG)

#define MLX5_ETH_INLINE_HEADER_SIZE (18)

#ifdef __cplusplus 
extern "C" {
#endif 

struct transport_t {
	char* send_buffer;
	char* recv_buffer;

	struct ibv_sge mp_recv_sge[8];
	struct ibv_recv_wr recv_wr[8];

	struct ibv_qp *send_qp;
	struct ibv_cq* send_cq;

	struct ibv_wq *recv_wq;
	struct ibv_cq* recv_cq;

	uint32_t send_key;
	uint32_t recv_key;
	size_t per_size;

	int send_buffer_index;		
	int recv_buffer_index;

	struct mlx5_cqe64* recv_cqe_arr;
	unsigned cqe_idx;
	unsigned long long recv_index_cycle;

	int recv_index_to_post;
	int recvs_to_post;
	int sends_to_signal;
};

static unsigned long long get_recv_cycle(int id, int counter) {
	return id * 512 + counter;	
}


void mlx5_try_transport(struct transport_t*, const char* name);
char* mlx5_get_next_send_buffer(transport_t *tr);
char* mlx5_get_next_recv_buffer(transport_t *tr);
int mlx5_try_recv(transport_t* tr);
int mlx5_try_send(transport_t* tr, char* buffer, int len);

#ifdef __cplusplus 
}
#endif

#endif
