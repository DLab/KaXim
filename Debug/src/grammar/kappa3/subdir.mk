################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/grammar/kappa3/Lexer.cpp \
../src/grammar/kappa3/Parser.cpp 

OBJS += \
./src/grammar/kappa3/Lexer.o \
./src/grammar/kappa3/Parser.o 

CPP_DEPS += \
./src/grammar/kappa3/Lexer.d \
./src/grammar/kappa3/Parser.d 


# Each subdirectory must supply rules for building sources it contributes
src/grammar/kappa3/%.o: ../src/grammar/kappa3/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	g++ -DDEBUG -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


