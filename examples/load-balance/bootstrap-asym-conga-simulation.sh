#!/bin/bash

cd ../..

for load in 0.8
do
        for run in 1
	do
                 nohup ./waf --run "conga-simulation --runMode=Conga-flow --randomSeed=$run --cdfFileName=examples/load-balance/VL2_CDF.txt --load=$load --asym=true" > /tmp/asym-conga-$load-$run.out 2>&1 &
		 nohup ./waf --run "conga-simulation --runMode=ECMP --randomSeed=$run --cdfFileName=examples/load-balance/VL2_CDF.txt --load=$load --asym=true" > /tmp/asym-ecmp-$load-$run.out 2>&1 &
	done
done

exit
