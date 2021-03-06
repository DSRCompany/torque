#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "pbs_config.h"
#include "mom_main.h"
#include "mom_config.h"
#include "pbs_error.h"
#include "test_mom_main.h"

extern bool parsing_hierarchy;
extern bool received_cluster_addrs;
extern char *path_mom_hierarchy;
extern int  MOMJobDirStickySet;

void read_mom_hierarchy();


START_TEST(test_read_mom_hierarchy)
  {
  system("rm -f bob");
  path_mom_hierarchy = strdup("bob");
  read_mom_hierarchy();
  fail_unless(received_cluster_addrs == false);
  system("touch bob");
  // the following lines need more spoofing in order to work correctly
//  parsing_hierarchy = true;
//  read_mom_hierarchy();
//  fail_unless(received_cluster_addrs == true);
//  parsing_hierarchy = false;
  }
END_TEST


START_TEST(test_mom_job_dir_sticky_config)
  {
  char *tempfilename = tempnam("/tmp", "test");
  fail_unless((tempfilename != NULL), "Failed to create a temporary filename");

  FILE *fp = fopen(tempfilename, "w");
  fail_unless(fp != NULL, "Failed to create a file to test mom_job_dir_sticky_config");

  fprintf(fp, "#some configuration\n");
  fprintf(fp, "$configversion xyz\n");
  fprintf(fp, "$loglevel 11\n");
  fflush(fp);

  int s = fclose(fp);
  fail_unless(s == 0, "Failed to close test file 1st time");

  get_mom_job_dir_sticky_config(tempfilename);
  fail_unless(MOMJobDirStickySet == 0, "Failed to detect MOMJobDirStickySet was absent");

  fp = fopen(tempfilename, "a");
  fail_unless(fp != NULL, "Failed to open file to append to continue testing on mom_job_dir_sticky_config");
  fprintf(fp, "$jobdirectory_sticky\ttrue\n");

  s = fclose(fp);
  fail_unless(s == 0, "Failed to close test file 2nd time");

  get_mom_job_dir_sticky_config(tempfilename);
  fail_unless(MOMJobDirStickySet == 1, "Failed to detect MOMJobDirStickySet");

  fp = fopen(tempfilename, "w");
  fail_unless(fp != NULL, "Failed to open file to write to continue testing on mom_job_dir_sticky_config");
  fprintf(fp, "#some more testing\n");
  fprintf(fp, "$jobdirectory_sticky false\n");

  s = fclose(fp);
  fail_unless(s == 0, "Failed to close test file 3rd time");

  get_mom_job_dir_sticky_config(tempfilename);
  fail_unless(MOMJobDirStickySet == 0, "Failed to detect MOMJobDirStickySet was false");

  unlink(tempfilename);
  get_mom_job_dir_sticky_config(tempfilename);
  fail_unless(MOMJobDirStickySet == 0, "Failed to detect MOMJobDirStickySet was unset");
  }
END_TEST


Suite *mom_main_suite(void)
  {
  Suite *s = suite_create("mom_main_suite methods");
  TCase *tc_core = tcase_create("test_read_mom_hierarchy");
  tcase_add_test(tc_core, test_read_mom_hierarchy);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("test_mom_job_dir_sticky_config");
  tcase_add_test(tc_core, test_mom_job_dir_sticky_config);
  suite_add_tcase(s, tc_core);

  return s;
  }

void rundebug()
  {
  }

int main(void)
  {
  int number_failed = 0;
  SRunner *sr = NULL;
  rundebug();
  sr = srunner_create(mom_main_suite());
  srunner_set_log(sr, "mom_main_suite.log");
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return number_failed;
  }
