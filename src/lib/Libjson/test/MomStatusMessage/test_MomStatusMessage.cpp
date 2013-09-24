#include <iostream>
#include <regex.h>
#include "MomStatusMessage.hpp"
#include "test_MomStatusMessage.hpp"
const std::string TEST_MOM1_ID = "TestMom1@testhost";
const std::string TEST_MOM2_ID = "TestMom2@testhost";
const std::string TEST_MOM3_ID = "TestMom3@testhost";
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
  boost::ptr_vector<std::string> mom_status;

  // Test wrong input
  testMessage.readMergeStringStatus(NULL, mom_status, true);
  testMessage.readMergeStringStatus("", mom_status, true);
  ck_assert_msg(testHelper.getStatusMap().empty(),
      "adding readMergeStringStatus with NULL/empty args touched the map");

  // Test simple status insertion
  mom_status.push_back(new std::string("father=Homer mother=Marge son=Bart daughter=Lisa\0"));
  testMessage.readMergeStringStatus(TEST_MOM1_ID.c_str(), mom_status, true);
  statusMap = testHelper.getStatusMap();
  ck_assert_msg(!statusMap.empty(), "status map is empty");
  ck_assert_int_eq(statusMap.count(TEST_MOM1_ID), 1);
  ck_assert_msg(statusMap[TEST_MOM1_ID].size(), 4);

  // Test simple status update
  mom_status.clear();
  mom_status.push_back(new std::string("father=Homer"));
  mom_status.push_back(new std::string("mother=Marge"));
  mom_status.push_back(new std::string("dog=Santa's Little Helper"));
  testMessage.readMergeStringStatus(TEST_MOM1_ID.c_str(), mom_status, true);
  statusMap = testHelper.getStatusMap();
  ck_assert_msg(!statusMap.empty(), "status map is empty");
  ck_assert_int_eq(statusMap.count(TEST_MOM1_ID), 1);
  ck_assert_msg(statusMap[TEST_MOM1_ID].size(), 3);
  
  // Test simple status
  mom_status.clear();
  mom_status.push_back(new std::string("opsys=linux"));
  mom_status.push_back(new std::string("uname=Linux ... x86_64"));
  mom_status.push_back(new std::string("WRONG NON KEY VALUE"));
  mom_status.push_back(new std::string("sessions=335 541 922"));
  mom_status.push_back(new std::string("nsessions=19"));
  mom_status.push_back(new std::string("<gpu_status>"));
  mom_status.push_back(new std::string("gpuid=idEmpty"));
  mom_status.push_back(new std::string("gpu_mode=Default"));
  mom_status.push_back(new std::string("</gpu_status>"));
  mom_status.push_back(new std::string("nusers=3"));
  mom_status.push_back(new std::string("idletime=9638"));
  mom_status.push_back(new std::string("totmem=12374424kb"));
  mom_status.push_back(new std::string("<mic_status>"));
  mom_status.push_back(new std::string("mic_id=mic1"));
  mom_status.push_back(new std::string("key=value"));
  mom_status.push_back(new std::string("WRONG NON KEY VALUE"));
  mom_status.push_back(new std::string("</mic_status>"));
  mom_status.push_back(new std::string("availmem=7265052kb"));
  mom_status.push_back(new std::string("physmem=8180124kb"));

  testMessage.readMergeStringStatus(TEST_MOM2_ID.c_str(), mom_status, false);
  statusMap = testHelper.getStatusMap();
  ck_assert_int_eq(statusMap.count(TEST_MOM2_ID), 1);
  
  Json::Value &nodeStatus = statusMap[TEST_MOM2_ID];
  ck_assert_int_eq(nodeStatus.size(), 12);
  ck_assert_int_eq(nodeStatus["gpu_status"]["gpus"].size(), 1);
  ck_assert_int_eq(nodeStatus["mic_status"]["mics"].size(), 1);

  // Test full status
  mom_status.clear();
  mom_status.push_back(new std::string("numa1"));
  mom_status.push_back(new std::string("opsys=linux"));
  mom_status.push_back(new std::string("uname=Linux ... x86_64"));
  mom_status.push_back(new std::string("WRONG NON KEY VALUE"));
  mom_status.push_back(new std::string("sessions=335 541 922"));
  mom_status.push_back(new std::string("nsessions=19"));
  mom_status.push_back(new std::string("<gpu_status>"));
  mom_status.push_back(new std::string("WRONG NON KEY VALUE"));
  mom_status.push_back(new std::string("</gpu_status>"));
  mom_status.push_back(new std::string("<gpu_status>"));
  mom_status.push_back(new std::string("gpuid=idEmpty"));
  mom_status.push_back(new std::string("gpu_mode=Default"));
  mom_status.push_back(new std::string("gpuid=testId"));
  mom_status.push_back(new std::string("driver_ver=0.0.0001"));
  mom_status.push_back(new std::string("gpu_mode=Non-default"));
  mom_status.push_back(new std::string("</gpu_status>"));
  mom_status.push_back(new std::string("nusers=3"));
  mom_status.push_back(new std::string("<gpu_status>"));
  mom_status.push_back(new std::string("gpuid=0000:01:00.0"));
  mom_status.push_back(new std::string("timestamp=Tue Aug 20 00:00:30 2013"));
  mom_status.push_back(new std::string("driver_ver=325.15"));
  mom_status.push_back(new std::string("gpu_product_name=GeForce 210"));
  mom_status.push_back(new std::string("</gpu_status>"));
  mom_status.push_back(new std::string("idletime=9638"));
  mom_status.push_back(new std::string("totmem=12374424kb"));
  mom_status.push_back(new std::string("<mic_status>"));
  mom_status.push_back(new std::string("</mic_status>"));
  mom_status.push_back(new std::string("<mic_status>"));
  mom_status.push_back(new std::string("mic_id=mic1"));
  mom_status.push_back(new std::string("key=value"));
  mom_status.push_back(new std::string("mic_id=mic2"));
  mom_status.push_back(new std::string("WRONG NON KEY VALUE"));
  mom_status.push_back(new std::string("</mic_status>"));
  mom_status.push_back(new std::string("availmem=7265052kb"));
  mom_status.push_back(new std::string("physmem=8180124kb"));
  mom_status.push_back(new std::string("numa2"));
  mom_status.push_back(new std::string("opsys=linux"));
  mom_status.push_back(new std::string("availmem=7265052kb"));
  mom_status.push_back(new std::string("physmem=8180124kb"));

  testMessage.readMergeStringStatus(TEST_MOM3_ID.c_str(), mom_status, false);
  statusMap = testHelper.getStatusMap();
  ck_assert_int_eq(statusMap.count(TEST_MOM3_ID), 1);
  
  Json::Value &nodeStatus2 = statusMap[TEST_MOM3_ID];
  ck_assert_int_eq(nodeStatus2.size(), 2);
  ck_assert_int_eq(nodeStatus2["numa"][0]["gpu_status"]["gpus"].size(), 3);
  ck_assert_int_eq(nodeStatus2["numa"][0]["mic_status"]["mics"].size(), 2);
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

START_TEST(test_clear)
  {
  MomStatusMessage testMessage;
  testMessage.clear();
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

  tc_core = tcase_create("test_clear");
  tcase_add_test(tc_core, test_clear);
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