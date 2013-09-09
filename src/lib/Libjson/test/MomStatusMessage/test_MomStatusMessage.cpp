#include <iostream>
#include <regex.h>
#include "MomStatusMessage.hpp"
#include "test_MomStatusMessage.hpp"

const std::string TEST_MOM1_ID = "TestMom1@testhost";
const std::string TEST_MOM2_ID = "TestMom2@testhost";

namespace TrqJson {
  class TestHelper
    {
    private:
      MomStatusMessage &message;
    public:
      TestHelper(MomStatusMessage &message) : message(message)
      {
      }
      std::map<std::string, Json::Value> getStatusMap()
        {
        return message.statusMap;
        }
    };
}

using namespace TrqJson;

START_TEST(test_readMergeStringStatus)
  {
  regex_t regex;
  std::map<std::string, Json::Value> statusMap;
  MomStatusMessage testMessage;
  TestHelper testHelper(testMessage);

  // Test wrong input
  testMessage.readMergeStringStatus(NULL, NULL, true);
  testMessage.readMergeStringStatus("", NULL, true);
  testMessage.readMergeStringStatus(TEST_MOM1_ID.c_str(), NULL, true);
  testMessage.readMergeStringStatus(TEST_MOM1_ID.c_str(), "", true);
  ck_assert_msg(testHelper.getStatusMap().empty(),
      "adding readMergeStringStatus with NULL/empty args touched the map");

  // Test simple status insertion
  testMessage.readMergeStringStatus(TEST_MOM1_ID.c_str(),
      "father=Homer\0mother=Marge\0son=Bart\0daughter=Lisa\0", true);
  statusMap = testHelper.getStatusMap();
  ck_assert_msg(!statusMap.empty(), "status map is empty");
  ck_assert_int_eq(statusMap.count(TEST_MOM1_ID), 1);
  ck_assert_msg(statusMap[TEST_MOM1_ID].size(), 4);

  // Test simple status update
  testMessage.readMergeStringStatus(TEST_MOM1_ID.c_str(),
      "father=Homer\0mother=Marge\0dog=Santa's Little Helper\0", true);
  statusMap = testHelper.getStatusMap();
  ck_assert_msg(!statusMap.empty(), "status map is empty");
  ck_assert_int_eq(statusMap.count(TEST_MOM1_ID), 1);
  ck_assert_msg(statusMap[TEST_MOM1_ID].size(), 3);

  // Test full status
  testMessage.readMergeStringStatus(TEST_MOM2_ID.c_str(),
      /* val 01     */ "opsys=linux\0"
      /* val 02     */ "uname=Linux ... x86_64\0"
      /* ignored    */ "WRONG NON KEY VALUE\0"
      /* val 03     */ "sessions=335 541 922\0"
      /* val 04     */ "nsessions=19\0"
      /* gpu        */ "<gpu_status>\0"
      /* /gpu       */ "</gpu_status>\0"
      /* gpu        */ "<gpu_status>\0"
      /* gpu1 01    */ "gpu_mode=Default\0"
      /* gpu2 01    */ "gpuid=idEmpty\0"
      /* gpu3 01    */ "gpuid=testId\0"
      /* gpu3 02    */ "driver_ver=0.0.0001\0"
      /* gpu3 03    */ "gpu_mode=Non-default\0"
      /* /gpu       */ "</gpu_status>\0"
      /* val 05     */ "nusers=3\0"
      /* gpu        */ "<gpu_status>\0"
      /* gpu all 01 */ "timestamp=Tue Aug 20 00:00:30 2013\0"
      /* gpu all 02 */ "driver_ver=325.15\0"
      /* gpu4 03    */ "gpuid=0000:01:00.0\0"
      /* gpu4 04    */ "gpu_product_name=GeForce 210\0"
      /* /gpu       */ "</gpu_status>\0"
      /* val 06     */ "idletime=9638\0"
      /* val 07     */ "totmem=12374424kb\0"
      /* mic        */ "<mic_status>\0"
      /* /mic       */ "</mic_status>\0"
      /* mic        */ "<mic_status>\0"
      /* mic1 01    */ "mic_id=mic1\0"
      /* mic1 02    */ "key=value\0"
      /* mic2 01    */ "mic_id=mic2\0"
      /* /mic       */ "</mic_status>\0"
      /* val 08     */ "availmem=7265052kb\0"
      /* val 09     */ "physmem=8180124kb\0"
      /* val 10 gpus */
      /* val 11 mics */
      /* val 12 node */
      , false);
  statusMap = testHelper.getStatusMap();
  ck_assert_int_eq(statusMap.count(TEST_MOM2_ID), 1);
  Json::Value &nodeStatus = statusMap[TEST_MOM2_ID];
  ck_assert_int_eq(nodeStatus.size(), 12);
  ck_assert_int_eq(nodeStatus["gpu_status"].size(), 4);
  ck_assert_int_eq(nodeStatus["mic_status"].size(), 2);
  }
END_TEST

