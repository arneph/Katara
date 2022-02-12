//
//  flags_test.cc
//  Katara
//
//  Created by Arne Philipeit on 2/5/22.
//  Copyright © 2022 Arne Philipeit. All rights reserved.
//

#include "src/common/flags/flags.h"

#include <sstream>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::IsEmpty;

namespace common {

TEST(FlagsParseTest, NoDefinedFlagsHandlesNoArgs) {
  std::vector<std::string> args;
  FlagSet flags;
  EXPECT_TRUE(flags.Parse(args, nullptr));
  EXPECT_THAT(args, IsEmpty());
}

TEST(FlagsParseTest, NoDefinedFlagsHandlesNoFlagsInArgs) {
  const std::vector<std::string> original_args{"abc", "123", "+-*!", "x"};
  std::vector<std::string> args = original_args;
  FlagSet flags;
  EXPECT_TRUE(flags.Parse(args, nullptr));
  EXPECT_EQ(args, original_args);
}

TEST(FlagsParseTest, NoDefinedFlagsHandlesNoFlagsAndFlagsTerminatorInArgs) {
  const std::vector<std::string> original_args{"abc",     "123", "+-*!", "--", "-should-be-ignored",
                                               "--hi=42", "x"};
  const std::vector<std::string> expected_args{"abc",     "123", "+-*!", "-should-be-ignored",
                                               "--hi=42", "x"};
  std::vector<std::string> args = original_args;
  FlagSet flags;
  EXPECT_TRUE(flags.Parse(args, nullptr));
  EXPECT_EQ(args, expected_args);
}

TEST(FlagsParseTest, NoDefinedFlagsHandlesUndefinedFlagWithDoubleDashInMiddleOfArgs) {
  const std::vector<std::string> original_args{"abc", "123", "+-*!", "--test", "x", "y"};
  const std::vector<std::string> expected_args{"abc", "123", "+-*!", "y"};
  std::vector<std::string> args = original_args;
  std::stringstream ss;
  FlagSet flags;
  EXPECT_FALSE(flags.Parse(args, &ss));
  EXPECT_EQ(args, expected_args);
  EXPECT_EQ(ss.str(), "flag -test does not exist\n");
}

TEST(FlagsParseTest, NoDefinedFlagsHandlesUndefinedFlagWithDoubleDashAndEqualsInMiddleOfArgs) {
  const std::vector<std::string> original_args{"abc", "123", "+-*!", "--test=42", "x", "y"};
  const std::vector<std::string> expected_args{"abc", "123", "+-*!", "x", "y"};
  std::vector<std::string> args = original_args;
  std::stringstream ss;
  FlagSet flags;
  EXPECT_FALSE(flags.Parse(args, &ss));
  EXPECT_EQ(args, expected_args);
  EXPECT_EQ(ss.str(), "flag -test does not exist\n");
}

TEST(FlagsParseTest, NoDefinedFlagsHandlesUndefinedFlagWithSingleDashInMiddleOfArgs) {
  const std::vector<std::string> original_args{"abc", "123", "+-*!", "-test", "x", "y"};
  const std::vector<std::string> expected_args{"abc", "123", "+-*!", "y"};
  std::vector<std::string> args = original_args;
  std::stringstream ss;
  FlagSet flags;
  EXPECT_FALSE(flags.Parse(args, &ss));
  EXPECT_EQ(args, expected_args);
  EXPECT_EQ(ss.str(), "flag -test does not exist\n");
}

TEST(FlagsParseTest, NoDefinedFlagsHandlesUndefinedFlagWithSingleDashAndEqualsInMiddleOfArgs) {
  const std::vector<std::string> original_args{"abc", "123", "+-*!", "-test=42", "x", "y"};
  const std::vector<std::string> expected_args{"abc", "123", "+-*!", "x", "y"};
  std::vector<std::string> args = original_args;
  std::stringstream ss;
  FlagSet flags;
  EXPECT_FALSE(flags.Parse(args, &ss));
  EXPECT_EQ(args, expected_args);
  EXPECT_EQ(ss.str(), "flag -test does not exist\n");
}

TEST(FlagsParseTest, NoDefinedFlagsHandlesUndefinedFlagWithDoubleDashAtEndOfArgs) {
  const std::vector<std::string> original_args{"abc", "123", "+-*!", "--test"};
  const std::vector<std::string> expected_args{"abc", "123", "+-*!"};
  std::vector<std::string> args = original_args;
  std::stringstream ss;
  FlagSet flags;
  EXPECT_FALSE(flags.Parse(args, &ss));
  EXPECT_EQ(args, expected_args);
  EXPECT_EQ(ss.str(), "flag -test does not exist\n");
}

TEST(FlagsParseTest, NoDefinedFlagsHandlesUndefinedFlagWithDoubleDashAndEqualsAtEndOfArgs) {
  const std::vector<std::string> original_args{"abc", "123", "+-*!", "--test=hi"};
  const std::vector<std::string> expected_args{"abc", "123", "+-*!"};
  std::vector<std::string> args = original_args;
  std::stringstream ss;
  FlagSet flags;
  EXPECT_FALSE(flags.Parse(args, &ss));
  EXPECT_EQ(args, expected_args);
  EXPECT_EQ(ss.str(), "flag -test does not exist\n");
}

TEST(FlagsParseTest, NoDefinedFlagsHandlesUndefinedFlagWithSingleDashAtEndOfArgs) {
  const std::vector<std::string> original_args{"abc", "123", "+-*!", "-test"};
  const std::vector<std::string> expected_args{"abc", "123", "+-*!"};
  std::vector<std::string> args = original_args;
  std::stringstream ss;
  FlagSet flags;
  EXPECT_FALSE(flags.Parse(args, &ss));
  EXPECT_EQ(args, expected_args);
  EXPECT_EQ(ss.str(), "flag -test does not exist\n");
}

TEST(FlagsParseTest, NoDefinedFlagsHandlesUndefinedFlagWithSingleDashAndEqualsAtEndOfArgs) {
  const std::vector<std::string> original_args{"abc", "123", "+-*!", "-test=hi"};
  const std::vector<std::string> expected_args{"abc", "123", "+-*!"};
  std::vector<std::string> args = original_args;
  std::stringstream ss;
  FlagSet flags;
  EXPECT_FALSE(flags.Parse(args, &ss));
  EXPECT_EQ(args, expected_args);
  EXPECT_EQ(ss.str(), "flag -test does not exist\n");
}

TEST(FlagsParseTest, NoDefinedFlagsHandlesSingleDashWithoutNameInMiddleOfArgs) {
  const std::vector<std::string> original_args{"abc", "123", "+-*!", "-", "x", "y"};
  const std::vector<std::string> expected_args{"abc", "123", "+-*!", "y"};
  std::vector<std::string> args = original_args;
  std::stringstream ss;
  FlagSet flags;
  EXPECT_FALSE(flags.Parse(args, &ss));
  EXPECT_EQ(args, expected_args);
  EXPECT_EQ(ss.str(), "missing flag name: -\n");
}

TEST(FlagsParseTest, NoDefinedFlagsHandlesSingleDashWithoutNameAndEqualsInMiddleOfArgs) {
  const std::vector<std::string> original_args{"abc", "123", "+-*!", "-=hi", "x", "y"};
  const std::vector<std::string> expected_args{"abc", "123", "+-*!", "x", "y"};
  std::vector<std::string> args = original_args;
  std::stringstream ss;
  FlagSet flags;
  EXPECT_FALSE(flags.Parse(args, &ss));
  EXPECT_EQ(args, expected_args);
  EXPECT_EQ(ss.str(), "missing flag name: -=hi\n");
}

TEST(FlagsParseTest, NoDefinedFlagsHandlesSingleDashWithoutNameAtEndOfArgs) {
  const std::vector<std::string> original_args{"abc", "123", "+-*!", "-=true"};
  const std::vector<std::string> expected_args{"abc", "123", "+-*!"};
  std::vector<std::string> args = original_args;
  std::stringstream ss;
  FlagSet flags;
  EXPECT_FALSE(flags.Parse(args, &ss));
  EXPECT_EQ(args, expected_args);
  EXPECT_EQ(ss.str(), "missing flag name: -=true\n");
}

TEST(FlagsParseTest, NoDefinedFlagsHandlesSingleDashWithoutNameAndEqualsAtEndOfArgs) {
  const std::vector<std::string> original_args{"abc", "123", "+-*!", "-=true"};
  const std::vector<std::string> expected_args{"abc", "123", "+-*!"};
  std::vector<std::string> args = original_args;
  std::stringstream ss;
  FlagSet flags;
  EXPECT_FALSE(flags.Parse(args, &ss));
  EXPECT_EQ(args, expected_args);
  EXPECT_EQ(ss.str(), "missing flag name: -=true\n");
}

TEST(FlagsParseTest, NoDefinedFlagsHandlesDoubleDashWithoutNameAndEqualsInMiddleOfArgs) {
  const std::vector<std::string> original_args{"abc", "123", "+-*!", "--=hi", "x", "y"};
  const std::vector<std::string> expected_args{"abc", "123", "+-*!", "x", "y"};
  std::vector<std::string> args = original_args;
  std::stringstream ss;
  FlagSet flags;
  EXPECT_FALSE(flags.Parse(args, &ss));
  EXPECT_EQ(args, expected_args);
  EXPECT_EQ(ss.str(), "missing flag name: --=hi\n");
}

TEST(FlagsParseTest, NoDefinedFlagsHandlesDoubleDashWithoutNameAndEqualsAtEndOfArgs) {
  const std::vector<std::string> original_args{"abc", "123", "+-*!", "--=true"};
  const std::vector<std::string> expected_args{"abc", "123", "+-*!"};
  std::vector<std::string> args = original_args;
  std::stringstream ss;
  FlagSet flags;
  EXPECT_FALSE(flags.Parse(args, &ss));
  EXPECT_EQ(args, expected_args);
  EXPECT_EQ(ss.str(), "missing flag name: --=true\n");
}

TEST(FlagsParseTest, BoolFlagHandlesNoAssignment) {
  std::vector<std::string> args;
  bool test_flag = false;
  FlagSet flags;
  flags.Add<bool>("test", "some usage", true, test_flag);
  EXPECT_TRUE(test_flag);
  EXPECT_TRUE(flags.Parse(args, nullptr));
  EXPECT_THAT(args, IsEmpty());
  EXPECT_TRUE(test_flag);
}

TEST(FlagsParseTest, BoolFlagHandlesAssignmentWithoutValue) {
  std::vector<std::string> args = {"--test"};
  bool test_flag = true;
  FlagSet flags;
  flags.Add<bool>("test", "some usage", false, test_flag);
  EXPECT_FALSE(test_flag);
  EXPECT_TRUE(flags.Parse(args, nullptr));
  EXPECT_THAT(args, IsEmpty());
  EXPECT_TRUE(test_flag);
}

TEST(FlagsParseTest, BoolFlagHandlesFalseAssignment) {
  for (std::string false_string :
       std::vector<std::string>{"0", "f", "F", "false", "False", "FALSE"}) {
    std::vector<std::string> args = {"--test=" + false_string};
    bool test_flag = false;
    FlagSet flags;
    flags.Add<bool>("test", "some usage", true, test_flag);
    EXPECT_TRUE(test_flag);
    EXPECT_TRUE(flags.Parse(args, nullptr));
    EXPECT_THAT(args, IsEmpty());
    EXPECT_FALSE(test_flag);
  }
}

TEST(FlagsParseTest, BoolFlagHandlesTrueAssignment) {
  for (std::string true_string : std::vector<std::string>{"1", "t", "T", "true", "True", "TRUE"}) {
    std::vector<std::string> args = {"--test=" + true_string};
    bool test_flag = true;
    FlagSet flags;
    flags.Add<bool>("test", "some usage", false, test_flag);
    EXPECT_FALSE(test_flag);
    EXPECT_TRUE(flags.Parse(args, nullptr));
    EXPECT_THAT(args, IsEmpty());
    EXPECT_TRUE(test_flag);
  }
}

TEST(FlagsParseTest, BoolFlagRejectsWrongAssignment) {
  for (std::string value_string : std::vector<std::string>{"2", "a", "A", "tRuE", "hihi", ""}) {
    std::vector<std::string> args = {"--test=" + value_string};
    std::stringstream ss;
    bool test_flag = true;
    FlagSet flags;
    flags.Add<bool>("test", "some usage", false, test_flag);
    EXPECT_FALSE(test_flag);
    EXPECT_FALSE(flags.Parse(args, &ss));
    EXPECT_THAT(args, IsEmpty());
    EXPECT_EQ(ss.str(), "flag -test does not accept value \"" + value_string + "\"\n");
  }
}

TEST(FlagsParseTest, IntFlagHandlesNoAssignment) {
  std::vector<std::string> args;
  int64_t test_flag = 42;
  FlagSet flags;
  flags.Add<int64_t>("test", "some usage", 123, test_flag);
  EXPECT_EQ(test_flag, 123);
  EXPECT_TRUE(flags.Parse(args, nullptr));
  EXPECT_THAT(args, IsEmpty());
  EXPECT_EQ(test_flag, 123);
}

TEST(FlagsParseTest, IntFlagRejectsAssignmentWithoutValue) {
  std::vector<std::string> args = {"--test"};
  std::stringstream ss;
  int64_t test_flag = 42;
  FlagSet flags;
  flags.Add<int64_t>("test", "some usage", 123, test_flag);
  EXPECT_EQ(test_flag, 123);
  EXPECT_FALSE(flags.Parse(args, &ss));
  EXPECT_THAT(args, IsEmpty());
  EXPECT_EQ(ss.str(), "flag -test does not accept value \"\"\n");
  EXPECT_EQ(test_flag, 123);
}

TEST(FlagsParseTest, IntFlagRejectsAssignmentWithEqualsWithoutValue) {
  std::vector<std::string> args = {"--test="};
  std::stringstream ss;
  int64_t test_flag = 42;
  FlagSet flags;
  flags.Add<int64_t>("test", "some usage", 123, test_flag);
  EXPECT_EQ(test_flag, 123);
  EXPECT_FALSE(flags.Parse(args, &ss));
  EXPECT_THAT(args, IsEmpty());
  EXPECT_EQ(ss.str(), "flag -test does not accept value \"\"\n");
  EXPECT_EQ(test_flag, 123);
}

TEST(FlagsParseTest, IntFlagAcceptsBasicAssignment) {
  std::vector<std::string> args = {"--test", "789"};
  int64_t test_flag = 42;
  FlagSet flags;
  flags.Add<int64_t>("test", "some usage", 123, test_flag);
  EXPECT_EQ(test_flag, 123);
  EXPECT_TRUE(flags.Parse(args, nullptr));
  EXPECT_THAT(args, IsEmpty());
  EXPECT_EQ(test_flag, 789);
}

TEST(FlagsParseTest, IntFlagAcceptsBasicAssignmentWithEquals) {
  std::vector<std::string> args = {"--test=789"};
  int64_t test_flag = 42;
  FlagSet flags;
  flags.Add<int64_t>("test", "some usage", 123, test_flag);
  EXPECT_EQ(test_flag, 123);
  EXPECT_TRUE(flags.Parse(args, nullptr));
  EXPECT_THAT(args, IsEmpty());
  EXPECT_EQ(test_flag, 789);
}

TEST(FlagsParseTest, IntFlagAcceptsAssignmentOfZero) {
  std::vector<std::string> args = {"--test", "0"};
  int64_t test_flag = 42;
  FlagSet flags;
  flags.Add<int64_t>("test", "some usage", 123, test_flag);
  EXPECT_EQ(test_flag, 123);
  EXPECT_TRUE(flags.Parse(args, nullptr));
  EXPECT_THAT(args, IsEmpty());
  EXPECT_EQ(test_flag, 0);
}

TEST(FlagsParseTest, IntFlagAcceptsAssignmentOfZeroWithEquals) {
  std::vector<std::string> args = {"--test=0"};
  int64_t test_flag = 42;
  FlagSet flags;
  flags.Add<int64_t>("test", "some usage", 123, test_flag);
  EXPECT_EQ(test_flag, 123);
  EXPECT_TRUE(flags.Parse(args, nullptr));
  EXPECT_THAT(args, IsEmpty());
  EXPECT_EQ(test_flag, 0);
}

TEST(FlagsParseTest, IntFlagAcceptsAssignmentOfNegativeNumber) {
  std::vector<std::string> args = {"--test", "-7"};
  int64_t test_flag = 42;
  FlagSet flags;
  flags.Add<int64_t>("test", "some usage", 123, test_flag);
  EXPECT_EQ(test_flag, 123);
  EXPECT_TRUE(flags.Parse(args, nullptr));
  EXPECT_THAT(args, IsEmpty());
  EXPECT_EQ(test_flag, -7);
}

TEST(FlagsParseTest, IntFlagAcceptsAssignmentOfNegativeNumberWithEquals) {
  std::vector<std::string> args = {"--test=-7"};
  int64_t test_flag = 42;
  FlagSet flags;
  flags.Add<int64_t>("test", "some usage", 123, test_flag);
  EXPECT_EQ(test_flag, 123);
  EXPECT_TRUE(flags.Parse(args, nullptr));
  EXPECT_THAT(args, IsEmpty());
  EXPECT_EQ(test_flag, -7);
}

TEST(FlagsParseTest, StringFlagHandlesNoAssignment) {
  std::vector<std::string> args;
  std::string test_flag = "initial value";
  FlagSet flags;
  flags.Add<std::string>("test", "some usage", "default value", test_flag);
  EXPECT_EQ(test_flag, "default value");
  EXPECT_TRUE(flags.Parse(args, nullptr));
  EXPECT_THAT(args, IsEmpty());
  EXPECT_EQ(test_flag, "default value");
}

TEST(FlagsParseTest, StringFlagHandlesAssignmentWithoutValue) {
  std::vector<std::string> args = {"--test"};
  std::string test_flag = "initial value";
  FlagSet flags;
  flags.Add<std::string>("test", "some usage", "default value", test_flag);
  EXPECT_EQ(test_flag, "default value");
  EXPECT_TRUE(flags.Parse(args, nullptr));
  EXPECT_THAT(args, IsEmpty());
  EXPECT_EQ(test_flag, "");
}

TEST(FlagsParseTest, StringFlagHandlesAssignmentWithEqualsWithoutValue) {
  std::vector<std::string> args = {"--test="};
  std::string test_flag = "initial value";
  FlagSet flags;
  flags.Add<std::string>("test", "some usage", "default value", test_flag);
  EXPECT_EQ(test_flag, "default value");
  EXPECT_TRUE(flags.Parse(args, nullptr));
  EXPECT_THAT(args, IsEmpty());
  EXPECT_EQ(test_flag, "");
}

TEST(FlagsParseTest, StringFlagHandlesAssignment) {
  std::vector<std::string> args = {"--test", "hello"};
  std::string test_flag = "initial value";
  FlagSet flags;
  flags.Add<std::string>("test", "some usage", "default value", test_flag);
  EXPECT_EQ(test_flag, "default value");
  EXPECT_TRUE(flags.Parse(args, nullptr));
  EXPECT_THAT(args, IsEmpty());
  EXPECT_EQ(test_flag, "hello");
}

TEST(FlagsParseTest, StringFlagHandlesAssignmentWithEquals) {
  std::vector<std::string> args = {"--test=hello"};
  std::string test_flag = "initial value";
  FlagSet flags;
  flags.Add<std::string>("test", "some usage", "default value", test_flag);
  EXPECT_EQ(test_flag, "default value");
  EXPECT_TRUE(flags.Parse(args, nullptr));
  EXPECT_THAT(args, IsEmpty());
  EXPECT_EQ(test_flag, "hello");
}

TEST(FlagsParseTest, HandlesCombinationOfFlags) {
  const std::vector<std::string> original_args{"abc",
                                               "-int_flag_b=777",
                                               "--bool_flag_b",
                                               "xyz",
                                               "--=nope",
                                               "-",
                                               "remove_me",
                                               "some_arg",
                                               "--fake_flag=fake_value",
                                               "some_other_arg",
                                               "--bool_flag_a=2",
                                               "--int_flag_a",
                                               "555",
                                               "--string_flag_b=",
                                               "123",
                                               "-string_flag_a",
                                               "hype",
                                               "--",
                                               "--int_flag_b=444",
                                               "ijk",
                                               "--bool_flag_a=false",
                                               "yoyo",
                                               "-int_flag_a=nope",
                                               "--string_flag_a"};
  const std::vector<std::string> expected_args{"abc",
                                               "xyz",
                                               "some_arg",
                                               "some_other_arg",
                                               "123",
                                               "--int_flag_b=444",
                                               "ijk",
                                               "--bool_flag_a=false",
                                               "yoyo",
                                               "-int_flag_a=nope",
                                               "--string_flag_a"};
  std::vector<std::string> args = original_args;
  std::stringstream ss;

  bool bool_flag_a = false;
  bool bool_flag_b = true;
  int64_t int_flag_a = 1234;
  int64_t int_flag_b = 6789;
  std::string string_flag_a = "yo";
  std::string string_flag_b = "hey";

  FlagSet flags;
  flags.Add<bool>("bool_flag_a", "bool_flag_a usage", false, bool_flag_a);
  flags.Add<bool>("bool_flag_b", "bool_flag_b usage", false, bool_flag_b);
  flags.Add<int64_t>("int_flag_a", "int_flag_a usage", 111, int_flag_a);
  flags.Add<int64_t>("int_flag_b", "int_flag_b usage", 999, int_flag_b);
  flags.Add<std::string>("string_flag_a", "string_flag_a usage", "sup", string_flag_a);
  flags.Add<std::string>("string_flag_b", "string_flag_b usage", "hi", string_flag_b);

  EXPECT_FALSE(bool_flag_a);
  EXPECT_FALSE(bool_flag_b);
  EXPECT_EQ(int_flag_a, 111);
  EXPECT_EQ(int_flag_b, 999);
  EXPECT_EQ(string_flag_a, "sup");
  EXPECT_EQ(string_flag_b, "hi");

  EXPECT_FALSE(flags.Parse(args, &ss));
  EXPECT_EQ(args, expected_args);
  EXPECT_EQ(ss.str(),
            "missing flag name: --=nope\n"
            "missing flag name: -\n"
            "flag -fake_flag does not exist\n"
            "flag -bool_flag_a does not accept value \"2\"\n");

  EXPECT_FALSE(bool_flag_a);
  EXPECT_TRUE(bool_flag_b);
  EXPECT_EQ(int_flag_a, 555);
  EXPECT_EQ(int_flag_b, 777);
  EXPECT_EQ(string_flag_a, "hype");
  EXPECT_EQ(string_flag_b, "");
}

TEST(FlagsParseTest, HandlesCombinationOfFlagsInNestedFlagSets) {
  const std::vector<std::string> original_args{"abc",
                                               "-int_flag_b=777",
                                               "--bool_flag_b",
                                               "xyz",
                                               "--=nope",
                                               "-",
                                               "remove_me",
                                               "some_arg",
                                               "--fake_flag=fake_value",
                                               "some_other_arg",
                                               "--bool_flag_a=2",
                                               "--int_flag_a",
                                               "555",
                                               "--string_flag_b=",
                                               "123",
                                               "-string_flag_a",
                                               "hype",
                                               "--",
                                               "--int_flag_b=444",
                                               "ijk",
                                               "--bool_flag_a=false",
                                               "yoyo",
                                               "-int_flag_a=nope",
                                               "--string_flag_a"};
  const std::vector<std::string> expected_args{"abc",
                                               "xyz",
                                               "some_arg",
                                               "some_other_arg",
                                               "123",
                                               "--int_flag_b=444",
                                               "ijk",
                                               "--bool_flag_a=false",
                                               "yoyo",
                                               "-int_flag_a=nope",
                                               "--string_flag_a"};
  std::vector<std::string> args = original_args;
  std::stringstream ss;

  bool bool_flag_a = false;
  bool bool_flag_b = true;
  int64_t int_flag_a = 1234;
  int64_t int_flag_b = 6789;
  std::string string_flag_a = "yo";
  std::string string_flag_b = "hey";

  FlagSet parent_flags;
  FlagSet child_flags = parent_flags.CreateChild();
  parent_flags.Add<bool>("bool_flag_a", "bool_flag_a usage", false, bool_flag_a);
  child_flags.Add<bool>("bool_flag_b", "bool_flag_b usage", false, bool_flag_b);
  child_flags.Add<int64_t>("int_flag_a", "int_flag_a usage", 111, int_flag_a);
  parent_flags.Add<int64_t>("int_flag_b", "int_flag_b usage", 999, int_flag_b);
  parent_flags.Add<std::string>("string_flag_a", "string_flag_a usage", "sup", string_flag_a);
  child_flags.Add<std::string>("string_flag_b", "string_flag_b usage", "hi", string_flag_b);

  EXPECT_FALSE(bool_flag_a);
  EXPECT_FALSE(bool_flag_b);
  EXPECT_EQ(int_flag_a, 111);
  EXPECT_EQ(int_flag_b, 999);
  EXPECT_EQ(string_flag_a, "sup");
  EXPECT_EQ(string_flag_b, "hi");

  EXPECT_FALSE(child_flags.Parse(args, &ss));
  EXPECT_EQ(args, expected_args);
  EXPECT_EQ(ss.str(),
            "missing flag name: --=nope\n"
            "missing flag name: -\n"
            "flag -fake_flag does not exist\n"
            "flag -bool_flag_a does not accept value \"2\"\n");

  EXPECT_FALSE(bool_flag_a);
  EXPECT_TRUE(bool_flag_b);
  EXPECT_EQ(int_flag_a, 555);
  EXPECT_EQ(int_flag_b, 777);
  EXPECT_EQ(string_flag_a, "hype");
  EXPECT_EQ(string_flag_b, "");
}

TEST(FlagsTest, PrintsDefaults) {
  bool bool_flag_a;
  bool bool_flag_b;
  int64_t int_flag_a;
  int64_t int_flag_b;
  std::string string_flag_a;
  std::string string_flag_b;

  FlagSet flags;
  flags.Add<bool>("a", "bool_flag_a usage", false, bool_flag_a);
  flags.Add<bool>("bool_flag_b", "bool_flag_b usage\non multiple\nlines", true, bool_flag_b);
  flags.Add<int64_t>("int_flag_a", "int_flag_a usage", 111, int_flag_a);
  flags.Add<int64_t>("x", "int_flag_b usage", 0, int_flag_b);
  flags.Add<std::string>("string_flag_a", "string_flag_a usage", "", string_flag_a);
  flags.Add<std::string>("string_flag_b", "string_flag_b usage", "hi", string_flag_b);

  std::stringstream ss;
  flags.PrintDefaults(&ss);
  EXPECT_EQ(ss.str(),
            "  -a  bool_flag_a usage\n"
            "  -bool_flag_b\n"
            "      bool_flag_b usage\n"
            "      on multiple\n"
            "      lines (default true)\n"
            "  -int_flag_a\n"
            "      int_flag_a usage (default 111)\n"
            "  -string_flag_a\n"
            "      string_flag_a usage\n"
            "  -string_flag_b\n"
            "      string_flag_b usage (default hi)\n"
            "  -x  int_flag_b usage\n");
}

}  // namespace common
