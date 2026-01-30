#include <stdio.h>
#include <string.h>
#include "user_name_store.h"

#define USER_NAME_PATH "/spiffs/user_name.txt"

static char s_user_name[USER_NAME_MAX_LEN + 1] = {0};
static int  s_loaded = 0;  

static void user_name_try_load(void)
{
    if (s_loaded) {
        return;
    }

    FILE *f = fopen(USER_NAME_PATH, "r");
    if (!f) {
        s_user_name[0] = '\0';
        s_loaded = 1;
        return;
    }

    size_t n = fread(s_user_name, 1, USER_NAME_MAX_LEN, f);
    fclose(f);

    size_t len = n;
    while (len > 0 &&
           (s_user_name[len - 1] == '\n' ||
            s_user_name[len - 1] == '\r' ||
            s_user_name[len - 1] == ' ')) {
        len--;
    }
    s_user_name[len] = '\0';

    s_loaded = 1;
}

const char *user_name_get(void)
{
    user_name_try_load();
    if (s_user_name[0] == '\0') {
        return NULL;
    }
    return s_user_name;
}

void user_name_set(const char *name)
{
    user_name_try_load(); 

    if (!name) {
        return;
    }

    size_t len = strnlen(name, USER_NAME_MAX_LEN);
    memcpy(s_user_name, name, len);
    s_user_name[len] = '\0';

    FILE *f = fopen(USER_NAME_PATH, "w");
    if (!f) {
        return;
    }
    fwrite(s_user_name, 1, len, f);
    fclose(f);
}