START_TEST(test_readMergeJsonStatuses)
  {
  int rc;
  regex_t regex;
  std::map<std::string, Json::Value> statusMap;
  MomStatusMessage testMessage;
  TestHelper testHelper(testMessage);
  const char *str;

  // Test wrong input
  rc = testMessage.readMergeJsonStatuses(0, NULL);
  ck_assert_int_eq(rc, -1);

  // Non Json
  str = "this is non-json string";
  rc = testMessage.readMergeJsonStatuses(strlen(str), str);
  ck_assert_int_eq(rc, -1);

  // Non message
  str = "{\"something\":\"something_more\"}";
  rc = testMessage.readMergeJsonStatuses(strlen(str), str);
  ck_assert_int_eq(rc, -1);

  // Wrong type of message type field
  str = "{\"something\":\"something_more\","
    "\"messageType\":1234,"
    "\"senderId\":\"sender_node\","
    "\"body\":[]}";
  rc = testMessage.readMergeJsonStatuses(strlen(str), str);
  ck_assert_int_eq(rc, -1);

  // Message isn't "status"
  str = "{\"something\":\"something_more\","
    "\"messageType\":\"anotherType\","
    "\"senderId\":\"sender_node\","
    "\"body\":[]}";
  rc = testMessage.readMergeJsonStatuses(strlen(str), str);
  ck_assert_int_eq(rc, -1);

  // Sender id is absend
  str = "{\"something\":\"something_more\","
    "\"messageType\":\"anotherType\","
    "\"body\":[]}";
  rc = testMessage.readMergeJsonStatuses(strlen(str), str);
  ck_assert_int_eq(rc, -1);

  // Sender id isn't string
  str = "{\"something\":\"something_more\","
    "\"messageType\":\"anotherType\","
    "\"senderId\":1234,"
    "\"body\":[]}";
  rc = testMessage.readMergeJsonStatuses(strlen(str), str);
  ck_assert_int_eq(rc, -1);

  // Body isn't array
  str = "{\"something\":\"something_more\","
    "\"messageType\":\"status\","
    "\"senderId\":\"sender_node\","
    "\"body\":1234}";
  rc = testMessage.readMergeJsonStatuses(strlen(str), str);
  ck_assert_int_eq(rc, -1);

  ck_assert_msg(testHelper.getStatusMap().empty(),
      "adding readMergeJsonStatuses with wrong args touched the map");

  // Test empty body array
  str = "{\"something\":\"something_more\","
    "\"messageType\":\"status\","
    "\"senderId\":\"sender_node\","
    "\"body\":[]}";
  rc = testMessage.readMergeJsonStatuses(strlen(str), str);
  ck_assert_int_eq(rc, 0);

  ck_assert_msg(testHelper.getStatusMap().empty(),
      "adding readMergeJsonStatuses with empty body touched the map");

  // Test normal case (with wrong parts)
  str = "{\"something\":\"something_more\","
    "\"messageType\":\"status\","
    "\"senderId\":\"sender_node\","
    "\"body\":["
    "{\"node\":\"node1@host\",\"key\":\"value\"},"
    "{                        \"key\":\"value\"}," /* node without id (skipped) */
    "{\"node\":\"node2@host\",\"key\":\"value\"}"
    "]}";
  rc = testMessage.readMergeJsonStatuses(strlen(str), str);
  ck_assert_int_eq(rc, 2);
  ck_assert_int_eq(testHelper.getStatusMap().size(), 2);
  }
END_TEST

START_TEST(test_body_and_type)
  {
  int rc;
  regex_t regex;
  std::map<std::string, Json::Value> statusMap;
  MomStatusMessage testMessage;
  const char *status_str;

  // Test empty body array
  status_str = "{"
    "\"messageType\":\"status\","
    "\"senderId\":\"sender_node\","
    "\"body\":["
    "{\"node\":\"node1@host\",\"key\":\"value\"},"
    "{\"node\":\"node2@host\",\"key\":\"value\"}"
    "]}";
  rc = testMessage.readMergeJsonStatuses(strlen(status_str), status_str);
  ck_assert_int_eq(rc, 2);

  testMessage.setMomId("mom@host");

  std::string *out = testMessage.write();
  ck_assert_msg(!out->empty(), "Message::write() returned empty string");

  Json::Value root;
  Json::Reader reader;
  bool parsed = reader.parse(*out, root);
  ck_assert_msg(parsed, "Failed parse written Message");

  Json::Value messageType = root["messageType"];
  ck_assert_msg(messageType.isString(), "messageType value isn't string");
  ck_assert_str_eq(messageType.asCString(), "status");

  Json::Value body = root["body"];
  ck_assert_msg(body.isArray(), "body isn't array value");
  ck_assert_msg(body.size() == 2, "body expect to be contained 2 values");
  }
END_TEST

Suite *MomStatusMessage_suite(void)
  {
  Suite *s = suite_create("MomStatusMessage_suite methods");
  TCase *tc_core = tcase_create("test_readMergeStringStatus");
  tcase_add_test(tc_core, test_readMergeStringStatus);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("test_readMergeJsonStatuses");
  tcase_add_test(tc_core, test_readMergeJsonStatuses);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("test_body_and_type");
  tcase_add_test(tc_core, test_body_and_type);
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
  sr = srunner_create(MomStatusMessage_suite());
  srunner_set_log(sr, "MomStatusMessage_suite.log");
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return number_failed;
  }
