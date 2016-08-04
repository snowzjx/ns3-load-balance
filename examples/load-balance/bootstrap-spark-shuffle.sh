#!/bin/bash

cd ../..


for run in 1 
do
	nohup ./waf --run "spark-shuffle --runMode=Conga --randomSeed=$run" > /tmp/spark-shuffle-conga-$run.out 2>&1 &
	nohup ./waf --run "spark-shuffle --runMode=Conga-flow --randomSeed=$run" > /tmp/spark-shuffle-conga-flow-$run.out 2>&1 &
	nohup ./waf --run "spark-shuffle --runMode=ECMP --randomSeed=$run" > /tmp/spark-shffle-ecmp-$run.out 2>&1 &
done

exit
