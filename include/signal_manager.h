#ifndef SIGNAL_MANAGER_H
#define SIGNAL_MANAGER_H

int signal_manager_init(void);
int shutdown_requested(void);

/* Returns 1 for a pending request, 0 otherwise, or -1 on error. */
int take_report_request(void);

#endif
