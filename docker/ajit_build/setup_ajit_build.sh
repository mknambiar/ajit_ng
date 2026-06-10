#!/usr/bin/env bash

export TERM=xterm;

# cd $AJIT_HOME; # see ../ajit_base/Dockerfile

echo "################################################";
echo "  Building with the system SPARC GNU toolchain.";
echo "################################################";
sleep 3;

echo "################################################";
echo "  Building Tools, C Model and debug monitors.";
echo "################################################";
sleep 3;
bash $AJIT_HOME/scripts/build_ajit_public_resources.sh;


#echo "################################################";
#echo "  Building Ajit Tools/Scripts.";
#echo "################################################";
sleep 3;
#bash $AJIT_HOME/scripts/build_tools.sh;

