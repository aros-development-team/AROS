/* Main scheduler entry point */
void core_ExitInterrupt(regs_t *regs);
/* CPU-specific wrappers. Need to be implemented in CPU-specific parts */
void cpu_Switch(regs_t *regs);
void cpu_Dispatch(regs_t *regs);
