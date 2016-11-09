library(refyj)

model <- refyj::extract_model("Car.dxi")
model

evaluation  <- refyj::evaluate("Car.dxi", "Car.csv")
evaluation
