################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
pwm/svm/%.obj: ../pwm/svm/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'C2000 Compiler - building file: "$<"'
	"C:/ti/ccs2050/ccs/tools/compiler/ti-cgt-c2000_25.11.0.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu0 -Ooff --fp_mode=relaxed --include_path="C:/Users/giv6hc/workspace_ccstheia/epwm_svm" --include_path="C:/ti/c2000/C2000Ware_26_00_00_00" --include_path="C:/Users/giv6hc/workspace_ccstheia/epwm_svm/device" --include_path="C:/ti/c2000/C2000Ware_26_00_00_00/driverlib/f28004x/driverlib/" --include_path="C:/ti/ccs2050/ccs/tools/compiler/ti-cgt-c2000_25.11.0.LTS/include" --define=DEBUG --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="pwm/svm/$(basename $(<F)).d_raw" --include_path="C:/Users/giv6hc/workspace_ccstheia/epwm_svm/CPU1_RAM/syscfg" --obj_directory="pwm/svm" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


