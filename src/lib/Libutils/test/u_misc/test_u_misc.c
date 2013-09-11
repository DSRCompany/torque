<<<<<<< HEAD
#include <stdio.h>
#include <stdlib.h>
#include <check.h>


START_TEST(test_one)
  {
  }
END_TEST




START_TEST(test_two)
  {
=======
#include "utils.h"

#include "test_u_misc.h"

START_TEST(shuffle_indices_test)
  {
#ifndef TEST_ARRAY_SIZE
#define TEST_ARRAY_SIZE 777
#endif
  int *arr = (int *)malloc(TEST_ARRAY_SIZE * sizeof(int));

  /* NOTE: The function is disallowed to be called with NULL array or size < 0 with assert() */

  /* Check don't fail with boundary conditions */

  /* check zero size */
  arr[0] = 111;
  shuffle_indices(arr, 0);
  ck_assert_int_eq(arr[0], 111); // untouched

  /* check don't write outside the array */
  arr[0] = 222;
  arr[101] = 333;
  shuffle_indices(arr + 1, 100);
  ck_assert_int_eq(arr[0], 222);
  ck_assert_int_eq(arr[101], 333);
  
  /* check result */
  shuffle_indices(arr, TEST_ARRAY_SIZE);
  int zero_cnt = 0;
  for (int i = 0; i < TEST_ARRAY_SIZE; i++)
    {
    if (arr[i] == 0)
      {
      zero_cnt++;
      }
    }
  ck_assert_int_eq(zero_cnt, 1);

  /* free resources */
  free(arr);
>>>>>>> Connect to nodes in random order.
  }
END_TEST


<<<<<<< HEAD


Suite *u_misc_suite(void)
  {
  Suite *s = suite_create("u_misc test suite methods");
  TCase *tc_core = tcase_create("test_one");
  tcase_add_test(tc_core, test_one);
  suite_add_tcase(s, tc_core);
  
  tc_core = tcase_create("test_two");
  tcase_add_test(tc_core, test_two);
  suite_add_tcase(s, tc_core);
  
  return(s);
=======
Suite *u_misc_suite(void)
  {
  Suite *s = suite_create("u_misc methods");
  TCase *tc_core;
    
  tc_core = tcase_create("shuffle_indices_test");
  tcase_add_test(tc_core, shuffle_indices_test);
  suite_add_tcase(s, tc_core);

  return s;
>>>>>>> Connect to nodes in random order.
  }

void rundebug()
  {
  }

int main(void)
  {
  int number_failed = 0;
  SRunner *sr = NULL;
  rundebug();
  sr = srunner_create(u_misc_suite());
  srunner_set_log(sr, "u_misc_suite.log");
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
<<<<<<< HEAD
  return(number_failed);
=======
  return number_failed;
>>>>>>> Connect to nodes in random order.
  }
