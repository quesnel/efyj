library(refyj)

infos <- refyj::information("Car.dxi")

test_that("informaiton is correct", {
  expect_equal(length(infos), 2)
  expect_equal(length(infos$basic_attribute_names), 6)
  expect_equal(length(infos$basic_attribute_scale_values), 6)
  expect_equal(infos$basic_attribute_names[1], "BUY.PRICE")
  expect_equal(infos$basic_attribute_names[2], "MAINT.PRICE")
  expect_equal(infos$basic_attribute_names[3], "#PERS")
  expect_equal(infos$basic_attribute_names[4], "#DOORS")
  expect_equal(infos$basic_attribute_names[5], "LUGGAGE")
  expect_equal(infos$basic_attribute_names[6], "SAFETY")
  expect_equal(infos$basic_attribute_scale_values[1], 3)
  expect_equal(infos$basic_attribute_scale_values[2], 3)
  expect_equal(infos$basic_attribute_scale_values[3], 3)
  expect_equal(infos$basic_attribute_scale_values[4], 4)
  expect_equal(infos$basic_attribute_scale_values[5], 3)
  expect_equal(infos$basic_attribute_scale_values[6], 3)
})

