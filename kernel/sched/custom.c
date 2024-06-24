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

static void check_preempt_curr_custom(struct rq *rq, struct task_struct *p, int flags) {

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

#ifdef CONFIG_SMP

static struct task_struct *pick_task_for_migrating(struct rq *rq, struct rq *dst_rq) {
	struct list_head *head = &rq->custom.task_list;
	struct task_struct *p;
	struct sched_custom_entity *se;

	se = list_last_entry(&rq->custom.task_list, struct sched_custom_entity, task_list);

	list_for_each_entry(se, head, task_list) {
		p = container_of(se, struct task_struct, custom_se);

		if (p->policy == SCHED_CUSTOM && cpu_active(dst_rq->cpu) && !task_running(rq, p))
			return p;
	}

	return NULL;
}

int balance_custom(struct rq *rq, struct task_struct *prev, struct rq_flags *rf) {
	int task_cpu = rq->cpu; 
	int cpu; 
	int max = 0;
	struct task_struct *p;
	struct rq *src_rq, *busiest_rq;

	rq_unpin_lock(rq, rf);
	
	if (rq->nr_running)
		goto out;

	for_each_online_cpu(cpu) {
		if (task_cpu == cpu)
			continue;

		src_rq = cpu_rq(cpu);

		if (rq->custom.nr_running > max) {
			max = rq->custom.nr_running;
			busiest_rq = src_rq;
		}
	}

	if (max) {
		double_lock_balance(rq, busiest_rq);

		p = pick_task_for_migrating(busiest_rq, rq);

		if (!p) {
			double_unlock_balance(rq, busiest_rq);
			goto out;
		}

		deactivate_task(busiest_rq, p, 0);
		set_task_cpu(p, task_cpu);
		activate_task(rq, p, 0);

		double_unlock_balance(rq, busiest_rq);

		resched_curr(rq);

		rq_repin_lock(rq, rf);
		return 1;
	}

out:
	rq_repin_lock(rq, rf);
	return 0;
}

int select_task_rq_custom(struct task_struct *p, int task_cpu, int sd_flag, int flags) {
	struct rq *rq;
	int cpu, best_cpu = task_cpu;
	int min = -1;

	if (sd_flag != SD_BALANCE_WAKE && sd_flag != SD_BALANCE_FORK)
		goto out;

	rcu_read_lock();

	for_each_online_cpu(cpu) {
		rq = cpu_rq(cpu);

		if (min == -1 || rq->nr_running < min) {
			min = rq->nr_running;
			best_cpu = cpu;
		}
	}

	rcu_read_unlock();

out:
	return best_cpu;
}

void task_woken_custom(struct rq *this_rq, struct task_struct *task){

}

static void rq_online_custom(struct rq *rq) {

}

static void rq_offline_custom(struct rq *rq) {

}

#endif

void switched_from_custom(struct rq *this_rq, struct task_struct *task) {

}

void switched_to_custom(struct rq *this_rq, struct task_struct *task) {

}

void prio_changed_custom(struct rq *this_rq, struct task_struct *task, int oldprio) {

}

/*
 * Simple, special scheduling class for the per-CPU custom tasks:
 */
const struct sched_class custom_sched_class
	__section("__custom_sched_class") = {
    .enqueue_task		= enqueue_task_custom,
	.dequeue_task		= dequeue_task_custom,
	.yield_task			= yield_task_custom,

	.check_preempt_curr	= check_preempt_curr_custom,

	.pick_next_task		= pick_next_task_custom,
	.put_prev_task		= put_prev_task_custom,
	.set_next_task      = set_next_task_custom,

#ifdef CONFIG_SMP
	.balance			= balance_custom,
	.select_task_rq		= select_task_rq_custom,

	.task_woken			= task_woken_custom,
	.set_cpus_allowed	= set_cpus_allowed_common,
	.rq_online			= rq_online_custom,
	.rq_offline			= rq_offline_custom,
#endif

	.task_tick			= task_tick_custom,

	.switched_from		= switched_from_custom,
	.switched_to		= switched_to_custom,
	.prio_changed		= prio_changed_custom,

	.update_curr		= update_curr_custom,
};
