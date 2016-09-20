#!/bin/bash

cd ../..

for run in 1
do

    for minRTT in 40 70 100
    do
        for pathBetterRTT in 100 200 300
        do
            for Poss in 30 60 90
            do
                nohup ./waf --run "conga-simulation-large --runMode=TLB --TLBMinRTT=$minRTT --TLBBetterPathRTT=$pathBetterRTT --TLBPoss=$Poss --transportProt=DcTcp --cdfFileName=examples/load-balance/DCTCP_CDF.txt --load=0.8 --randomSeed=1" > /dev/null 2>&1
           done
		done
	done
done

exit
