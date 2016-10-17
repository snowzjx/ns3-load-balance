#!/bin/bash

# TLB Run mode: 0 counter, 1 min RTT, 2 random£¬ 11 for rtt+counter, 12 for rtt +dre

cd ../../build/examples/load-balance

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../..

echo "Starting Simulation: $1"

for run in 1
do
    for minRTT in 63
    do
        for T1 in 66
        do
            for seed in {21..30}
            do
                for TLBMode in 12
                do
                    nohup ./ns3-dev-conga-simulation-large-optimized --ID=$1 --runMode=TLB --StartTime=0 --EndTime=15 --FlowLaunchEndTime=6 --leafCount=4 --spineCount=4 --leafServerCapacity=10  --serverCount=8 --TLBRunMode=$TLBMode --TLBSmooth=true --TLBProbingEnable=false --TLBMinRTT=$minRTT --TLBT1=$T1 --TLBRerouting=false --TcpPause=false --TLBProbingInterval=500 --transportProt=DcTcp  --TLBBetterPathRTT=100 --TLBHighRTT=180 --TLBS=640000 --cdfFileName=../../../examples/load-balance/VL2_CDF.txt --load=0.8 --asymCapacity=false --asymCapacityPoss=20 --randomSeed=$seed > /dev/null 2>&1 &
                    # nohup ./ns3-dev-conga-simulation-large-optimized --ID=$1 --runMode=ECMP --StartTime=0 --EndTime=15 --FlowLaunchEndTime=6 --leafCount=4 --spineCount=4 --leafServerCapacity=10  --serverCount=8 --transportProt=DcTcp --cdfFileName=../../../examples/load-balance/DCTCP_CDF.txt --load=0.8   --asymCapacity=false --asymCapacityPoss=20 --randomSeed=$seed > /dev/null 2>&1 &
                done
            done
        done
	done
done

exit
