#!/bin/bash

# Clove Run mode: 0 Edge flowlet, 1 Clove-ECN, 2 Clove-INT

cd ../../build/examples/load-balance

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../..

echo "Starting Simulation: $1"

for seed in {21..25}
do
    for CloveRunMode in 0  1
    do
        for Ld in 0.4 0.6
        do
           nohup ./ns3-dev-conga-simulation-large-optimized --ID=$1 --runMode=Clove --cloveRunMode=$CloveRunMode --cloveDisToUncongestedPath=false --StartTime=0 --EndTime=7 --FlowLaunchEndTime=2 --leafCount=8 --spineCount=8 --leafServerCapacity=10  --serverCount=16 --transportProt=DcTcp --cdfFileName=../../../examples/load-balance/DCTCP_CDF.txt --load=$Ld --randomSeed=$seed > /dev/null 2>&1 &
 #          nohup ./ns3-dev-conga-simulation-large-optimized --ID=$1 --runMode=Clove --cloveRunMode=$CloveRunMode --cloveDisToUncongestedPath=true --StartTime=0 --EndTime=7 --FlowLaunchEndTime=2 --leafCount=8 --spineCount=8 --leafServerCapacity=10  --serverCount=16 --transportProt=DcTcp --cdfFileName=../../../examples/load-balance/VL2_CDF.txt --load=$Ld --randomSeed=$seed > /dev/null 2>&1 &
        #  nohup ./ns3-dev-conga-simulation-large-optimized --ID=$1 --runMode=TLB --StartTime=0 --EndTime=3 --FlowLaunchEndTime=0.5 --leafCount=8 --spineCount=8 --leafServerCapacity=10  --serverCount=16 --TLBRunMode=12 --TLBSmooth=true --TLBProbingEnable=true --TLBMinRTT=63 --TLBT1=66 --TLBRerouting=true --TcpPause=false --TLBProbingInterval=500 --transportProt=DcTcp  --TLBBetterPathRTT=100 --TLBHighRTT=180 --TLBS=640000 --cdfFileName=../../../examples/load-balance/VL2_CDF.txt --load=$Ld --asymCapacity=false --asymCapacityPoss=20 --randomSeed=$seed > /dev/null 2>&1 &
 #          nohup ./ns3-dev-conga-simulation-large-optimized --ID=$1 --runMode=ECMP --StartTime=0 --EndTime=3 --FlowLaunchEndTime=0.5 --leafCount=8 --spineCount=8 --leafServerCapacity=10  --serverCount=16 --transportProt=DcTcp --cdfFileName=../../../examples/load-balance/VL2_CDF.txt --load=$Ld --asymCapacity=false --asymCapacityPoss=20 --randomSeed=$seed > /dev/null 2>&1 &
  #         nohup ./ns3-dev-conga-simulation-large-optimized --ID=$1 --runMode=Conga --StartTime=0 --EndTime=3 --FlowLaunchEndTime=0.5 --leafCount=8 --spineCount=8 --leafServerCapacity=10  --serverCount=16 --transportProt=DcTcp --cdfFileName=../../../examples/load-balance/VL2_CDF.txt --load=$Ld --asymCapacity=false --asymCapacityPoss=20 --randomSeed=$seed > /dev/null 2>&1 &
        done
    done
done
exit

