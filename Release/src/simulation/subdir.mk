################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/simulation/Counter.cpp \
../src/simulation/Parameters.cpp \
../src/simulation/Perturbation.cpp \
../src/simulation/Plot.cpp \
../src/simulation/Results.cpp \
../src/simulation/Results_test.cpp \
../src/simulation/Rule.cpp \
../src/simulation/Simulation.cpp 

OBJS += \
./src/simulation/Counter.o \
./src/simulation/Parameters.o \
./src/simulation/Perturbation.o \
./src/simulation/Plot.o \
./src/simulation/Results.o \
./src/simulation/Results_test.o \
./src/simulation/Rule.o \
./src/simulation/Simulation.o 

CPP_DEPS += \
./src/simulation/Counter.d \
./src/simulation/Parameters.d \
./src/simulation/Perturbation.d \
./src/simulation/Plot.d \
./src/simulation/Results.d \
./src/simulation/Results_test.d \
./src/simulation/Rule.d \
./src/simulation/Simulation.d 


# Each subdirectory must supply rules for building sources it contributes
src/simulation/%.o: ../src/simulation/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -I/usr/include/eigen3/ -O3 -Wall -c -fmessage-length=0 -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


