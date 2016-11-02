#!/bin/bash

# Clove Run mode: 0 Edge flowlet, 1 Clove-ECN, 2 Clove-INT

cd ../../build/examples/load-balance

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../..

echo "Starting Simulation: $1"

for seed in {21..30}
do
    for CloveRunMode in 0 1
    do
        nohup ./ns3-dev-conga-simulation-large-optimized --ID=$1 --runMode=Clove --cloveRunMode=$CloveRunMode --cloveDisToUncongestedPath=false --StartTime=0 --EndTime=15 --FlowLaunchEndTime=6 --leafCount=4 --spineCount=4 --leafServerCapacity=10  --serverCount=8 --transportProt=DcTcp --cdfFileName=../../../examples/load-balance/VL2_CDF.txt --load=0.8 --randomSeed=$seed > /dev/null 2>&1 &
	done
done

exit
