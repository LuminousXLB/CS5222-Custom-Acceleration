# Commands to compile your matrix multiply design
set src_dir "."
open_project accel
set_top mmult_hw
add_files $src_dir/mmult_float.cpp
add_files -tb $src_dir/mmult_test.cpp
open_solution "solution0" -flow_target vivado
set_part {xc7z020clg484-1}
create_clock -period 10 -name default

config_compile -pipeline_loops 0
set_clock_uncertainty 12.5%

csim_design -clean
csynth_design
close_project
exit