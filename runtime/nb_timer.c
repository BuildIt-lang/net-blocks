#include "nb_timer.h"
#include <stdlib.h>

nb__timer nb__allocated_timers[MAX_TIMER_ALLOCS];
nb__timer* nb__timer_free_list = NULL;

// Actual slots to hold the timers
nb__timer* nb__timer_slots[MAX_TIMER_SLOTS];

nb__timer* nb__alloc_timer(void) {
	if (nb__timer_free_list == NULL)
		return NULL;
	nb__timer* to_ret = nb__timer_free_list;
	nb__timer_free_list = to_ret->next;
	return to_ret;
}
void nb__return_timer(nb__timer* t) {
	t->next = nb__timer_free_list;
	nb__timer_free_list = t;
}
void nb__insert_timer(nb__timer* t, unsigned long long to, nb__timer_callback_t cb, void* argument) {
	t->callback = cb;
	t->argument = argument;
	t->timeout = to;

	// TODO: Ensure that we do not insert timers too ahead
	int slot = to % MAX_TIMER_SLOTS;
	t->next = nb__timer_slots[slot];
	t->prev = NULL;
	if (t->next)
		t->next->prev = t;
	nb__timer_slots[slot] = t;		
}
void nb__remove_timer(nb__timer* t) {
	if (t->next) {
		t->next->prev = t->prev;
	}
	if (t->prev) {
		t->prev->next = t->next;
	} else {
		int slot = t->timeout % MAX_TIMER_SLOTS;
		nb__timer_slots[slot] = t->next;
	}
	t->next = t->prev = NULL;
	t->timeout = -1;
}

static unsigned long long nb__last_timer_checked;
extern unsigned long long nb__get_time_ms_now(void);
void nb__init_timers(void) {
	nb__timer_free_list = &nb__allocated_timers[0];
	for (int i = 0; i < MAX_TIMER_ALLOCS - 1; i++) {
		nb__allocated_timers[i].next = &nb__allocated_timers[i+1];
	}
	nb__allocated_timers[MAX_TIMER_ALLOCS-1].next = NULL;
	
	for (int i = 0; i < MAX_TIMER_SLOTS; i++) {
		nb__timer_slots[i] = NULL;
	}
	nb__last_timer_checked = nb__get_time_ms_now();
}


void nb__check_timers(void) {
	unsigned long long t_now = nb__get_time_ms_now();
	// Check all timers till now
	// This is because sometimes timers might not be checked if the stack is stuck
	for (unsigned long long i = nb__last_timer_checked + 1; i <= t_now; i++) {
		int slot = i % MAX_TIMER_SLOTS;
		while (nb__timer_slots[slot]) {
			nb__timer* t = nb__timer_slots[slot];
			// This is so that we keep the current slot consistent
			// It is possible that insertions and deletions might happen in callbacks
			if (t->next) 
				t->next->prev = NULL;
			nb__timer_slots[slot] = t->next;
			// It is the callback's job to free up this object
			// callbacks are allowed to reinsert timers if they wish
			t->callback(t, t->argument, i);
		}
	}
	nb__last_timer_checked = t_now;
}
