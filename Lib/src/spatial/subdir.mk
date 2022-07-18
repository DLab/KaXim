################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/spatial/Channel.cpp \
../src/spatial/Compartment.cpp \
../src/spatial/Transport.cpp \
../src/spatial/Transport_test.cpp 

OBJS += \
./src/spatial/Channel.o \
./src/spatial/Compartment.o \
./src/spatial/Transport.o \
./src/spatial/Transport_test.o 

CPP_DEPS += \
./src/spatial/Channel.d \
./src/spatial/Compartment.d \
./src/spatial/Transport.d \
./src/spatial/Transport_test.d 


# Each subdirectory must supply rules for building sources it contributes
src/spatial/%.o: ../src/spatial/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -I/usr/include/eigen3/ -O3 -Wall -c -fmessage-length=0 -fopenmp -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


