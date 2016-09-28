#!/bin/bash

# TLB Run mode: 0 counter, 1 min RTT, 2 random£¬ 11 for rtt+counter, 12 for rtt +dre

cd ../../build/examples/load-balance

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../..

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
                       nohup ./ns3-dev-conga-simulation-large-optimized --runMode=TLB --leafCount=4 --spineCount=4 --leafServerCapacity=10  --serverCount=4 --TLBRunMode=$TLBMode --TLBSmooth=true --TLBProbingEnable=true --TLBMinRTT=$minRTT --TLBT1=$T1 --TLBProbingInterval=51 --transportProt=DcTcp --cdfFileName=../../../examples/load-balance/DCTCP_CDF.txt --load=0.8 --randomSeed=$seed > /dev/null 2>&1 &
                       nohup ./ns3-dev-conga-simulation-large-optimized --runMode=ECMP --leafCount=4 --spineCount=4 --leafServerCapacity=10  --serverCount=4 --transportProt=DcTcp --cdfFileName=../../../examples/load-balance/DCTCP_CDF.txt --load=0.8 --randomSeed=$seed > /dev/null 2>&1 &
                done
            done
        done
	done
done

exit
