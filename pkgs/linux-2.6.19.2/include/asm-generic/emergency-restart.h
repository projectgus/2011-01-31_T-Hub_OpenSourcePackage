#ifndef _ASM_GENERIC_EMERGENCY_RESTART_H
#define _ASM_GENERIC_EMERGENCY_RESTART_H

static inline void machine_emergency_restart(void)
{
	machine_restart(NULL);
}

static inline void mahcine_soft_emergency_restart(void)
{
	soft_machine_restart(NULL);
}
#endif /* _ASM_GENERIC_EMERGENCY_RESTART_H */
