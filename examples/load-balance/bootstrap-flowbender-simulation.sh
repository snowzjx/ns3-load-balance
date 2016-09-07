#!/bin/bash

cd ../..

for run in 1
do

    for load in 0.5 0.8
	do
        for T in 0.05 0.10 0.15 0.20 0.25 0.5
        do
            nohup ./waf --run "conga-simulation-large --runMode=FlowBender --flowBenderT=$T --transportProt=DcTcp --randomSeed=$run --cdfFileName=examples/load-balance/VL2_CDF.txt --load=$load" > /tmp/large-conga-flowBender-DcTcp-$T-$load-$run.out 2>&1 &
		done
	done
done

exit
