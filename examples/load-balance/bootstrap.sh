#!/bin/bash

cd ../..

for load in 0.3 0.5 0.8
do
        for run in 1 2 3 
	do
                 nohup ./waf --run "conga-simulation --runMode=Conga --randomSeed=$run --cdfFileName=examples/load-balance/VL2_CDF.txt --load=$load" > /tmp/conga-$load-$run.out 2>&1 &
		 nohup ./waf --run "conga-simulation --runMode=ECMP --randomSeed=$run --cdfFileName=examples/load-balance/VL2_CDF.txt --load=$load" > /tmp/ecmp-$load-$run.out 2>&1 &
	done
done

exit
