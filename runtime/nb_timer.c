#include "nb_timer.h"
#include <stdlib.h>

nb__timer nb__allocated_timers[MAX_TIMER_ALLOCS];
nb__timer* nb__timer_free_list = NULL;

// We are going to hold the timers in a ll
// This makes management somewhat easy
nb__timer* nb__timers_head = NULL;

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

	t->next = t->prev = NULL;

	// We find curr such that curr is the timer 
	// _after_ where to insert
	nb__timer* curr = nb__timers_head;
	nb__timer* prev = NULL;
	while (curr != NULL) {
		if (curr->timeout > to) 
			break;	
		prev = curr;
		curr = curr->next;
	}
	if (prev == NULL) {
		// Timer is being inserted at the begining 
		nb__timers_head = t;
		t->next = curr;
		if (curr) curr->prev = t;	
	} else {
		prev->next = t;
		t->next = curr;
		t->prev = prev;
		if (curr) curr->prev = t;
	}


}
void nb__remove_timer(nb__timer* t) {
	if (t->next) {
		t->next->prev = t->prev;
	}
	if (t->prev) {
		t->prev->next = t->next;
	} else 
		nb__timers_head = t->next;
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
	nb__last_timer_checked = nb__get_time_ms_now();
}


void nb__check_timers(void) {
	unsigned long long t_now = nb__get_time_ms_now();
	// Fire all timers who's timeout <= time now

	while (nb__timers_head != NULL) {
		if (nb__timers_head->timeout <= t_now) {
			nb__timer* t = nb__timers_head;
			nb__remove_timer(t);
			// We are firing the callback with current time as the third argument
			t->callback(t, t->argument, t_now);
		} else
			break;
	}

	nb__last_timer_checked = t_now;
}
