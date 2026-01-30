// NEW: components/ui/include/user_name_store.h
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define USER_NAME_MAX_LEN 63

const char *user_name_get(void);

void user_name_set(const char *name);

#ifdef __cplusplus
}
#endif
