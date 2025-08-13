# ajit_ng
AJIT Next Generation Simulator

#Commands to run for compiling

sitar translate ajit_ng.sitar #Translate

sitar compile -d . -d Output -d cpu/src -d cpu_interface/src -d tlbs/src -d functionLibrary/src -d half_precision_float/src -d half_precision_float/aa2clib/src >  err.txt 2>&1 #compile

cat err.txt | more #Check your errors and Debug
