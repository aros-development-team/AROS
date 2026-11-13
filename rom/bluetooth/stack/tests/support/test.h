#ifndef BT_TEST_SUPPORT_H
#define BT_TEST_SUPPORT_H

extern int bt_test_failures;
extern int bt_test_count;

void bt_test_check(int cond, const char *expr, const char *file, int line);

#define BT_CHECK(cond) bt_test_check((cond), #cond, __FILE__, __LINE__)

#endif /* BT_TEST_SUPPORT_H */
