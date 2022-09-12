################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/util/Exceptions.cpp \
../src/util/Warning.cpp 

OBJS += \
./src/util/Exceptions.o \
./src/util/Warning.o 

CPP_DEPS += \
./src/util/Exceptions.d \
./src/util/Warning.d 


# Each subdirectory must supply rules for building sources it contributes
src/util/%.o: ../src/util/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -I/usr/local/include/eigen3/ -O3 -Wall -c -fmessage-length=0 -fopenmp -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


