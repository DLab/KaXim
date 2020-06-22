################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/matching/InjRandSet.cpp \
../src/matching/Injection.cpp 

OBJS += \
./src/matching/InjRandSet.o \
./src/matching/Injection.o 

CPP_DEPS += \
./src/matching/InjRandSet.d \
./src/matching/Injection.d 


# Each subdirectory must supply rules for building sources it contributes
src/matching/%.o: ../src/matching/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -DDEBUG -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


