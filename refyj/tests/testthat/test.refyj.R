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

extracts <- refyj::extract("Car.dxi", "Car.csv")

test_that("extract csv data from Car.dxi and Car.csv", {
	expect_equal(extracts$simulations[1], "Car1")
	expect_equal(extracts$simulations[2], "Car2")
	expect_equal(extracts$simulations[3], "Car3")
	expect_equal(extracts$simulations[4], "Car4")
	expect_equal(extracts$simulations[5], "Car5")
	expect_equal(extracts$simulations[6], "Car6")

	expect_equal(extracts$places[1], "Castanet-Tolosan")
	expect_equal(extracts$places[2], "Castanet-Tolosan")
	expect_equal(extracts$places[3], "Castanet-Tolosan")
	expect_equal(extracts$places[4], "Castanet-Tolosan")
	expect_equal(extracts$places[5], "Castanet-Tolosan")
	expect_equal(extracts$places[6], "Castanet-Tolosan")

	expect_equal(extracts$departments[1], 1991)
	expect_equal(extracts$departments[2], 1991)
	expect_equal(extracts$departments[3], 1991)
	expect_equal(extracts$departments[4], 1991)
	expect_equal(extracts$departments[5], 1991)
	expect_equal(extracts$departments[6], 1991)

	expect_equal(extracts$observed[1], 3)
	expect_equal(extracts$observed[2], 2)
	expect_equal(extracts$observed[3], 0)
	expect_equal(extracts$observed[4], 0)
	expect_equal(extracts$observed[5], 3)
	expect_equal(extracts$observed[6], 3)

	to_compare <- c(1,2,2,2,2,2,1,1,2,2,2,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,2,2,2,2,2,2,2,2,2)
	expect_equal(extracts$scale_values, to_compare)
})

ret_eval <- refyj::evaluate("Car.dxi", 
							extracts$simulations,
							extracts$places,
							extracts$departments,
							extracts$years,
							extracts$observed,
							extracts$scale_values)

test_that("evaluate Car.dxi and Car.csv", {
	expect_equal(ret_eval$simulations, ret_eval$observation)
	expect_equal(ret_eval$observation, extracts$observed)
	expect_equal(ret_eval$linear_weighted_kappa, 1.0)
	expect_equal(ret_eval$squared_weighted_kappa, 1.0)
})

ret_adj <- refyj::adjustment("Car.dxi",
							extracts$simulations,
							extracts$places,
							extracts$departments,
							extracts$years,
							extracts$observed,
							extracts$scale_values,
							TRUE,
							4,
							1)

test_that("adjustment Car.dxi and Car.csv", {
	expect_equal(ret_adj$modifiers, c(0,0,0,0,0,0,0,4,1,0,0,0,0,4,1,0,5,1))
	expect_equal(ret_adj$kappa, c(1,1,1,1))
})