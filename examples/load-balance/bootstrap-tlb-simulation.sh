#!/bin/bash

cd ../..

for run in 1
do

    for minRTT in 50 70 100
    do
        for T1 in 100 200 300
        do
            for ecnPortionL in 0.1 0.3
            do
                nohup ./waf --run "conga-simulation-large --runMode=TLB --TLBMinRTT=$minRTT --TLBT1=$T1 --TLBECNPortionLow=$ecnPortionL --transportProt=DcTcp --cdfFileName=examples/load-balance/DCTCP_CDF.txt --load=0.8 --randomSeed=1" > /dev/null 2>&1
           done
		done
	done
done

exit
