#include <regex.h>
#include "Message.hpp"
#include "test_Message.hpp"

const std::string TEST_MOM_ID = "TestMom@testhost";
const std::string TEST_MSG_TYPE = "testMessageType";

namespace TrqJson {

class TestMessage : public TrqJson::Message
  {
  protected:
    void generateBody(Json::Value &messageBody) { messageBody["test"] = "test"; }
    std::string getMessageType() { return TEST_MSG_TYPE; }
  };

};

using namespace TrqJson;

START_TEST(test_one)
  {
  int rc;
  regex_t regex;
  TestMessage testMessage;

  testMessage.setMomId(TEST_MOM_ID);

  std::string *out = testMessage.write();
  ck_assert_msg(!out->empty(), "Message::write() returned empty string");

  Json::Value root;
  Json::Reader reader;
  bool parsed = reader.parse(*out, root);
  ck_assert_msg(parsed, "Failed parse written Message");

  Json::Value msgId = root["messageId"];
  ck_assert_msg(msgId.isString(), "messageId value isn't string");
  rc = regcomp(&regex, "[[:xdigit:]]{8}-([[:xdigit:]]{4}-){3}[[:xdigit:]]{12}", REG_EXTENDED);
  ck_assert_msg(rc == 0, "Can't compile regex (test code error)");
  rc = regexec(&regex, msgId.asCString(), 0, NULL, 0);
  ck_assert_msg(rc == 0, "msgId isn't in UUID format");

  Json::Value msgType = root["messageType"];
  std::cout << msgType.asString() << std::endl;
  ck_assert_msg(msgType.isString(), "messageType value isn't string");
  ck_assert_str_eq(msgType.asCString(), TEST_MSG_TYPE.c_str());

  Json::Value ttl = root["ttl"];
  ck_assert_msg(ttl.isInt(), "ttl value isn't int");
  ck_assert_int_eq(ttl.asInt(), 3000);

  Json::Value sentDate = root["sentDate"];
  ck_assert_msg(sentDate.isString(), "sentDate value isn't string");
  rc = regcomp(&regex, "[[:digit:]]{4}(-[[:digit:]]{2}){2} ([[:digit:]]{2}:){2}[[:digit:]]{2}\\."
      "[[:digit:]]{3}[+-][[:digit:]]{4}", REG_EXTENDED);
  ck_assert_msg(rc == 0, "Can't compile regex (test code error)");
  rc = regexec(&regex, sentDate.asCString(), 0, NULL, 0);
  ck_assert_msg(rc == 0, "sentDate isn't in ISO8601 format");

  Json::Value senderId = root["senderId"];
  ck_assert_msg(senderId.isString(), "senderId value isn't string");
  ck_assert_str_eq(senderId.asCString(), TEST_MOM_ID.c_str());

  // Check message body was called
  Json::Value bodyVal = root["body"]["test"];
  ck_assert_msg(bodyVal.isString(), "test value in body not found");
  ck_assert_str_eq(bodyVal.asCString(), "test");

  // Check delete buffer function
  Message::deleteString(NULL, out);
  Message::deleteString(NULL, NULL);
  }
END_TEST

Suite *Message_suite(void)
  {
  Suite *s = suite_create("Message_suite methods");
  TCase *tc_core = tcase_create("test_one");
  tcase_add_test(tc_core, test_one);
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
  sr = srunner_create(Message_suite());
  srunner_set_log(sr, "Message_suite.log");
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return number_failed;
  }
