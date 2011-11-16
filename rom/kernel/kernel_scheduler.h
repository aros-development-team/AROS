BOOL core_Schedule(void);			/* Reschedule the current task if needed */
void core_Switch(void);				/* Switch away from the current task     */
struct Task *core_Dispatch(void);		/* Select the new task for execution     */
