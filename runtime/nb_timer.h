#ifndef NB_TIMER_IMPL_H
#define NB_TIMER_IMPL_H

#define MAX_TIMER_ALLOCS (1024)

struct nb__timer_obj;

typedef void (*nb__timer_callback_t)(struct nb__timer_obj*, void*, unsigned long long);

struct nb__timer_obj {
	nb__timer_callback_t callback;
	void* argument;
	
	// For chaining
	struct nb__timer_obj* next;
	struct nb__timer_obj* prev;
	unsigned long long timeout;
};

typedef struct nb__timer_obj nb__timer;

extern nb__timer nb__allocated_timers[MAX_TIMER_ALLOCS];
extern nb__timer* nb__timer_free_list;

extern nb__timer* nb__alloc_timer(void);
extern void nb__return_timer(nb__timer*);
void nb__insert_timer(nb__timer* t, unsigned long long to, nb__timer_callback_t cb, void* argument);
extern void nb__remove_timer(nb__timer*);
extern void nb__init_timers(void);

extern void nb__check_timers(void);

#endif
