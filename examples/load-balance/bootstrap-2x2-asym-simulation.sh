#!/bin/bash

cd ../..

for run in 1
do

    for load in 0.5 0.8
	do
		for trans in DcTcp Tcp
		do
			for runMode in Conga Conga-flow ECMP 
			do
                nohup ./waf --run "conga-simulation-large --spineCount=2 --leafCount=2 --serverCount=32 --linkCount=2 --spineLeafCapacity=40 --leafServerCapacity=10 --asym=true --runMode=$runMode --transportProt=$trans --randomSeed=$run --cdfFileName=examples/load-balance/VL2_CDF.txt --load=$load" > /tmp/large-conga-$runMode-$trans-$load-$run.out 2>&1 &
			done
            for runMode in Presto DRB 
            do
				nohup ./waf --run "conga-simulation-large --spineCount=2 --leafCount=2 --serverCount=32 --linkCount=2 --spineLeafCapacity=40 --leafServerCapacity=10 --asym=true --runMode=$runMode --transportProt=$trans --resequenceBuffer=true --randomSeed=$run --cdfFileName=examples/load-balance/VL2_CDF.txt --load=$load" > /tmp/large-conga-$runMode-$trans-$load-$run.out 2>&1 &
			done
        done
	done
done

exit
