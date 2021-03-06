#include "../src/contour.hpp"

#include <gtest/gtest.h>

using namespace std;

class ContourTest : public ::testing::Test {
 protected:
  Contour contour_;
};

TEST_F(ContourTest, Initialization) {
  EXPECT_EQ(0, contour_.max_x());
  EXPECT_EQ(0, contour_.max_y());
}

TEST_F(ContourTest, Update) {
  contour_.Print();
  contour_.Update(0, 1.0, 1.0);
  contour_.Print();
  EXPECT_EQ(1.0, contour_.max_x());
  EXPECT_EQ(1.0, contour_.max_y());
  contour_.Update(1.0, 2.0, 3.0);
  contour_.Print();
  EXPECT_EQ(3.0, contour_.max_x());
  EXPECT_EQ(3.0, contour_.max_y());
  contour_.Update(3.0, 4.0, 5.0);
  contour_.Print();
  EXPECT_EQ(7.0, contour_.max_x());
  EXPECT_EQ(5.0, contour_.max_y());
  contour_.Update(1.0, 7.0, 2.0);
  contour_.Print();
  EXPECT_EQ(8.0, contour_.max_x());
  EXPECT_EQ(7.0, contour_.max_y());
  contour_.Update(8.0, 2.0, 1.0);
  contour_.Print();
  EXPECT_EQ(10.0, contour_.max_x());
  EXPECT_EQ(7.0, contour_.max_y());
  contour_.Update(10.0, 4.0, 7.0);
  contour_.Print();
  EXPECT_EQ(14.0, contour_.max_x());
  EXPECT_EQ(7.0, contour_.max_y());
  contour_.Update(8.0, 2.0, 6.0);
  contour_.Print();
  EXPECT_EQ(14.0, contour_.max_x());
  EXPECT_EQ(7.0, contour_.max_y());
}
