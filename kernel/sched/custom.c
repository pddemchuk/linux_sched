#include "sched.h"


void init_custom_rq(struct custom_rq *custom_rq) {
	custom_rq->nr_running = 0;
	INIT_LIST_HEAD(&custom_rq->task_list);
}

static void update_curr_custom(struct rq *rq)
{
	return;
}

static void dequeue_task_custom(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_custom_entity *se = &p->custom_se;

	list_del(&se->task_list);

	se->on_rq = 0;

	rq->custom.nr_running--;
	sub_nr_running(rq, 1);
}

static void enqueue_task_custom(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_custom_entity *se = &p->custom_se;

	struct list_head *old_tasks = &rq->custom.task_list;
	struct list_head *new_tasks = &se->task_list;
	list_add_tail(new_tasks, old_tasks);

	se->on_rq = 1;

	rq->custom.nr_running++;
	add_nr_running(rq, 1);
}

static void yield_task_custom(struct rq *rq) 
{
	update_curr_custom(rq);
}

static inline void set_next_task_custom(struct rq *rq, struct task_struct *p, bool first)
{
	p->se.exec_start = rq_clock_task(rq);
}

static struct task_struct *pick_next_task_custom(struct rq *rq)
{
	if (!rq->custom.nr_running)
		return NULL;

	struct sched_custom_entity *next_se;
	struct task_struct *next_task;

	next_se = list_first_entry(&rq->custom.task_list, struct sched_custom_entity, task_list);
	next_task = container_of(next_se, struct task_struct, custom_se);

	if (!next_task)
		return NULL;

	set_next_task_custom(rq, next_task, true);
	next_task->se.exec_start = rq_clock_task(rq);
	return next_task;
}

static void put_prev_task_custom(struct rq *rq, struct task_struct *p)
{
	update_curr_custom(rq);
}

static void task_tick_custom(struct rq *rq, struct task_struct *p, int queued)
{
	update_curr_custom(rq);
}

/*
 * Simple, special scheduling class for the per-CPU custom tasks:
 */
const struct sched_class custom_sched_class
	__section("__custom_sched_class") = {
    .enqueue_task		= enqueue_task_custom,
	.dequeue_task		= dequeue_task_custom,
	.yield_task			= yield_task_custom,

	.pick_next_task		= pick_next_task_custom,
	.put_prev_task		= put_prev_task_custom,
	.set_next_task      = set_next_task_custom,

	.task_tick			= task_tick_custom,

	.update_curr		= update_curr_custom,
};
