//
//  data_view_test.cc
//  Katara-tests
//
//  Created by Arne Philipeit on 8/8/21.
//  Copyright © 2021 Arne Philipeit. All rights reserved.
//

#include "src/common/data_view.h"

#include <array>

#include "gtest/gtest.h"

namespace common {

TEST(DataViewTest, ConstructionAndAccessSucceeds) {
  std::array<uint8_t, 123> data;
  for (std::size_t i = 0; i < data.size(); i++) {
    data[i] = int8_t(i * i);
  }

  DataView data_view_a(data.data(), data.size());
  const DataView data_view_b(data.data(), data.size());

  EXPECT_EQ(data_view_a.base(), data.data());
  EXPECT_EQ(data_view_a.size(), data.size());
  EXPECT_EQ(data_view_a[0], 0);
  EXPECT_EQ(data_view_a[52], 144);
  EXPECT_EQ(data_view_a[122], 36);

  EXPECT_EQ(data_view_b.base(), data.data());
  EXPECT_EQ(data_view_b.size(), data.size());
  EXPECT_EQ(data_view_b[0], 0);
  EXPECT_EQ(data_view_b[52], 144);
  EXPECT_EQ(data_view_b[122], 36);

  data_view_a[0] = 17;
  data_view_a[42] = 27;
  data_view_a[122] = 37;

  EXPECT_EQ(data_view_b[0], 17);
  EXPECT_EQ(data_view_b[42], 27);
  EXPECT_EQ(data_view_b[122], 37);

  EXPECT_EQ(data_view_a[0], 17);
  EXPECT_EQ(data_view_a[42], 27);
  EXPECT_EQ(data_view_a[122], 37);
}

TEST(DataViewTest, ReturnsCorrectSubViews) {
  std::array<uint8_t, 333> data;
  for (std::size_t i = 0; i < data.size(); i++) {
    data[i] = int8_t(i * i);
  }

  DataView data_view_a(data.data(), data.size());
  const DataView data_view_b(data.data(), data.size());

  DataView data_subview_x = data_view_a.SubView(64);
  const DataView data_subview_y = data_view_b.SubView(67, 73);

  EXPECT_EQ(data_subview_x[0], 0);
  EXPECT_EQ(data_subview_x[1], 129);
  EXPECT_EQ(data_subview_x[2], 4);
  EXPECT_EQ(data_subview_x[3], 137);
  EXPECT_EQ(data_subview_x[4], 16);
  EXPECT_EQ(data_subview_x[5], 153);
  EXPECT_EQ(data_subview_x[8], 64);
  EXPECT_EQ(data_subview_x[268], 144);

  EXPECT_EQ(data_subview_y[0], 137);
  EXPECT_EQ(data_subview_y[1], 16);
  EXPECT_EQ(data_subview_y[2], 153);
  EXPECT_EQ(data_subview_y[5], 64);

  data_subview_x[4] = 111;

  EXPECT_EQ(data_subview_y[1], 111);
  EXPECT_EQ(data_view_b[68], 111);
  EXPECT_EQ(data_view_a[68], 111);
  EXPECT_EQ(data_subview_x[4], 111);
}

}  // namespace common
