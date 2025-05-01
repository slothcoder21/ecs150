#include <stdlib.h>
#include <ucontext.h>

#include "scheduler.h"

static ucontext_t *ctx; //task contexts array
static char **stacks; //pointer array for tasks stack
static ucontext_t main_ctx; //scheduler context

int scheduler_run(void (*task_func)(int), int task_cnt, const int *task_sched)
{
    // we need to allocate memory first
	ctx = malloc(task_cnt * sizeof(ucontext_t)); 
    if(!ctx) //if its out of memory
    {
        return -1;
    }
    stacks = malloc(task_cnt * sizeof(char *));
    if(!stacks)
    {
        return -1; 
    }
    // initializing context for each task
    for(int i = 0; i < task_cnt; i++)
    {
        stacks[i] = malloc(STACK_SIZE);
        if(!stacks[i])
        {
            // if it fails clean up allocated memory
            while(--i >= 0)
            {
                free(stacks[i]);
            }
            free(stacks);
            free(ctx);

            return -1;
        }
        //initializing context object for thread
        getcontext(&ctx[i]);
        ctx[i].uc_stack.ss_sp = stacks[i];
        ctx[i].uc_stack.ss_size = STACK_SIZE;
        ctx[i].uc_stack.ss_flags = 0;

        // When this function returns we want control to go back into the scheduler
        // linking it to the main context
        ctx[i].uc_link = &main_ctx;

        makecontext(&ctx[i], (void (*)) task_func, 1, i);
    }

    //Iterate and execute tasks in the exact order given in task_sched
    for (int i = 0; task_sched[i] >= 0; i++) {
        int tid = task_sched[i];
        swapcontext(&main_ctx, &ctx[tid]); //swap the schedulers context 
    }

    //cleaning up the stacks
    for (int i = 0; i < task_cnt; i++) {
        free(stacks[i]);
    }
    free(stacks);
    free(ctx);
    
	return 0;
}

void scheduler_yield(int task_id)
{
	/* TODO */
    swapcontext(&ctx[task_id], &main_ctx);
}