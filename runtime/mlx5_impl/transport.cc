#include "transport.h"

void post_wq_recvs(struct mlx5dv_rwq* rwq, char* buffer, unsigned int lkey, int count) {	
	struct mlx5_mprq_wqe * wqes = (struct mlx5_mprq_wqe*) rwq->buf;
	struct mlx5_wqe_data_seg * scat;
	int per_size = (1 << 9) * IBV_FRAME_SIZE;
	for (int i = 0; i < count; i++) {
		scat = &wqes[i].dseg;
		char* my_buffer = buffer + per_size * i;
		scat->addr = htobe64((uintptr_t)my_buffer);
		scat->byte_count = htobe32(per_size);
		scat->lkey = htobe32(lkey);	
	}
	*(rwq->dbrec) = htobe32(count);
}

void mlx5_try_transport(struct transport_t* tr, const char* requested_iface_name) {
	int num_devices = 0;
	struct ibv_device **dev_list = ibv_get_device_list(&num_devices);

	int resolved_dev_id = -1, resolved_dev_port_id = -1;
	struct ibv_context* resolved_context = nullptr;
	
	for (int i = 0; i < num_devices; i++) {
		struct ibv_context* ibv_ctx = ibv_open_device(dev_list[i]);
		if (ibv_ctx == nullptr) {
			std::cerr << "Failed to open device\n";
			exit(-1);
		}
		struct ibv_device_attr device_attr;
		memset(&device_attr, 0, sizeof(device_attr));
		if (ibv_query_device(ibv_ctx, &device_attr) != 0) {
			std::cerr << "Failed to query device\n";
			exit(-1);
		}
		std::string resolved_ibdev_name = std::string(ibv_ctx->device->name);
		std::string resolved_netdev_name = ibdev2netdev(resolved_ibdev_name);
		std::cout << "Trying device = " << resolved_netdev_name  << std::endl;

		if (resolved_netdev_name != requested_iface_name) continue;

		for (uint8_t port_i = 1; port_i <= device_attr.phys_port_cnt; port_i++) {
			struct ibv_port_attr port_attr;
			int e;
			if (e = ibv_query_port(ibv_ctx, port_i, &port_attr) != 0) {
				std::cerr << "Failed to query port\n";
				std::cerr << "Error = " << e << std::endl;
				exit(-1);
			}
			if (port_attr.phys_state != IBV_PORT_ACTIVE && port_attr.phys_state != IBV_PORT_ACTIVE_DEFER) {
				continue;
			}
			// Found an active port, lets go!
			resolved_dev_id = i;
			resolved_context = ibv_ctx;
			resolved_dev_port_id = port_i;	
			break;
		}
		if (resolved_dev_id != -1)
			break;
		if (ibv_close_device(ibv_ctx) != 0) {
			std::cerr << "Failed to close device\n";
			exit(-1);
		}
	
	}
	if (resolved_dev_id == -1) {
		std::cerr << "Couldn't find any active port\n";
		exit(-1);
	}
	std::string resolved_ibdev_name = std::string(resolved_context->device->name);
	std::string resolved_netdev_name = ibdev2netdev(resolved_ibdev_name);
	std::cout << "Resolved netdev name = " << resolved_netdev_name << std::endl;
	
	struct mlx5dv_context attrs_out;
	attrs_out.comp_mask = MLX5DV_CONTEXT_MASK_STRIDING_RQ;
	mlx5dv_query_device(resolved_context, &attrs_out);
	std::cout << "Attrs flags = " << attrs_out.flags << std::endl;
	std::cout << "Striding rq caps = " << attrs_out.striding_rq_caps.supported_qpts << std::endl;

	struct ibv_pd* pd = ibv_alloc_pd(resolved_context);
	rt_assert(pd != nullptr, "Failed to alloc protection domain\n");

	//struct ibv_cq* send_cq = ibv_create_cq(resolved_context, 128, nullptr, nullptr, 0);
	//rt_assert(send_cq != nullptr, "Error creating send cq");

	struct ibv_cq_init_attr_ex cq_init_attr;
	memset(&cq_init_attr, 0, sizeof(cq_init_attr));	
	cq_init_attr.cqe = 4;
	cq_init_attr.flags = IBV_CREATE_CQ_ATTR_IGNORE_OVERRUN;
	cq_init_attr.parent_domain = pd;
	cq_init_attr.comp_mask = IBV_CQ_INIT_ATTR_MASK_FLAGS;
	cq_init_attr.wc_flags = IBV_WC_STANDARD_FLAGS;
	struct ibv_cq * send_cq = ibv_cq_ex_to_cq(ibv_create_cq_ex(resolved_context, &cq_init_attr));

	//printf("error = %d\n", errno);
	rt_assert(send_cq != nullptr, "Failed to create send_cq");

	
	// this is a dummy recv_cq we create for the send qp, this will never be used
	struct ibv_cq* dummy_recv_cq = ibv_create_cq(resolved_context, 8, nullptr, nullptr, 0);
	rt_assert(dummy_recv_cq != nullptr, "Error creating dummy recv cq");

	struct ibv_qp_init_attr qp_init_attr;
	memset(&qp_init_attr, 0, sizeof(qp_init_attr));	
	qp_init_attr.send_cq = send_cq;
	qp_init_attr.recv_cq = dummy_recv_cq;
	qp_init_attr.cap.max_send_wr = 128;
	qp_init_attr.cap.max_send_sge = 2; // This might be needed to be adjusted for larger messages or for scatter gather. This is how many sges per packet. It is 2 now for header + data
	qp_init_attr.cap.max_recv_wr = 0; // this will not be receiving
	qp_init_attr.cap.max_recv_sge = 0;
	qp_init_attr.cap.max_inline_data = MAX_INLINE_DATA_SIZE; // this helps us reduce transfers over PCIE for small packets and helps reduce latency
	qp_init_attr.qp_type = IBV_QPT_RAW_PACKET; // this allows us to write arbitrary data to the network 
	qp_init_attr.sq_sig_all = 0;

	struct ibv_qp* send_qp = ibv_create_qp(pd, &qp_init_attr);
	
	rt_assert(send_qp != nullptr, "Error creating send qp");

	// We need to configure the QP we just created
	struct ibv_qp_attr qp_attr;
	memset(&qp_attr, 0, sizeof(qp_attr));
	
	qp_attr.qp_state = IBV_QPS_INIT;
	qp_attr.port_num = 1; // TODO: check if this needs to be dynamically determined
	int res = ibv_modify_qp(send_qp, &qp_attr, IBV_QP_STATE | IBV_QP_PORT);
	rt_assert(res == 0, "Error initializing QP");


	// Just make sure we can switch states without any issues
	memset(&qp_attr, 0, sizeof(qp_attr));
	qp_attr.qp_state = IBV_QPS_RTR;
	res = ibv_modify_qp(send_qp, &qp_attr, IBV_QP_STATE);
	rt_assert(res == 0, "Error setting QP state to RTR");	

	memset(&qp_attr, 0, sizeof(qp_attr));
	qp_attr.qp_state = IBV_QPS_RTS;
	res = ibv_modify_qp(send_qp, &qp_attr, IBV_QP_STATE);
	rt_assert(res == 0, "Error setting QP state to RTS");	


	// Now for the recv qp
	// Create recv_cq using the _ex API

	//struct ibv_cq* recv_cq = ibv_create_cq(resolved_context, 16, nullptr, nullptr, 0);
	//rt_assert(recv_cq != nullptr, "Failed to create recv_cq");
/*
	struct ibv_cq_init_attr_ex cq_init_attr;
	memset(&cq_init_attr, 0, sizeof(cq_init_attr));	
	cq_init_attr.cqe = 4;
	cq_init_attr.flags = IBV_CREATE_CQ_ATTR_IGNORE_OVERRUN;
	cq_init_attr.parent_domain = pd;
	cq_init_attr.comp_mask = IBV_CQ_INIT_ATTR_MASK_FLAGS;
	cq_init_attr.wc_flags = IBV_WC_STANDARD_FLAGS;
	struct ibv_cq * recv_cq = (struct ibv_cq*) ibv_create_cq_ex(resolved_context, &cq_init_attr);

	//printf("error = %d\n", errno);
	rt_assert(recv_cq != nullptr, "Failed to create recv_cq");
*/
	// Create the recv_cq also with the mlx5dv API
	//struct ibv_cq_init_attr_ex cq_init_attr;
	memset(&cq_init_attr, 0, sizeof(cq_init_attr));	
	cq_init_attr.cqe = 4;
	cq_init_attr.flags = IBV_CREATE_CQ_ATTR_IGNORE_OVERRUN;
	cq_init_attr.parent_domain = pd;
	cq_init_attr.comp_mask = IBV_CQ_INIT_ATTR_MASK_FLAGS;
	cq_init_attr.wc_flags = IBV_WC_STANDARD_FLAGS;

	struct mlx5dv_cq_init_attr mlx_cq_init_attr;
	memset(&mlx_cq_init_attr, 0, sizeof(mlx_cq_init_attr));
	mlx_cq_init_attr.comp_mask = 0;		
	//mlx_cq_init_attr.comp_mask |= MLX5DV_CQ_INIT_ATTR_MASK_COMPRESSED_CQE;
	//mlx_cq_init_attr.cqe_comp_res_format = MLX5DV_CQE_RES_FORMAT_CSUM_STRIDX;
	
	struct ibv_cq* recv_cq = ibv_cq_ex_to_cq(mlx5dv_create_cq(resolved_context, &cq_init_attr, &mlx_cq_init_attr));
	rt_assert(recv_cq != nullptr, "Failed to create recv_cq");
	
	// We will fallback to the mlx5 API because it provides more options
	// Specifically we need to be able to create striding WQEs	

	struct ibv_wq_init_attr wq_init_attr;
	memset(&wq_init_attr, 0, sizeof(wq_init_attr));
	struct mlx5dv_wq_init_attr mlx5_wq_attr;
	memset(&mlx5_wq_attr, 0, sizeof(mlx5_wq_attr));
	
	wq_init_attr.wq_type = IBV_WQT_RQ;
	wq_init_attr.max_wr = 4096 / (1 << 9);
	// No scatter when receiving packets
	wq_init_attr.max_sge = 1;
	wq_init_attr.pd = pd;
	wq_init_attr.cq = recv_cq;
	wq_init_attr.comp_mask = IBV_WQ_INIT_ATTR_FLAGS;
			
	// Now for the strided stuff	
	mlx5_wq_attr.comp_mask = MLX5DV_WQ_INIT_ATTR_MASK_STRIDING_RQ;
	//mlx5_wq_attr.comp_mask = 0;
	mlx5_wq_attr.striding_rq_attrs.single_stride_log_num_of_bytes = IBV_FRAME_SIZE_LOG;
	mlx5_wq_attr.striding_rq_attrs.single_wqe_log_num_of_strides = 9;
	
	struct ibv_wq* recv_wq = mlx5dv_create_wq(resolved_context, &wq_init_attr, &mlx5_wq_attr);
	//struct ibv_wq* recv_wq = mlx5dv_create_wq(resolved_context, &wq_init_attr, NULL);
	//struct ibv_wq* recv_wq = ibv_create_wq(resolved_context, &wq_init_attr);
	rt_assert(recv_wq != nullptr, "Failed to create recv_wq");

	// Set WQ to ready state
	struct ibv_wq_attr wq_attr;
	memset(&wq_attr, 0, sizeof(wq_attr));
	wq_attr.attr_mask = IBV_WQ_ATTR_STATE;
	wq_attr.wq_state = IBV_WQS_RDY;
	res = ibv_modify_wq(recv_wq, &wq_attr);
	rt_assert(res == 0, "Failed to set recv wq to ready state");

	struct mlx5dv_obj obj;
	struct mlx5dv_rwq rwq;
	obj.rwq.in = recv_wq;
	obj.rwq.out = &rwq;
	res = mlx5dv_init_obj(&obj, MLX5DV_OBJ_RWQ);
	rt_assert(res == 0, "Failed to initialized rwq");


	std::cout << "rwq.wqe_cnt = " << rwq.wqe_cnt << std::endl;
	std::cout << "rwq.stride = " << rwq.stride << std::endl;

	struct ibv_rwq_ind_table_init_attr rwq_ind_table_init_attr;	
	memset(&rwq_ind_table_init_attr, 0, sizeof(rwq_ind_table_init_attr));
	
	rwq_ind_table_init_attr.log_ind_tbl_size = 0; // Ignore the hash, send everything to the same wq
	rwq_ind_table_init_attr.ind_tbl = &recv_wq; // Single wq
	rwq_ind_table_init_attr.comp_mask = 0;
	
	struct ibv_rwq_ind_table *ind_tbl = ibv_create_rwq_ind_table(resolved_context, &rwq_ind_table_init_attr);
	rt_assert(ind_tbl != nullptr, "Failed to create ind table");
	
	// Now create a hash conf
	uint8_t toeplitz_key[] = {0x2c, 0xc6, 0x81, 0xd1, 0x5b, 0xdb, 0xf4, 0xf7,
		0xfc, 0xa2, 0x83, 0x19, 0xdb, 0x1a, 0x3e, 0x94,
		0x6b, 0x9e, 0x38, 0xd9, 0x2c, 0x9c, 0x03, 0xd1,
		0xad, 0x99, 0x44, 0xa7, 0xd9, 0x56, 0x3d, 0x59,
		0x06, 0x3c, 0x25, 0xf3, 0xfc, 0x1f, 0xdc, 0x2a};
	const int TOEPLITZ_RX_HASH_KEY_LEN =
		sizeof(toeplitz_key) / sizeof(toeplitz_key[0]);

	struct ibv_rx_hash_conf rx_hash_conf;
	memset(&rx_hash_conf, 0, sizeof(rx_hash_conf));
	rx_hash_conf.rx_hash_function = IBV_RX_HASH_FUNC_TOEPLITZ;
	rx_hash_conf.rx_hash_key_len = TOEPLITZ_RX_HASH_KEY_LEN;
	rx_hash_conf.rx_hash_key = toeplitz_key;
	rx_hash_conf.rx_hash_fields_mask = 0; // We are not actually using a hash function

	// Create the recv_qp
	struct ibv_qp_init_attr_ex qp_attr_ex;
	memset(&qp_attr_ex, 0, sizeof(qp_attr_ex));
	qp_attr_ex.comp_mask = IBV_QP_INIT_ATTR_RX_HASH | IBV_QP_INIT_ATTR_IND_TABLE | IBV_QP_INIT_ATTR_CREATE_FLAGS | IBV_QP_INIT_ATTR_PD;
	qp_attr_ex.create_flags = 0;
	qp_attr_ex.rx_hash_conf = rx_hash_conf;
	qp_attr_ex.qp_type = IBV_QPT_RAW_PACKET;
	qp_attr_ex.rwq_ind_tbl = ind_tbl;
	qp_attr_ex.pd = pd;

	struct ibv_qp* recv_qp = ibv_create_qp_ex(resolved_context, &qp_attr_ex);
	rt_assert(recv_qp != nullptr, "Failed to create recv_qp");

	



	// Finally create the flow rules
	//size_t rule_sz = sizeof(ibv_flow_attr) + sizeof(ibv_flow_spec_eth);
	size_t rule_sz = sizeof(ibv_flow_attr); // + sizeof(ibv_flow_spec_eth);
	uint8_t flow_rule[rule_sz];
	memset(flow_rule, 0, sizeof(flow_rule));
	uint8_t * buf = flow_rule;
	struct ibv_flow_attr* flow_attr = (struct ibv_flow_attr*) buf;
	flow_attr->type = IBV_FLOW_ATTR_NORMAL;
	flow_attr->size = rule_sz;
	flow_attr->priority = 0;
	//flow_attr->num_of_specs = 1;
	flow_attr->num_of_specs = 0;
	flow_attr->port = 1;
	flow_attr->flags = 0;

	/*
	buf += sizeof(struct ibv_flow_attr);
	
	struct ibv_flow_spec_eth* eth_spec = (struct ibv_flow_spec_eth*) buf;
	eth_spec->type = IBV_FLOW_SPEC_ETH;
	eth_spec->size = sizeof(struct ibv_flow_spec_eth);
	memset(&eth_spec->val.dst_mac, 0, sizeof(eth_spec->val.dst_mac));
	memset(&eth_spec->mask.dst_mac, 0, sizeof(eth_spec->mask.dst_mac));

	*/

	struct ibv_flow* recv_flow = ibv_create_flow(recv_qp, flow_attr);
	rt_assert(recv_flow != nullptr, "Failed to create recv flow");


	// Before we post the recvs, we have to arrange the recv_cq to be in the right "shape"
	struct mlx5_cq* mcq = to_mcq(recv_cq);
	struct mlx5_cqe64* recv_cqe_arr = (struct mlx5_cqe64*) mcq->buf_a.buf;
	rt_assert(recv_cqe_arr != nullptr, "Failed to find buffer inside recv_cq");
	int num_cqe = mcq->buf_a.length / sizeof(struct mlx5_cqe64);
	printf("Length of buffer = %d\n", num_cqe);	
	printf("CQE size = %d\n", mcq->active_cqes);	
	printf("Cons index = %d\n", mcq->cons_index);


	for (size_t i = 0; i < num_cqe; i++) {
		// Make it think that all buffers have been filled and we are about to fill up the first packet
		recv_cqe_arr[i].wqe_id = htons(UINT16_MAX);
		recv_cqe_arr[i].wqe_counter = htons(511);
		//grab_snapshot(&recv_cqe_arr[i]);
	}
	tr->recv_index_cycle = get_recv_cycle(UINT16_MAX, 511);
	tr->cqe_idx = 0;
	//grab_snapshot(&recv_cqe_arr[8]);


	size_t per_size = IBV_FRAME_SIZE * (1ull << 9);
	uint32_t lkey;
	// Extra 4096 bytes in the beginning for first packet headroom
	char* recv_buffer = alloc_shared(per_size * 8 + 4096, &lkey, pd);
	rt_assert(recv_buffer != nullptr, "Failed to allocate recv buffer");
	recv_buffer = recv_buffer + 4096;

	memset(recv_buffer, 0, per_size * 8);

	// Prepare the sges
	for (size_t i = 0; i < 8; i++) {
		char* my_buffer = recv_buffer + per_size * i;
		tr->mp_recv_sge[i].addr = (uint64_t)(uintptr_t)my_buffer;
		tr->mp_recv_sge[i].lkey = lkey;
		tr->mp_recv_sge[i].length = per_size;
		//tr->mp_recv_sge[i].length = 1024;

		tr->recv_wr[i].wr_id = i;
		tr->recv_wr[i].next = nullptr;
		tr->recv_wr[i].sg_list = &tr->mp_recv_sge[i];
		tr->recv_wr[i].num_sge = 1;

		struct ibv_recv_wr *bad_wr = nullptr;

		res = ibv_post_wq_recv(recv_wq, &tr->recv_wr[i], &bad_wr);
		//res = mlx5_post_wq_recv(recv_wq, &tr->recv_wr[i], &bad_wr);
		//res = ibv_post_recv(recv_qp, &tr->recv_wr[i], &bad_wr);
		rt_assert(res == 0, "Failed to post wq_recv");
		//printf("wr = %p, bad_wr = %p\n", &tr->recv_wr[i], bad_wr);
	}

	//post_wq_recvs(&rwq, recv_buffer, lkey, 8);
	// Finally prepare the send buffers
	size_t main_send_buffer_size = 512 * IBV_FRAME_SIZE;
	uint32_t send_lkey;
	char* send_buffer = alloc_shared(main_send_buffer_size, &send_lkey, pd);


	tr->send_buffer = send_buffer;
	tr->recv_buffer = recv_buffer;
	tr->send_qp = send_qp;
	tr->send_cq = send_cq;
	tr->recv_wq = recv_wq;
	tr->recv_cq = recv_cq;
	tr->send_key = send_lkey;
	tr->send_buffer_index = 0;
	tr->recv_buffer_index = 0;
	//tr->recv_cqe_arr = recv_cqe_arr;
	tr->recvs_to_post = 0;
	tr->sends_to_signal = 0;
	tr->recv_index_to_post = 0;
	tr->per_size = per_size;
	tr->recv_key = lkey;
	return;
}





