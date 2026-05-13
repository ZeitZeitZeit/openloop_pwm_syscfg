################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
build-799030566: ../epwm_sync_svm.syscfg
	@echo 'SysConfig - building file: "$<"'
	"C:/ti/ccs2050/ccs/utils/sysconfig_1.27.0/sysconfig_cli.bat" -s "C:/ti/c2000/C2000Ware_26_00_00_00/.metadata/sdk.json" -d "F28004x" -p "F28004x_100PZ" -r "F28004x_100PZ" --script "C:/Users/giv6hc/workspace_ccstheia/epwm_svm/epwm_sync_svm.syscfg" -o "syscfg" --compiler ccs
	@echo 'Finished building: "$<"'
	@echo ' '

syscfg/board.c: build-799030566 ../epwm_sync_svm.syscfg
syscfg/board.h: build-799030566
syscfg/board.cmd.genlibs: build-799030566
syscfg/board.opt: build-799030566
syscfg/board.json: build-799030566
syscfg/pinmux.csv: build-799030566
syscfg/epwm.dot: build-799030566
syscfg/c2000ware_libraries.cmd.genlibs: build-799030566
syscfg/c2000ware_libraries.opt: build-799030566
syscfg/c2000ware_libraries.c: build-799030566
syscfg/c2000ware_libraries.h: build-799030566
syscfg/clocktree.h: build-799030566
syscfg/transfer.opt: build-799030566
syscfg/transfer_utils.h: build-799030566
syscfg/transfer_utils.c: build-799030566
syscfg/export/export.c: build-799030566
syscfg/export/export.h: build-799030566
syscfg/export/export_package.c: build-799030566
syscfg/export/export_package.h: build-799030566
syscfg/gui_setup.bat: build-799030566
syscfg/signalsight/signalsight.c: build-799030566
syscfg/signalsight/signalsight.h: build-799030566
syscfg/signalsight/signalsight_hash.c: build-799030566
syscfg/signalsight/signalsight_hash.h: build-799030566
syscfg/signalsight/gui/signalsight_hash.json: build-799030566
syscfg/signalsight/gui/index.html: build-799030566
syscfg/signalsight/gui/project.json: build-799030566
syscfg/signalsight/gui/package.json: build-799030566
syscfg: build-799030566

syscfg/%.obj: ./syscfg/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'C2000 Compiler - building file: "$<"'
	"C:/ti/ccs2050/ccs/tools/compiler/ti-cgt-c2000_25.11.0.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu0 -Ooff --fp_mode=relaxed --include_path="C:/Users/giv6hc/workspace_ccstheia/epwm_svm" --include_path="C:/ti/c2000/C2000Ware_26_00_00_00" --include_path="C:/Users/giv6hc/workspace_ccstheia/epwm_svm/device" --include_path="C:/ti/c2000/C2000Ware_26_00_00_00/driverlib/f28004x/driverlib/" --include_path="C:/ti/ccs2050/ccs/tools/compiler/ti-cgt-c2000_25.11.0.LTS/include" --define=DEBUG --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="syscfg/$(basename $(<F)).d_raw" --include_path="C:/Users/giv6hc/workspace_ccstheia/epwm_svm/CPU1_RAM/syscfg" --obj_directory="syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

syscfg/export/%.obj: ./syscfg/export/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'C2000 Compiler - building file: "$<"'
	"C:/ti/ccs2050/ccs/tools/compiler/ti-cgt-c2000_25.11.0.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu0 -Ooff --fp_mode=relaxed --include_path="C:/Users/giv6hc/workspace_ccstheia/epwm_svm" --include_path="C:/ti/c2000/C2000Ware_26_00_00_00" --include_path="C:/Users/giv6hc/workspace_ccstheia/epwm_svm/device" --include_path="C:/ti/c2000/C2000Ware_26_00_00_00/driverlib/f28004x/driverlib/" --include_path="C:/ti/ccs2050/ccs/tools/compiler/ti-cgt-c2000_25.11.0.LTS/include" --define=DEBUG --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="syscfg/export/$(basename $(<F)).d_raw" --include_path="C:/Users/giv6hc/workspace_ccstheia/epwm_svm/CPU1_RAM/syscfg" --obj_directory="syscfg/export" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

syscfg/signalsight/%.obj: ./syscfg/signalsight/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'C2000 Compiler - building file: "$<"'
	"C:/ti/ccs2050/ccs/tools/compiler/ti-cgt-c2000_25.11.0.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu0 -Ooff --fp_mode=relaxed --include_path="C:/Users/giv6hc/workspace_ccstheia/epwm_svm" --include_path="C:/ti/c2000/C2000Ware_26_00_00_00" --include_path="C:/Users/giv6hc/workspace_ccstheia/epwm_svm/device" --include_path="C:/ti/c2000/C2000Ware_26_00_00_00/driverlib/f28004x/driverlib/" --include_path="C:/ti/ccs2050/ccs/tools/compiler/ti-cgt-c2000_25.11.0.LTS/include" --define=DEBUG --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="syscfg/signalsight/$(basename $(<F)).d_raw" --include_path="C:/Users/giv6hc/workspace_ccstheia/epwm_svm/CPU1_RAM/syscfg" --obj_directory="syscfg/signalsight" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'C2000 Compiler - building file: "$<"'
	"C:/ti/ccs2050/ccs/tools/compiler/ti-cgt-c2000_25.11.0.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla2 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu0 -Ooff --fp_mode=relaxed --include_path="C:/Users/giv6hc/workspace_ccstheia/epwm_svm" --include_path="C:/ti/c2000/C2000Ware_26_00_00_00" --include_path="C:/Users/giv6hc/workspace_ccstheia/epwm_svm/device" --include_path="C:/ti/c2000/C2000Ware_26_00_00_00/driverlib/f28004x/driverlib/" --include_path="C:/ti/ccs2050/ccs/tools/compiler/ti-cgt-c2000_25.11.0.LTS/include" --define=DEBUG --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --gen_func_subsections=on --abi=eabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" --include_path="C:/Users/giv6hc/workspace_ccstheia/epwm_svm/CPU1_RAM/syscfg" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


