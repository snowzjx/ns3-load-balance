#!/bin/bash

cd ../..


for run in 1 
do
	for prot in Tcp DcTcp
	do
		nohup ./waf --run "spark-shuffle --runMode=Conga --randomSeed=$run --transportProt=$prot" > /tmp/spark-shuffle-conga-$run-$prot.out 2>&1 &
		nohup ./waf --run "spark-shuffle --runMode=Conga-flow --randomSeed=$run --transportProt=$prot" > /tmp/spark-shuffle-conga-flow-$run-$prot.out 2>&1 &
		nohup ./waf --run "spark-shuffle --runMode=ECMP --randomSeed=$run --transportProt=$prot" > /tmp/spark-shuffle-ecmp-$run-$prot.out 2>&1 &
	done
done
exit
