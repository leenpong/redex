/**
 * Copyright (c) 2016-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include <gtest/gtest.h>

#include "AliasedRegisters.h"
#include <boost/optional.hpp>
#include <unordered_map>

using namespace aliased_registers;

Value zero = Value::create_register(0);
Value one = Value::create_register(1);
Value two = Value::create_register(2);
Value three = Value::create_register(3);
Value four = Value::create_register(4);

Value one_lit = Value::create_literal(1);

TEST(AliasedRegistersTest, identity) {
  AliasedRegisters a;
  EXPECT_TRUE(a.are_aliases(zero, zero));
  EXPECT_TRUE(a.are_aliases(one, one));
}

TEST(AliasedRegistersTest, simpleMake) {
  AliasedRegisters a;

  a.add_edge(zero, one);

  EXPECT_TRUE(a.are_aliases(zero, zero));
  EXPECT_TRUE(a.are_aliases(zero, one));
  EXPECT_TRUE(a.are_aliases(one, one));

  EXPECT_FALSE(a.are_aliases(zero, two));
  EXPECT_FALSE(a.are_aliases(one, two));
}

TEST(AliasedRegistersTest, makeBreakLow) {
  AliasedRegisters a;

  a.add_edge(zero, one);
  EXPECT_TRUE(a.are_aliases(zero, one));

  a.break_alias(zero);
  EXPECT_FALSE(a.are_aliases(zero, one));
}

TEST(AliasedRegistersTest, makeBreakHigh) {
  AliasedRegisters a;

  a.add_edge(zero, one);
  EXPECT_TRUE(a.are_aliases(zero, one));

  a.break_alias(one);
  EXPECT_FALSE(a.are_aliases(zero, one));
}

TEST(AliasedRegistersTest, transitiveBreakFirst) {
  AliasedRegisters a;

  a.add_edge(zero, one);
  a.move(two, one);
  EXPECT_TRUE(a.are_aliases(zero, two));

  a.break_alias(zero);
  EXPECT_FALSE(a.are_aliases(zero, two));
  EXPECT_TRUE(a.are_aliases(one, two));
}

TEST(AliasedRegistersTest, transitiveBreakMiddle) {
  AliasedRegisters a;

  a.add_edge(zero, one);
  a.move(two, one);
  EXPECT_TRUE(a.are_aliases(zero, two));

  a.break_alias(one);
  EXPECT_TRUE(a.are_aliases(zero, two));
}

TEST(AliasedRegistersTest, transitiveBreakEnd) {
  AliasedRegisters a;

  a.add_edge(zero, one);
  a.move(two, one);
  EXPECT_TRUE(a.are_aliases(zero, two));

  a.break_alias(two);
  EXPECT_FALSE(a.are_aliases(zero, two));
  EXPECT_TRUE(a.are_aliases(zero, one));
}

TEST(AliasedRegistersTest, transitiveTwoStep) {
  AliasedRegisters a;

  a.move(zero, one);
  a.move(two, one);
  a.move(three, two);

  EXPECT_TRUE(a.are_aliases(zero, three));
  EXPECT_TRUE(a.are_aliases(zero, two));
  EXPECT_TRUE(a.are_aliases(zero, one));

  EXPECT_TRUE(a.are_aliases(one, zero));
  EXPECT_TRUE(a.are_aliases(one, two));
  EXPECT_TRUE(a.are_aliases(one, three));

  EXPECT_TRUE(a.are_aliases(two, zero));
  EXPECT_TRUE(a.are_aliases(two, one));
  EXPECT_TRUE(a.are_aliases(two, three));

  EXPECT_TRUE(a.are_aliases(three, zero));
  EXPECT_TRUE(a.are_aliases(three, one));
  EXPECT_TRUE(a.are_aliases(three, two));

  a.break_alias(two);

  EXPECT_TRUE(a.are_aliases(zero, one));
  EXPECT_TRUE(a.are_aliases(one, zero));
}

TEST(AliasedRegistersTest, transitiveCycleBreak) {
  AliasedRegisters a;

  a.move(zero, one);
  a.move(two, one);
  a.move(three, two);
  a.move(three, zero);

  EXPECT_TRUE(a.are_aliases(zero, three));
  EXPECT_TRUE(a.are_aliases(zero, two));
  EXPECT_TRUE(a.are_aliases(zero, one));

  EXPECT_TRUE(a.are_aliases(one, zero));
  EXPECT_TRUE(a.are_aliases(one, two));
  EXPECT_TRUE(a.are_aliases(one, three));

  EXPECT_TRUE(a.are_aliases(two, zero));
  EXPECT_TRUE(a.are_aliases(two, one));
  EXPECT_TRUE(a.are_aliases(two, three));

  EXPECT_TRUE(a.are_aliases(three, zero));
  EXPECT_TRUE(a.are_aliases(three, one));
  EXPECT_TRUE(a.are_aliases(three, two));

  a.break_alias(two);

  EXPECT_TRUE(a.are_aliases(zero, one));
  EXPECT_TRUE(a.are_aliases(one, zero));

  EXPECT_TRUE(a.are_aliases(zero, three));
  EXPECT_TRUE(a.are_aliases(three, zero));

  EXPECT_TRUE(a.are_aliases(one, three));
  EXPECT_TRUE(a.are_aliases(three, one));
}

TEST(AliasedRegistersTest, getRepresentative) {
  AliasedRegisters a;
  a.add_edge(zero, one);
  Register zero_rep = a.get_representative(zero);
  Register one_rep = a.get_representative(one);
  EXPECT_EQ(0, zero_rep);
  EXPECT_EQ(0, one_rep);
}

TEST(AliasedRegistersTest, getRepresentativeTwoLinks) {
  AliasedRegisters a;
  a.add_edge(zero, one);
  a.add_edge(one, two);
  Register zero_rep = a.get_representative(zero);
  Register one_rep = a.get_representative(one);
  Register two_rep = a.get_representative(one);
  EXPECT_EQ(0, zero_rep);
  EXPECT_EQ(0, one_rep);
  EXPECT_EQ(0, two_rep);
}

TEST(AliasedRegistersTest, breakLineGraph) {
  AliasedRegisters a;
  a.add_edge(zero, one);
  a.move(two, one);
  a.break_alias(one);
  EXPECT_TRUE(a.are_aliases(zero, two));

  a.clear();
  a.move(one, two);
  a.move(zero, one);
  a.break_alias(one);
  EXPECT_TRUE(a.are_aliases(zero, two));
  EXPECT_TRUE(a.are_aliases(two, zero));
  EXPECT_FALSE(a.are_aliases(one, two));
  EXPECT_FALSE(a.are_aliases(one, zero));
}

TEST(AliasedRegistersTest, getRepresentativeNone) {
  AliasedRegisters a;
  Register zero_rep = a.get_representative(zero);
  EXPECT_EQ(0, zero_rep);
}

TEST(AliasedRegistersTest, getRepresentativeTwoComponents) {
  AliasedRegisters a;
  a.add_edge(zero, one);
  a.add_edge(two, three);

  Register zero_rep = a.get_representative(zero);
  Register one_rep = a.get_representative(one);
  EXPECT_EQ(0, zero_rep);
  EXPECT_EQ(0, one_rep);

  Register two_rep = a.get_representative(two);
  Register three_rep = a.get_representative(three);
  EXPECT_EQ(2, two_rep);
  EXPECT_EQ(2, three_rep);
}

TEST(AliasedRegistersTest, getRepresentativeNoLits) {
  AliasedRegisters a;
  a.add_edge(two, one_lit);
  auto two_rep = a.get_representative(two);
  EXPECT_EQ(2, two_rep);
}

TEST(AliasedRegistersTest, AbstractValueLeq) {
  AliasedRegisters a;
  AliasedRegisters b;
  EXPECT_TRUE(a.leq(b));
  EXPECT_TRUE(b.leq(a));

  a.add_edge(zero, one);
  b.add_edge(zero, one);

  EXPECT_TRUE(a.leq(b));

  b.add_edge(zero, two);
  EXPECT_FALSE(a.leq(b));
  EXPECT_TRUE(b.leq(a));
}

TEST(AliasedRegistersTest, AbstractValueLeqAndNotEqual) {
  AliasedRegisters a;
  AliasedRegisters b;

  a.add_edge(zero, one);
  b.add_edge(two, three);

  EXPECT_FALSE(a.leq(b));
  EXPECT_FALSE(b.leq(a));
  EXPECT_FALSE(a.equals(b));
  EXPECT_FALSE(b.equals(a));
}

TEST(AliasedRegistersTest, AbstractValueEquals) {
  AliasedRegisters a;
  AliasedRegisters b;
  EXPECT_TRUE(a.equals(b));
  EXPECT_TRUE(b.equals(a));

  a.add_edge(zero, one);
  b.add_edge(zero, one);

  EXPECT_TRUE(a.equals(b));
  EXPECT_TRUE(b.equals(a));

  b.add_edge(zero, two);
  EXPECT_FALSE(a.equals(b));
  EXPECT_FALSE(b.equals(a));
}

TEST(AliasedRegistersTest, AbstractValueEqualsAndClear) {
  AliasedRegisters a;
  AliasedRegisters b;
  EXPECT_TRUE(a.equals(b));

  a.add_edge(zero, one);
  b.add_edge(zero, one);

  EXPECT_TRUE(a.equals(b));

  b.clear();
  EXPECT_TRUE(a.equals(a));
  EXPECT_TRUE(b.equals(b));
  EXPECT_FALSE(a.equals(b));
}

TEST(AliasedRegistersTest, AbstractValueMeet) {
  AliasedRegisters a;
  AliasedRegisters b;

  a.add_edge(zero, one);
  b.add_edge(one, two);

  a.meet_with(b);

  EXPECT_TRUE(a.are_aliases(zero, two));
  EXPECT_FALSE(a.are_aliases(zero, three));

  EXPECT_FALSE(b.are_aliases(zero, one));
  EXPECT_TRUE(b.are_aliases(one, two));
  EXPECT_FALSE(b.are_aliases(zero, two));
  EXPECT_FALSE(b.are_aliases(zero, three));
}

TEST(AliasedRegistersTest, AbstractValueJoinNone) {
  AliasedRegisters a;
  AliasedRegisters b;

  a.add_edge(zero, one);
  b.add_edge(one, two);

  a.join_with(b);

  EXPECT_FALSE(a.are_aliases(zero, one));
  EXPECT_FALSE(a.are_aliases(one, two));
  EXPECT_FALSE(a.are_aliases(zero, two));
  EXPECT_FALSE(a.are_aliases(zero, three));
}

TEST(AliasedRegistersTest, AbstractValueJoinSome) {
  AliasedRegisters a;
  AliasedRegisters b;

  a.add_edge(zero, one);
  b.add_edge(zero, one);
  b.move(two, one);

  a.join_with(b);

  EXPECT_TRUE(a.are_aliases(zero, one));
  EXPECT_FALSE(a.are_aliases(one, two));
  EXPECT_FALSE(a.are_aliases(zero, two));
  EXPECT_FALSE(a.are_aliases(zero, three));

  EXPECT_TRUE(b.are_aliases(zero, one));
  EXPECT_TRUE(b.are_aliases(one, two));
  EXPECT_TRUE(b.are_aliases(zero, two));
  EXPECT_FALSE(b.are_aliases(zero, three));
}

TEST(AliasedRegistersTest, AbstractValueJoin) {
  AliasedRegisters a;
  AliasedRegisters b;

  a.add_edge(zero, one);
  a.move(two, zero);
  a.move(three, zero);

  b.add_edge(four, one);
  b.move(two, four);
  b.move(three, four);

  a.join_with(b);

  EXPECT_TRUE(a.are_aliases(one, two));
  EXPECT_TRUE(a.are_aliases(one, three));
  EXPECT_TRUE(a.are_aliases(two, three));

  EXPECT_FALSE(a.are_aliases(zero, one));
  EXPECT_FALSE(a.are_aliases(zero, two));
  EXPECT_FALSE(a.are_aliases(zero, three));
  EXPECT_FALSE(a.are_aliases(zero, four));

  EXPECT_FALSE(a.are_aliases(four, one));
  EXPECT_FALSE(a.are_aliases(four, two));
  EXPECT_FALSE(a.are_aliases(four, three));
}