int mlx5_try_send(transport_t* tr, char* buffer, int len) {
	struct ibv_send_wr send_wr;
	struct ibv_sge send_sgl;

	struct ibv_send_wr *wr = &send_wr;
	struct ibv_sge *sgl = &send_sgl;
	wr->num_sge = 1;
	wr->next = nullptr;
	wr->sg_list = sgl;
	wr->send_flags = 0;
	if (len < MAX_INLINE_DATA_SIZE + MLX5_ETH_INLINE_HEADER_SIZE)
		wr->send_flags |= IBV_SEND_INLINE;

	//wr.send_flags |= IBV_SEND_SIGNALED;
	wr->opcode = IBV_WR_SEND;
	
	sgl->addr = (uint64_t)buffer;
	sgl->length = len;
	sgl->lkey = tr->send_key;	
	struct ibv_send_wr* bad_wr = nullptr;
	int res = ibv_post_send(tr->send_qp, wr, &bad_wr);
	//printf("Err = %d\n", res);
	rt_assert(res == 0, "Failed to post send");

	tr->sends_to_signal++;
/*
	struct ibv_wc wc;
	while (ibv_poll_cq(tr->send_cq, 1, &wc) == 0);
*/
	

	return res;
}

const unsigned long long total_cycles = 65536 * 512;

