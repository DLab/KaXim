################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/pattern/mixture/Agent.cpp \
../src/pattern/mixture/Component.cpp \
../src/pattern/mixture/Mixture.cpp 

OBJS += \
./src/pattern/mixture/Agent.o \
./src/pattern/mixture/Component.o \
./src/pattern/mixture/Mixture.o 

CPP_DEPS += \
./src/pattern/mixture/Agent.d \
./src/pattern/mixture/Component.d \
./src/pattern/mixture/Mixture.d 


# Each subdirectory must supply rules for building sources it contributes
src/pattern/mixture/%.o: ../src/pattern/mixture/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -I/usr/local/include/eigen3/ -O3 -Wall -c -fmessage-length=0 -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


