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
                nohup ./waf --run "conga-simulation-large --spineCount=4 --leafCount=4 --serverCount=8 --linkCount=1 --spineLeafCapacity=10 --leafServerCapacity=10 --asym2=true --runMode=$runMode --transportProt=$trans --randomSeed=$run --cdfFileName=examples/load-balance/VL2_CDF.txt --load=$load" > /tmp/large-conga-$runMode-$trans-$load-$run.out 2>&1 &
			done
            for runMode in Presto DRB 
            do
				nohup ./waf --run "conga-simulation-large --spineCount=4 --leafCount=4 --serverCount=8 --linkCount=1 --spineLeafCapacity=10 --leafServerCapacity=10 --asym2=true --runMode=$runMode --transportProt=$trans --resequenceBuffer=true --randomSeed=$run --cdfFileName=examples/load-balance/VL2_CDF.txt --load=$load" > /tmp/large-conga-$runMode-$trans-$load-$run.out 2>&1 &
			done
        done
	done
done

exit
