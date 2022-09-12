################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/bindings/PythonBind.cpp 

OBJS += \
./src/bindings/PythonBind.o 

CPP_DEPS += \
./src/bindings/PythonBind.d 


# Each subdirectory must supply rules for building sources it contributes
src/bindings/%.o: ../src/bindings/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -DDEBUG -I/usr/local/include/eigen3/ -O0 -g3 -p -Wall -c -fmessage-length=0 -fopenmp -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


