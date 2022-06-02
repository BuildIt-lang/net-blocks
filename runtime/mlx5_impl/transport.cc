#include "transport.h"
#include "mlx5_defs.h"
Transport::Transport(short udp_port) {
	int num_devices = 0;
	struct ibv_device **dev_list = ibv_get_device_list(&num_devices);
	
	for (int i = 0; i < num_devices; i++) {
		struct ibv_context * ib_ctx = ibv_open_device(dev_list[i]);
		if (ib_ctx == nullptr) {
			std::cerr << "Failed to open device\n"; 
			exit(-1);
		}
		struct ibv_device_attr device_attr;
		memset(&device_attr, 0, sizeof(device_attr));
		if (ibv_query_device(ib_ctx, &device_attr) != 0) {
			std::cerr << "Failed to query device\n";
			exit(-1);
		}
		for (uint8_t port_i = 1; port_i <= device_attr.phys_port_cnt; port_i++) {
			struct ibv_port_attr port_attr;
			if (ibv_query_port(ib_ctx, port_i, &port_attr) != 0) {
				std::cerr << "Failed to query port\n";
				exit(-1);
			}
			if (port_attr.phys_state != IBV_PORT_ACTIVE && port_attr.phys_state != IBV_PORT_ACTIVE_DEFER) {
				continue;
			}
			// Found an active port, lets go!
			resolved_dev_id = i;
			resolved_context = ib_ctx;
			resolved_dev_port_id = port_i;
			
			std::cout << "Found an active port on " << ib_ctx->device->name << ": (" << (int)port_i << ")" 
				<< std::endl;
		}	
		if (resolved_dev_id != -1) {
			break;	
		}
		if (ibv_close_device(ib_ctx)  != 0) {
			std::cerr << "Failed to close device\n";
			exit(-1);
		}
	}
	
	if (resolved_dev_id == -1) {
		std::cerr << "Couldn't find any active ports\n";
		exit(-1);
	}	


	std::string resolved_ibdev_name = std::string(resolved_context->device->name);
	std::string resolved_netdev_name = ibdev2netdev(resolved_ibdev_name);
	fill_interface_mac(resolved_netdev_name, resolved_mac);
	
	std::cout << "Resolved netdev name = " << resolved_netdev_name << std::endl;
	printf("Resolved Mac addr = %02x:%02x:%02x:%02x:%02x:%02x\n", resolved_mac[0], resolved_mac[1], resolved_mac[2], resolved_mac[3], resolved_mac[4], resolved_mac[5]);


	pd = ibv_alloc_pd(resolved_context);
	if (pd == nullptr) {
		std::cerr << "Failed to alloc protection domain\n";
		exit(-1);
	}
	// Init basic QP
	struct ibv_exp_cq_init_attr cq_init_attr;
	memset(&cq_init_attr, 0, sizeof(cq_init_attr));
	send_cq = ibv_exp_create_cq(resolved_context, 128, nullptr, nullptr, 0, &cq_init_attr);
		
	rt_assert(send_cq != nullptr, "Error creating CQ");	
	// This recv_cq_simple is a dummy recv_cq we create for the send qp. This will never be used
	recv_cq_simple = ibv_exp_create_cq(resolved_context, 8, nullptr, nullptr, 0, &cq_init_attr);
	rt_assert(recv_cq_simple != nullptr, "Error creating recv cq");
	
	// We don't need a RECV CQ, create a QP now	
	struct ibv_exp_qp_init_attr qp_init_attr;
	memset(&qp_init_attr, 0,  sizeof(qp_init_attr));
	qp_init_attr.comp_mask = IBV_EXP_QP_INIT_ATTR_PD | IBV_EXP_QP_INIT_ATTR_CREATE_FLAGS;
	qp_init_attr.pd = pd;
	qp_init_attr.send_cq = send_cq;
	//qp_init_attr.recv_cq = send_cq;
	qp_init_attr.recv_cq = recv_cq_simple;
	qp_init_attr.cap.max_send_wr = 128;
	qp_init_attr.cap.max_send_sge = 2; // This might need to adjusted for larger messages or for scatter/gather
	qp_init_attr.cap.max_recv_wr = 0;
	qp_init_attr.cap.max_recv_sge = 0;
	qp_init_attr.cap.max_inline_data = MAX_INLINE_DATA_SIZE;
	qp_init_attr.exp_create_flags = IBV_EXP_QP_CREATE_IGNORE_SQ_OVERFLOW;
	qp_init_attr.qp_type = IBV_QPT_RAW_PACKET; // This QP type allows us to write arbitrary byts to the Network
	
	qp = ibv_exp_create_qp(resolved_context, &qp_init_attr);
	
	rt_assert(qp != nullptr, "Error creating QP");
	
	// We need to configure the just created QP
	struct ibv_exp_qp_attr qp_attr;
	memset(&qp_attr, 0, sizeof(qp_attr));
	qp_attr.qp_state = IBV_QPS_INIT;
	qp_attr.port_num = 1; // This might need to be changed if using a differnet port
	int res = ibv_exp_modify_qp(qp, &qp_attr, IBV_QP_STATE | IBV_QP_PORT);
	rt_assert(res == 0, "Error initializing QP");
	

	// Just make  sure the qp can switch states without any issues
	memset(&qp_attr, 0, sizeof(qp_attr));
	qp_attr.qp_state = IBV_QPS_RTR;
	res = ibv_exp_modify_qp(qp, &qp_attr, IBV_QP_STATE);
	rt_assert(res == 0, "Error initializing QP");

	memset(&qp_attr, 0, sizeof(qp_attr));
	qp_attr.qp_state = IBV_QPS_RTS;
	res = ibv_exp_modify_qp(qp, &qp_attr, IBV_QP_STATE);
	rt_assert(res == 0, "Error initializing QP");

	
	// Let us allocate the Recv QPs
	//struct ibv_exp_cq_init_attr cq_init_attr;
	memset(&cq_init_attr, 0, sizeof(cq_init_attr));
	recv_cq = ibv_exp_create_cq(resolved_context, 4, nullptr, nullptr, 0, &cq_init_attr);
	rt_assert(recv_cq != nullptr, "Failed to allocate recv cq");

	struct ibv_exp_cq_attr cq_attr;
	memset(&cq_attr, 0, sizeof(cq_attr));
	cq_attr.comp_mask = IBV_EXP_CQ_ATTR_CQ_CAP_FLAGS;
	cq_attr.cq_cap_flags = IBV_EXP_CQ_IGNORE_OVERRUN;
	rt_assert(ibv_exp_modify_cq(recv_cq, &cq_attr, IBV_EXP_CQ_CAP_FLAGS) == 0, "Failed to modify cq flags");
	
	struct ibv_exp_wq_init_attr wq_init_attr;
	memset(&wq_init_attr, 0, sizeof(wq_init_attr));
	
	wq_init_attr.wq_type = IBV_EXP_WQT_RQ;
	wq_init_attr.max_recv_wr = 4096 / (1 << 9);
	wq_init_attr.max_recv_sge = 1;
	wq_init_attr.pd = pd;
	wq_init_attr.cq = recv_cq;
	
	wq_init_attr.comp_mask |= IBV_EXP_CREATE_WQ_MP_RQ;
	wq_init_attr.mp_rq.use_shift = IBV_EXP_MP_RQ_NO_SHIFT;
	wq_init_attr.mp_rq.single_wqe_log_num_of_strides = 9;
	//wq_init_attr.mp_rq.single_stride_log_num_of_bytes = 10;
	wq_init_attr.mp_rq.single_stride_log_num_of_bytes = IBV_FRAME_SIZE_LOG;
	wq = ibv_exp_create_wq(resolved_context, &wq_init_attr);
	rt_assert(wq != nullptr, "Failed to allocate wq");

	// Set WQ to ready state
	struct ibv_exp_wq_attr wq_attr;
	memset(&wq_attr, 0, sizeof(wq_attr));
	wq_attr.attr_mask = IBV_EXP_WQ_ATTR_STATE;
	wq_attr.wq_state = IBV_EXP_WQS_RDY;
	rt_assert(ibv_exp_modify_wq(wq, &wq_attr) == 0, "Failed to set wq state to ready");
	
	enum ibv_exp_query_intf_status intf_status = IBV_EXP_INTF_STAT_OK;
	struct ibv_exp_query_intf_params query_intf_params;
	memset(&query_intf_params, 0, sizeof(query_intf_params));
	query_intf_params.intf_scope = IBV_EXP_INTF_GLOBAL;
	query_intf_params.intf = IBV_EXP_INTF_WQ;
	query_intf_params.obj = wq;
	wq_family = reinterpret_cast<struct ibv_exp_wq_family *>(
			ibv_exp_query_intf(resolved_context, &query_intf_params, &intf_status));
	rt_assert(wq_family != nullptr, "Failed to get WQ interface");	


	// Create indirect table
	// Not sure what the indirect table or the hash_conf is for??
	struct ibv_exp_rwq_ind_table_init_attr rwq_ind_table_init_attr;
	memset(&rwq_ind_table_init_attr, 0, sizeof(rwq_ind_table_init_attr));
	rwq_ind_table_init_attr.pd = pd;
	rwq_ind_table_init_attr.log_ind_tbl_size = 0;  // Ignore hash
	rwq_ind_table_init_attr.ind_tbl = &wq;         // Pointer to RECV work queue
	rwq_ind_table_init_attr.comp_mask = 0;
	ind_tbl =
		ibv_exp_create_rwq_ind_table(resolved_context, &rwq_ind_table_init_attr);
	rt_assert(ind_tbl != nullptr, "Failed to create indirection table");

	// Create rx_hash_conf and indirection table for the QP
	uint8_t toeplitz_key[] = {0x6d, 0x5a, 0x56, 0xda, 0x25, 0x5b, 0x0e, 0xc2,
		0x41, 0x67, 0x25, 0x3d, 0x43, 0xa3, 0x8f, 0xb0,
		0xd0, 0xca, 0x2b, 0xcb, 0xae, 0x7b, 0x30, 0xb4,
		0x77, 0xcb, 0x2d, 0xa3, 0x80, 0x30, 0xf2, 0x0c,
		0x6a, 0x42, 0xb7, 0x3b, 0xbe, 0xac, 0x01, 0xfa};
	const int TOEPLITZ_RX_HASH_KEY_LEN =
		sizeof(toeplitz_key) / sizeof(toeplitz_key[0]);

	struct ibv_exp_rx_hash_conf rx_hash_conf;
	memset(&rx_hash_conf, 0, sizeof(rx_hash_conf));
	rx_hash_conf.rx_hash_function = IBV_EXP_RX_HASH_FUNC_TOEPLITZ;
	rx_hash_conf.rx_hash_key_len = TOEPLITZ_RX_HASH_KEY_LEN;
	rx_hash_conf.rx_hash_key = toeplitz_key;
	rx_hash_conf.rx_hash_fields_mask = 0; //IBV_EXP_RX_HASH_DST_PORT_UDP;
	rx_hash_conf.rwq_ind_tbl = ind_tbl;	



	//struct ibv_exp_qp_init_attr qp_init_attr;
	memset(&qp_init_attr, 0, sizeof(qp_init_attr));
	qp_init_attr.comp_mask = IBV_EXP_QP_INIT_ATTR_CREATE_FLAGS | IBV_EXP_QP_INIT_ATTR_PD | IBV_EXP_QP_INIT_ATTR_RX_HASH;
	
	qp_init_attr.rx_hash_conf = &rx_hash_conf;
	qp_init_attr.pd = pd;
	qp_init_attr.qp_type = IBV_QPT_RAW_PACKET;
	
	mp_recv_qp = ibv_exp_create_qp(resolved_context, &qp_init_attr);
	rt_assert(mp_recv_qp != nullptr, "Failed to create receive QP");
	
	
	// Finally we will create a flow rule for this QP	
	// We will create a simple flow rule that accepts all packets on the mac

	size_t rule_sz;
	if (udp_port == 0) {
		rule_sz = sizeof(ibv_exp_flow_attr) + sizeof(ibv_exp_flow_spec_eth);
	} else {
		rule_sz = sizeof(ibv_exp_flow_attr) + sizeof(ibv_exp_flow_spec_eth) + sizeof(ibv_exp_flow_spec_ipv4_ext) + sizeof(ibv_exp_flow_spec_tcp_udp);
	}
	//constexpr size_t rule_sz = sizeof(ibv_exp_flow_attr) + sizeof(ibv_exp_flow_spec_eth);
	uint8_t flow_rule[rule_sz];
	memset(flow_rule, 0, rule_sz);
	uint8_t * buf = flow_rule;
	
	auto *flow_attr = reinterpret_cast<struct ibv_exp_flow_attr*> (flow_rule);
	flow_attr->type = IBV_EXP_FLOW_ATTR_NORMAL;
	flow_attr->size = rule_sz;
	flow_attr->priority = 0;
	if (udp_port == 0) {
		flow_attr->num_of_specs = 1;
	} else {
		flow_attr->num_of_specs = 3;
	}
	flow_attr->port = 1;
	flow_attr->flags = 0;
	flow_attr->reserved = 0;
	buf += sizeof(struct ibv_exp_flow_attr);
	
	auto *eth_spec = reinterpret_cast<struct ibv_exp_flow_spec_eth*>(buf);
	eth_spec->type = IBV_EXP_FLOW_SPEC_ETH;
	eth_spec->size = sizeof(struct ibv_exp_flow_spec_eth);
	memcpy(&eth_spec->val.dst_mac, resolved_mac, sizeof(resolved_mac));
	memset(&eth_spec->mask.dst_mac, 0xff, sizeof(resolved_mac));
	buf += sizeof(struct ibv_exp_flow_spec_eth);

	if (udp_port != 0) {

		auto *spec_ipv4 = reinterpret_cast<struct ibv_exp_flow_spec_ipv4_ext*>(buf);
		spec_ipv4->type = IBV_EXP_FLOW_SPEC_IPV4_EXT;
		spec_ipv4->size = sizeof(struct ibv_exp_flow_spec_ipv4_ext);
		buf += sizeof(struct ibv_exp_flow_spec_ipv4_ext);

		auto udp_spec = reinterpret_cast<struct ibv_exp_flow_spec_tcp_udp*>(buf);
		udp_spec->type = IBV_EXP_FLOW_SPEC_UDP;
		udp_spec->size = sizeof(struct ibv_exp_flow_spec_tcp_udp);
		udp_spec->val.dst_port = htons(udp_port);
		udp_spec->mask.dst_port = 0xffffu;
	} else 
		std::cout << "Creating flow without IP\n";



	recv_flow = ibv_exp_create_flow(mp_recv_qp, flow_attr);
	//recv_flow = ibv_exp_create_flow(qp, flow_attr);
	rt_assert(recv_flow != nullptr, "Failed to create flow");		

		
	auto *_mlx5_cq = reinterpret_cast<mlx5_cq*>(recv_cq);
	rt_assert(_mlx5_cq->buf_a.buf != nullptr, "Couldn't find buf inside recv cq");
	
	recv_cqe_arr = reinterpret_cast<volatile mlx5_cqe64*>(_mlx5_cq->buf_a.buf);

	for (size_t i = 0; i < 8; i++) {
		recv_cqe_arr[i].wqe_id = htons(UINT16_MAX);
		recv_cqe_arr[i].wqe_counter = htons((1ull << 9) - 8 + i);
	}
	prev_snapshot = grab_snapshot(&recv_cqe_arr[7]);
	
	size_t per_size = (1ull << IBV_FRAME_SIZE_LOG) * (1ull << 9);
	int lkey;
	// We allocate extra 4k in the beginning	
	// for first packet headroom
	recv_buffer = (unsigned char*) alloc_shared(per_size * 8 + 4096, &lkey, pd);
	recv_buffer = recv_buffer + 4096;
	rt_assert(recv_buffer != nullptr, "Failed to allocate recv buffer");
	memset(recv_buffer, 0, per_size * 8);
	

	for (size_t  i = 0; i < 8; i++) {
		unsigned char* my_buffer = recv_buffer + per_size * i;
		mp_recv_sge[i].addr = reinterpret_cast<uint64_t>(my_buffer);
		mp_recv_sge[i].lkey = lkey;	
		mp_recv_sge[i].length = per_size;
		wq_family->recv_burst(wq, &mp_recv_sge[i], 1);
	}
/*	
	for (size_t i = 0; i < 32; i++) {
		send_wr[i].next = &send_wr[i+1];
		send_wr[i].opcode = IBV_WR_SEND;
		send_wr[i].sg_list = &send_sgl[i][0];
	}
*/
	

	// Prepare the send buffers with fixed size
	//unsigned long long main_send_buffer_size = 512 * 1024;
	unsigned long long main_send_buffer_size = 512 * IBV_FRAME_SIZE;

	char* send_buffer = alloc_shared(main_send_buffer_size, &lkey, pd);
	main_send_buffer = send_buffer;
	send_buffer_index = 0;
	rt_assert(send_buffer != nullptr, "Failed to allocated send buffer");
	send_lkey = lkey;
	for (int i = 0; i < 512; i++) {
		//char* this_buffer = send_buffer + i * 1024;
		char* this_buffer = send_buffer + i * IBV_FRAME_SIZE;
		send_buffers[i].buffer = this_buffer;
		send_buffers[i].length = 0;
		send_buffers[i].lkey = lkey;
	}

	
}
void Transport::send_message(struct msgbuffer* send_buffer) {
	// Prepare one wr for send
	//struct ibv_send_wr &wr = send_wr[0];
	struct ibv_send_wr wr;
	struct ibv_sge sgl;
	wr.num_sge = 1;
	wr.next = nullptr;
	wr.sg_list = &sgl;
	wr.send_flags = 0;
	if (send_buffer->length < MAX_INLINE_DATA_SIZE + MLX5_ETH_INLINE_HEADER_SIZE)
		wr.send_flags |= IBV_SEND_INLINE;
	wr.opcode = IBV_WR_SEND;
	
	sgl.addr = reinterpret_cast<uint64_t>(send_buffer->buffer);
	sgl.length = send_buffer->length;
	sgl.lkey = send_buffer->lkey;

	//struct ibv_sge *sgl = &(send_buffer->sge_list[0]);
	
	//wr.send_flags = 0;
	//wr.send_flags = IBV_SEND_SIGNALED;
	// These fields are already set, except length
	//sgl[0].addr = reinterpret_cast<uint64_t>(send_buffer->buffer);
	//sgl[0].length = send_buffer->length;
	//sgl[0].lkey = send_buffer->lkey;
	
	
//#define DEBUG
#ifdef DEBUG
	std::cout << "About to send" << std::endl;
	std::cout << "Size = " << sgl.length << std::endl;
	char * ptr = reinterpret_cast<char*>(sgl.addr);
	for (size_t i = 0; i < sgl.length; i++) {
		printf("%02x ", static_cast<unsigned char>(ptr[i]));
	}
	std::cout << std::endl;
	std::cout << "-------------" << std::endl;
#endif

	struct ibv_send_wr* bad_wr = nullptr;	
	int res = ibv_post_send(qp, &wr, &bad_wr);
	if( res != 0) {
		printf("IBV post send failed with %d\n", res);
	}	
	rt_assert(res == 0, "ibv_post_send_failed");
	
	// Wrs can be immediately reused or deleted. 	
	//send_wr[0].next = &send_wr[1];
	//poll_cq_one_helper(send_cq);
#ifdef USE_SIGNALED
	size_t num_tries = 0;
	struct ibv_wc wc;
	while (ibv_poll_cq(send_cq, 1, &wc) == 0) {
		num_tries++;	
	}
	std::cout << "SENDING COMPLETED after tries = " << num_tries << std::endl;
	std::cout << "WC.status = "  << wc.status << std::endl;	
	std::cout << "bad wr = " << bad_wr << std::endl;
#endif	
}


static inline int idx_from_snap(cqe_snapshot_t &snap) {
	return snap.wqe_id * 512 + snap.wqe_counter;
}
static inline int packets_from_snaps(cqe_snapshot_t &prev, cqe_snapshot_t &curr) {
	int idx1 = idx_from_snap(prev);
	int idx2 = idx_from_snap(curr);
	
	return (idx2 + 65536ull * 512 - idx1) % (65536ull * 512);
}

int Transport::try_recv(void) {
	cqe_snapshot_t current;
	current = grab_snapshot(&recv_cqe_arr[cqe_idx]);
	int delta = packets_from_snaps(prev_snapshot, current);
	if (delta == 0 || delta > 4096)
		return 0;
	cqe_idx = (cqe_idx + 1) % 8;
	prev_snapshot = current;
	return delta;
}
int Transport::try_recv_old(void) {
	struct ibv_wc recv_wc[16];
	int ret = ibv_poll_cq(recv_cq, 16, recv_wc);
	std::cout << "Poll received = " << ret << std::endl;
	return ret;
}