int get_packet_count(transport_t* tr) {
	struct mlx5_cq* mcq = to_mcq(tr->recv_cq);
	struct mlx5_cqe64* recv_cqe_arr = (struct mlx5_cqe64*) mcq->buf_a.buf;

	int id = ntohs(recv_cqe_arr[tr->cqe_idx].wqe_id);
	int counter = ntohs(recv_cqe_arr[tr->cqe_idx].wqe_counter);
	
	unsigned long long new_cycle = get_recv_cycle(id, counter);
	unsigned long long diff = (new_cycle + total_cycles - tr->recv_index_cycle) % total_cycles;

	if (diff == 0 || diff > 4096) return 0;
	
	tr->cqe_idx = (tr->cqe_idx + 1) % 8;
	tr->recv_index_cycle = new_cycle;

	return diff;

}
	


int mlx5_try_recv(transport_t* tr) {
/*
	struct ibv_wc recv_wc[128];
	int ret = ibv_poll_cq(tr->recv_cq, 128, recv_wc);
	struct mlx5_cq* mcq = to_mcq(tr->recv_cq);
	struct mlx5_cqe64* recv_cqe_arr = (struct mlx5_cqe64*) mcq->buf_a.buf;
	if (ret != 0) {
		for (int i = 0; i < 8; i++) {
			printf("recv wrid = %d, %d\n", ntohs(recv_cqe_arr[i].wqe_id), ntohs(recv_cqe_arr[i].wqe_counter));
		}
	}
*/
	int ret = get_packet_count(tr);
	//if (ret != 0) printf("Received status = %s, vendor error = %d\n", ibv_wc_status_str(recv_wc[0].status), recv_wc[0].vendor_err);
	tr->recvs_to_post += ret;
	while (tr->recvs_to_post >= 512) {
		int idx = tr->recv_index_to_post;
		tr->recv_index_to_post = (tr->recv_index_to_post + 1) % 8;

		char* my_buffer = tr->recv_buffer + tr->per_size * idx;
		tr->mp_recv_sge[idx].addr = (uint64_t)(uintptr_t)my_buffer;
		tr->mp_recv_sge[idx].lkey = tr->recv_key;
		tr->mp_recv_sge[idx].length = tr->per_size;

		tr->recv_wr[idx].wr_id = idx;
		tr->recv_wr[idx].next = nullptr;
		tr->recv_wr[idx].sg_list = &tr->mp_recv_sge[idx];
		tr->recv_wr[idx].num_sge = 1;

		struct ibv_recv_wr *bad_wr = nullptr;
		int res = ibv_post_wq_recv(tr->recv_wq, &tr->recv_wr[idx], &bad_wr);
		rt_assert(res == 0, "Failed to post recv_wq");
		tr->recvs_to_post -= 512;
	}
	return ret;
}

char* mlx5_get_next_send_buffer(transport_t *tr) {
	char* buffer = tr->send_buffer + IBV_FRAME_SIZE * tr->send_buffer_index;
	tr->send_buffer_index = (tr->send_buffer_index + 1) % 512;
	return buffer;
}

char* mlx5_get_next_recv_buffer(transport_t *tr) {
	char* buffer = tr->recv_buffer + IBV_FRAME_SIZE * tr->recv_buffer_index;
	tr->recv_buffer_index = (tr->recv_buffer_index + 1) % 4096;
	return buffer;
}

