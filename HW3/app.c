/*
 * Simple tester app
 *
 * Expected output is:
 * task 0
 * task 1
 * task 2
 * task 0
 * task 1
 * task 2
 */
#include <stdio.h>

#include "scheduler.h"

void task(int id)
{
	while (1) {
		printf("task %d\n", id);
		scheduler_yield(id);
	}
}

int main(void)
{
	const int schedule[] = {0, 1, 2, 0, 1, 2, -1};
	scheduler_run(task, 3, schedule);
	return 0;
}