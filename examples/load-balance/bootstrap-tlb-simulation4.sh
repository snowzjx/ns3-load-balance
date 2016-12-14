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
            for seed in  {21..25}
            do
                for TLBMode in 12
                do
                  for Ld in 0.8
                   do
                    for trans in "DcTcp"
                      do
                       for workload in "VL2" #GS FB
                        do
                          for offset in 0 # 20 40
                            do
                             for offset2 in 0 # 20 80 
                               do
                              for base in 10 # 60 100 200
                                do              
 nohup ./ns3-dev-conga-simulation-large-optimized --ID=$1 --runMode=TLB --StartTime=0 --EndTime=7 --FlowLaunchEndTime=2 --leafCount=8 --spineCount=8 --leafServerCapacity=10  --serverCount=16 --TLBRunMode=$TLBMode --TLBSmooth=true --TLBProbingEnable=true --TLBMinRTT=$minRTT --TLBT1=$T1 --TLBRerouting=true --TcpPause=false --TLBProbingInterval=500 --transportProt=$trans  --TLBBetterPathRTT=$((100+$offset+$offset2)) --TLBHighRTT=$((180+$offset2)) --TLBS=640000 --cdfFileName=../../../examples/load-balance/${workload}_CDF.txt --load=$Ld --quantifyRTTBase=$base  --asymCapacity=false --asymCapacityPoss=20 --randomSeed=$seed > /dev/null 2>&1 &
# nohup ./ns3-dev-conga-simulation-large-optimized --ID=$1 --runMode=TLB --StartTime=0 --EndTime=0.5 --FlowLaunchEndTime=0.001 --leafCount=8 --spineCount=8 --leafServerCapacity=10  --serverCount=16 --TLBRunMode=$TLBMode --TLBSmooth=true --TLBProbingEnable=true --TLBMinRTT=$minRTT --TLBT1=$T1 --TLBRerouting=true  --TcpPause=false --TLBProbingInterval=500 --transportProt=$trans  --TLBBetterPathRTT=$((100+$offset)) --TLBHighRTT=$((180+$offset)) --TLBS=640000 --cdfFileName=../../../examples/load-balance/${workload}_CDF.txt --load=$Ld --asymCapacity=false --asymCapacityPoss=20 --randomSeed=$seed > /dev/null 2>&1 &

#                nohup ./ns3-dev-conga-simulation-large-optimized --ID=$1 --runMode=Presto --resequenceBuffer=false --StartTime=0 --EndTime=7 --FlowLaunchEndTime=2 --leafCount=8 --spineCount=8 --leafServerCapacity=10  --serverCount=16 --transportProt=$trans --cdfFileName=../../../examples/load-balance/$workload_CDF.txt --load=$Ld --asymCapacity=false --asymCapacityPoss=20 --randomSeed=$seed > /dev/null 2>&1 &
#               nohup ./ns3-dev-conga-simulation-large-optimized --ID=$1 --runMode=ECMP  --StartTime=0 --EndTime=7 --FlowLaunchEndTime=2 --leafCount=8 --spineCount=8 --leafServerCapacity=10  --serverCount=16 --transportProt=$trans --cdfFileName=../../../examples/load-balance/${workload}_CDF.txt --load=$Ld --asymCapacity=false --asymCapacityPoss=20 --randomSeed=$seed > /dev/null 2>&1 &
    #              nohup ./ns3-dev-conga-simulation-large-optimized --ID=$1 --runMode=Conga  --StartTime=0 --EndTime=7 --FlowLaunchEndTime=2 --leafCount=8 --spineCount=8 --leafServerCapacity=10  --serverCount=16 --transportProt=$trans --cdfFileName=../../../examples/load-balance/${workload}_CDF.txt --load=$Ld --asymCapacity=false --asymCapacityPoss=20 --randomSeed=$seed > /dev/null 2>&1 &
# nohup ./ns3-dev-conga-simulation-large-optimized --ID=$1 --runMode=Conga-flow  --StartTime=0 --EndTime=7 --FlowLaunchEndTime=2 --leafCount=8     --spineCount=8 --leafServerCapacity=10  --serverCount=16 --transportProt=$trans --cdfFileName=../../../examples/load-balance/${workload}_CDF.txt --load=$Ld --asymCapacity=false --asymCapacityPoss=20 --randomSeed=$seed > /dev/null 2>&1 &
					   done
					done	
				done
            done
        done
	done
done
done
done
done
done
exit

