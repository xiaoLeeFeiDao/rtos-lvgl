#ifndef DASHBOARD_H
#define DASHBOARD_H

#ifdef __cplusplus
extern "C" {
#endif

void dashboard_create(void);
void dashboard_update(int tick, int gauge_val);

#ifdef __cplusplus
}
#endif

#endif /* DASHBOARD_H */
