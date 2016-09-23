#!/bin/bash

cd ../..

for run in 1
do
    for minRTT in 10 60 100
    do
        for T1 in 50 200
        do
            for probingInterval in 50 200
            do
                for TLBMode in 0 1 2
                do
                    nohup ./waf --run "conga-simulation-large --runMode=TLB --TLBRunMode=$TLBMode --TLBMinRTT=$minRTT --TLBT1=$T1 --TLBProbingInterval=$probingInterval --transportProt=DcTcp --cdfFileName=examples/load-balance/DCTCP_CDF.txt --load=0.8 --randomSeed=1" > /dev/null 2>&1 &
                done
            done
        done
	done
done

exit
