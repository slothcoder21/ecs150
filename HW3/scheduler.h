#ifndef SCHEDULER_H
#define SCHEDULER_H

/*
 * Scheduler API
 *
 * This header shall NOT be modified
 */

/* Stack size for tasks in bytes */
#define STACK_SIZE 4096

/* Max number of tasks */
#define TASK_COUNT_MAX 8

/*
 * scheduler_run - Create tasks and run scheduling sequence
 *
 * @task_func: task function
 * @task_cnt: number of tasks to create
 * @task_sched: task scheduling sequence
 *
 * This function creates @task_cnt tasks of the same function @task_func, and
 * schedules them according to the sequence provided by @task_sched.
 *
 * The task function is expected to take its integer ID as argument, and to
 * never return. It should simply yield by cooperatively call function
 * scheduler_yield().
 *
 * IDs are in the range [0, @task_cnt - 1]. The minimum number of tasks is 1 and
 * the maximum is @TASK_COUNT_MAX.
 *
 * The scheduling sequence should only contain valid IDs, otherwise the task
 * scheduling should dynamically stop and the function fail.  If we encounter
 * special flag value -1, then the task scheduling successfully ends.
 *
 * Return: 0 in case of success (complete scheduling of tasks), or -1 in case of
 * failure. Failures include invalid arguments, memory allocation issues,
 * incorrect scheduling IDs, etc.
 */
int scheduler_run(void (*task_func)(int), int task_cnt, const int *task_sched);

/*
 * scheduler_yield - Yield current task
 * @task_id: ID number of current task
 *
 * This function yields the current task, identified by @task_id, and retursn to
 * the main execution context.
 */
void scheduler_yield(int task_id);

#endif /* SCHEDULER_H */