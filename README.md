ns3-load-balance
===
![Build Status](https://travis-ci.com/snowzjx/ns3-load-balance.svg?token=h9rZZxytGHrsS5Xgsb6n&branch=master)

We have implemented the following transportation protocol and load balance scheme.

Transport Protocol
---
1. [DCTCP](https://people.csail.mit.edu/alizadeh/papers/dctcp-sigcomm10.pdf)

Load Balance Scheme
---
1. [Hermes](http://www.cse.ust.hk/~kaichen/papers/hermes-sigcomm17.pdf)
2. Per flow ECMP
3. [CONGA](https://people.csail.mit.edu/alizadeh/papers/conga-sigcomm14.pdf)
4. [DRB](http://conferences.sigcomm.org/co-next/2013/program/p49.pdf)
5. [Presto](http://pages.cs.wisc.edu/~akella/papers/presto-sigcomm15.pdf)
6. Weighted Presto, which has to be used together with asymmetric topology
7. [FlowBender](http://conferences2.sigcomm.org/co-next/2014/CoNEXT_papers/p149.pdf) 
8. [CLOVE](https://www.cs.princeton.edu/~jrex/papers/clove16.pdf)
9. [DRILL](http://conferences.sigcomm.org/hotnets/2015/papers/ghorbani.pdf)
10. [LetFlow](https://people.csail.mit.edu/alizadeh/papers/letflow-nsdi17.pdf)

Routing 
---
1. [XPath](http://www.cse.ust.hk/~kaichen/papers/xpath-nsdi15.pdf)

Monitor
---
1. LinkMonitor

Hermes - Resilient Datacenter Load Balancing in the Wild
---


How to run test cases
---
You can use `./waf --run conga-simulation-large` to run most of the test cases to reproduce the results of our paper.

You can follow the instructions below to change the parameters of the test cases.

### Set the running ID
```
--ID:                             Running ID [default value: 0], the running ID will be added to the beginning of all ouput files.
```

### Set the random seed
```
--randomSeed:                     Random seed, 0 for random generated seed (based on current time) [default value: 0]
```

### Set simulation time
```
--StartTime:                      Start time of the simulation [default value: 0]
--EndTime:                        End time of the simulation [defaule value: 0.25s]
--FlowLaunchEndTime:              End time of the flow launch period [default value: 0.1s]
```
From *StartTime* to *FlowLaunchEndTime*, flows will be generated according to CDF (see below). From *FlowLaunchEndTime* to *EndTime*, no flows will generated. 

### Set the load balancing scheme & transport protocol
```
--runMode:                        Load balancing scheme: TLB (Hermes), Conga, Conga-flow, Presto, DRB, FlowBender, ECMP, Clove, DRILL, LetFlow [default value: Conga]
--transportProt:                  Transport protocol to use: Tcp, DcTcp [default value: Tcp]
--enableLargeDupAck:              Whether to set the ReTxThreshold to a very large value to mask reordering [default value: false]
--enableLargeSynRetries:          Whether the SYN packet would retry thousands of times [default value: false]
--enableFastReConnection:         Whether the SYN gap will be very small when reconnecting [default value: false]
--enableLargeDataRetries:         Whether the data retransmission will be more than 6 times [default value: false]
```

### Set the topology
```
--serverCount:                    The server count [default value: 8]
--spineCount:                     The spine count [default value: 4]
--leafCount:                      The leaf count [default value: 4]
--linkCount:                      The link count between one spine and one leaf switch [default value: 1]
--spineLeafCapacity:              Spine <-> leaf capacity (in Gbps) [default value: 10Gbps]
--leafServerCapacity:             Leaf <-> server capacity (in Gbps) [default value: 10Gbps]
--linkLatency:                    Link latency of one hop (in microsecond) [default value: 10us]
```

The topology used in all test cases is a spine-leaf topology, see Figure 7 in Conga paper for more details.

To simulate an asymmetric topology, you can change the following parameters:

```
--asymCapacity:                   Whether the capacity is asym, which means some link will have only 1/5 the capacity of others [default value: false]
--asymCapacityPoss:               The possibility that a path will have only 1/5 capacity (in %) [default value: 40%]
--asymCapacity2:                  Whether the Spine0-Leaf0's capacity is only 1/5 [default value: false] 
```

### Set the traffic pattern
```
--cdfFileName:                    File name for flow distribution [no default value, YOU MUST SPECIFY THIS VALUE]
--load:                           Load of the network, 0.0 - 1.0 [default value: 0]
```

### Set the resequence buffer

If you are using DRB, Presto or other packet spay load balancing scheme, you can turn on the resequence buffer to avoid the performance degradation caused by TCP disorder.

```
--resequenceBuffer:              Whether to enable the resequence buffer [default value: false]
--resequenceInOrderTimer:        In order queue timeout (in microseconds) [default value: 5us]
--resequenceOutOrderTimer:       Out order queue timeout (in microseconds) [default value: 500us]
--resequenceInOrderSize:         In order queue size (in packets) [default value: 100 packets]
--resequenceBufferLog:           Whether to enable the resequence buffer logging system [default value: false]
```

### Set the grey failure 

You can change the following parameters to simulate grey failures in DCN, such as pakcet random drop or packet black hole.

Random packet drop:
```
--enableRandomDrop:              Whether the Spine-0 to other leaves has the random drop problem [default value: false]
--randomDropRate:                The random drop rate when the random drop is enabled [default value: 0.005]
```

Packet black hole:
```
--blackHoleMode:                 The packet black hole mode, 0 for disable, 1 for matching src address only, 2 for matching dest address only, 3 for both src/dest address [default value: 0]
--blackHoleSrcAddr:              The packet black hole source address [default value: 10.1.1.1]
--blackHoleSrcMask:              The packet black hole source mask [default value: 255.255.255.240]
--blackHoleDestAddr:             The packet black hole destination address [default value: 10.1.2.0]
--blackHoleDestMask:             The packet black hole destination mask [default value: 255.255.255.0]
```

### Set Hermes parameters:

Notification: TLB is the former name of Hermes.

```
--TLBMinRTT:                     The RTT threshold used to judge a good path in TLB (T_{RTT_LOW}) in microseconds [Recommended value: 60 (20 + one-way base RTT)]
--TLBHighRTT:                    The RTT threshold used to judge a good path in TLB (T_{RTT_HIGH}) in microseconds [Recommended value: 180]
--TLBBetterPathRTT:              The RTT threshold used to judge whether a path is better than another one (Delta_{RTT}) in microseconds [Recommended value: 100]
--TLBT1:                         The path aging time interval (i.e., the frequency to update path condition) in microseconds [Recommended value: 60]
--TLBECNPortionLow:              The ECN portion used in judging a good path in TLB (T_{ECN}). (We mainly use RTT in our simulation and not all parameters related to ECN is exposed here. Please refer to /src/tlb/model/ipv4-tlb.cc for more details)
--TLBRunMode:                    The running mode of TLB (i.e., how to choose path from candidate paths), 0 for minimize counter, 1 for minimize RTT, 2 for random, 11 for RTT counter, 12 for RTT DRE [Recommended value: 12]
--TLBProbingEnable:              Whether the TLB probing is enabled [Recommended value: true]
--TLBProbingInterval:            Probing interval for TLB probing (in microseconds) [Recommended value: 100-500]
--TLBSmooth:                     Whether the RTT calculation is smoothed (i.e., using EWMA) [Recommended value: true]
--TLBRerouting:                  Whether the rerouting is enabled in TLB [Recommended value: true]
--TLBS:                          The sent size used to judge a whether a flow should change path in TLB (in bytes) [Recommended value: 600000]
--TLBReverseACK:                 Whether to enable the TLB reverse ACK path selection [Default value: false]
--quantifyRTTBase:               The quantify RTT base (granularity to quantify paths to different categories according to RTT) [Default value: 10] 
--TLBFlowletTimeout:             The flowlet timeout value [Default value: 5ms]
```

### Set Conga parameters:
If you are using Conga loading balancing scheme, you should set the following parameters:

```
--congaFlowletTimeout:           Flowlet timeout in Conga (in microseconds) [default value: 500us]
--congaAwareAsym:                Whether Conga is aware of the capacity of asymmetric path capacity [default value: true]
```
### Set LetFlow parameters:
If you are using LetFlow loading balancing scheme, you should set the following parameters:
```
--letFlowFlowletTimeout:         Flowlet timeout in LetFlow (in microseconds) [default value: 500us]
```

### Set Clove parameters:
If you are using Clove loading balancing scheme, you should set the following parameters:
```
--cloveFlowletTimeout:           Flowlet timeout for Clove (in microseconds) [default value: 500us]
--cloveRunMode:                  Clove run mode, 0 for edge flowlet, 1 for ECN, 2 for INT (not yet implemented) [default value: 0]
--cloveHalfRTT:                  Half RTT used in Clove ECN (in microseconds) [default value: 40us]
--cloveDisToUncongestedPath:     Whether Clove will distribute the weight to uncongested path (no ECN) or all paths [default value: false]
```

### Set FlowBender parameters:
If you are using FlowBender loading balancing scheme, you should set the following parameters:
```
--flowBenderT:                   The T in FlowBender [default value: 0.05]
--flowBenderN:                   The N in FlowBender [default value: 1]
```

### Some experimental parameters:
Please *DO NOT* use those parameters if you haven't read all the code
```
--TcpPause:                      Whether TCP will pause in Hermes & FlowBender [default value: false]
--applicationPauseThresh:        How many packets can pass before we have delay, 0 for disable [default value: 0]
--applicationPauseTime:          The time for a delay (in microseconds) [default value: 1000us]
```
