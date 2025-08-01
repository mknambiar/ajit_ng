# ajit_ng
AJIT Next Generation Simulator

#Commands to run for compiling

sitar translate ajit_ng.sitar #Translate

sitar compile -d . -d Output -d cpu/include -d tlbs/include -d common/include >  err.txt 2>&1 #compile

at err.txt | more #Check your errors and Debug
