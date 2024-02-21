#include <infiniband/verbs.h>
#include <infiniband/mlx5dv.h>

// This is the container of stuff from ccan
#define check_type(expr, type)                  \
        ((typeof(expr) *)0 != (type *)0)

#define check_types_match(expr1, expr2)         \
        ((typeof(expr1) *)0 != (typeof(expr2) *)0)

#define container_off(containing_type, member)  \
        offsetof(containing_type, member)

#define container_of(member_ptr, containing_type, member)               \
         ((containing_type *)                                           \
          ((char *)(member_ptr)                                         \
           - container_off(containing_type, member))                    \
          + check_types_match(*(member_ptr), ((containing_type *)0)->member))

struct verbs_cq {
	union {
		struct ibv_cq cq;
		struct ibv_cq_ex cq_ex;
	};
};
enum mlx5_alloc_type {
        MLX5_ALLOC_TYPE_ANON,
        MLX5_ALLOC_TYPE_HUGE,
        MLX5_ALLOC_TYPE_CONTIG,
        MLX5_ALLOC_TYPE_PREFER_HUGE,
        MLX5_ALLOC_TYPE_PREFER_CONTIG,
        MLX5_ALLOC_TYPE_EXTERNAL,
        MLX5_ALLOC_TYPE_CUSTOM,
        MLX5_ALLOC_TYPE_ALL
};
struct mlx5_spinlock {
        pthread_spinlock_t              lock;
        int                             in_use;
        int                             need_lock;
};
struct mlx5_buf {
        void                           *buf;
        size_t                          length;
        int                             base;
        struct mlx5_hugetlb_mem        *hmem;
        enum mlx5_alloc_type            type;
        uint64_t                        resource_type;
        size_t                          req_alignment;
        struct mlx5_parent_domain       *mparent_domain;
};
struct mlx5_cq {
        struct verbs_cq                 verbs_cq;
        struct mlx5_buf                 buf_a;
        struct mlx5_buf                 buf_b;
        struct mlx5_buf                *active_buf;
        struct mlx5_buf                *resize_buf;
        int                             resize_cqes;
        int                             active_cqes;
        struct mlx5_spinlock            lock;
        uint32_t                        cqn;
        uint32_t                        cons_index;
        __be32                         *dbrec;
        bool                            custom_db;
        int                             arm_sn;
        int                             cqe_sz;
        int                             resize_cqe_sz;
        int                             stall_next_poll;
        int                             stall_enable;
        uint64_t                        stall_last_count;
        int                             stall_adaptive_enable;
        int                             stall_cycles;
        struct mlx5_resource            *cur_rsc;
        struct mlx5_srq                 *cur_srq;
        struct mlx5_cqe64               *cqe64;
        uint32_t                        flags;
        int                             cached_opcode;
        struct mlx5dv_clock_info        last_clock_info;
        struct ibv_pd                   *parent_domain;
};
static inline struct mlx5_cq *to_mcq(struct ibv_cq *ibcq)
{
        return container_of(ibcq, struct mlx5_cq, verbs_cq.cq);
}
#define MLX5_CQ_SET_CI (0)
static void update_cons_index(struct mlx5_cq *cq)
{
        cq->dbrec[MLX5_CQ_SET_CI] = htobe32(cq->cons_index & 0xffffff);
}


struct mlx5_wq {
        uint64_t                       *wrid;
        unsigned                       *wqe_head;
        struct mlx5_spinlock            lock;
        unsigned                        wqe_cnt;
        unsigned                        max_post;
        unsigned                        head;
        unsigned                        tail;
        unsigned                        cur_post;
        int                             max_gs;
        /*
         * Equal to max_gs when qp is in RTS state for sq, or in INIT state for
         * rq, equal to -1 otherwise, used to verify qp_state in data path.
         */
        int                             qp_state_max_gs;
        int                             wqe_shift;
        int                             offset;
        void                           *qend;
        uint32_t                        *wr_data;
};
struct mlx5_context {
        struct verbs_context            ibv_ctx;
};
static inline struct mlx5_context *to_mctx(struct ibv_context *ibctx)
{
        return container_of(ibctx, struct mlx5_context, ibv_ctx.context);
}
