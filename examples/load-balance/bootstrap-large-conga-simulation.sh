#!/bin/bash

cd ../..

for run in 1
do

        for load in 0.3 0.5 0.8
	do
		for trans in DcTcp Tcp
		do
			for runMode in Conga Conga-flow ECMP Presto
			do
				nohup ./waf --run "conga-simulation-large --runMode=$runMode --transportProt=$trans --randomSeed=$run --cdfFileName=examples/load-balance/VL2_CDF.txt --load=$load" > /tmp/large-conga-$runMode-$trans-$load-$run.out 2>&1 &
			done
		done
	done
done

exit
