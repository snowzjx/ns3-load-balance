ns3-load-balance
===
![Build Status](https://travis-ci.com/snowzjx/ns3-load-balance.svg?token=h9rZZxytGHrsS5Xgsb6n&branch=master)

We have implemented the following transportation protocol and load balance scheme.

Transport Protocol
---
1. [DCTCP](http://simula.stanford.edu/~alizade/Site/DCTCP_files/dctcp-final.pdf)

Load Balance Scheme
---
1. [Hermes](http://delivery.acm.org/10.1145/3100000/3098841/p253-Zhang.pdf?ip=143.89.162.8&id=3098841&acc=OPEN&key=CDD1E79C27AC4E65%2EFC30B8D6EF32B758%2E4D4702B0C3E38B35%2E6D218144511F3437&CFID=806052151&CFTOKEN=32034367&__acm__=1504685825_e269484188deac95118498dd9e4ea239)
2. Per flow ECMP
3. [CONGA](https://people.csail.mit.edu/alizadeh/papers/conga-sigcomm14.pdf)
4. [DRB](http://conferences.sigcomm.org/co-next/2013/program/p49.pdf)
5. [Presto](http://pages.cs.wisc.edu/~akella/papers/presto-sigcomm15.pdf)
6. Weighted Presto, which has to be used together with asymmetric topology
7. [FlowBender](http://conferences2.sigcomm.org/co-next/2014/CoNEXT_papers/p149.pdf) 
8. [CLOVE](https://www.cs.princeton.edu/~nkatta/papers/clove-hotnets16.pdf)
9. [DRILL](http://conferences.sigcomm.org/hotnets/2015/papers/ghorbani.pdf)
10.[LetFlow](https://people.csail.mit.edu/alizadeh/papers/letflow-nsdi17.pdf)

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

```



### Set the traffic pattern
```
--cdfFileName:                    File name for flow distribution [no default value, YOU MUST SPECIFY THIS VALUE]
--load:                           Load of the network, 0.0 - 1.0 [default value: 0]
```

```
Program Arguments:
    --resequenceBuffer:           Whether enabling the resequence buffer [false]
    --resequenceInOrderTimer:     In order queue timeout in resequence buffer [5]
    --resequenceOutOrderTimer:    Out order queue timeout in resequence buffer [500]
    --resequenceInOrderSize:      In order queue size in resequence buffer [100]
    --resequenceBufferLog:        Whether enabling the resequence buffer logging system [false]
    --asymCapacity:               Whether the capacity is asym, which means some link will have only 1/10 the capacity of others [false]
    --asymCapacityPoss:           The possibility that a path will have only 1/10 capacity [40]
    --flowBenderT:                The T in flowBender [0.05]
    --flowBenderN:                The N in flowBender [1]
        --TLBMinRTT:                  Min RTT used to judge a good path in TLB [40]
    --TLBHighRTT:                 High RTT used to judge a bad path in TLB [180]
    --TLBPoss:                    Possibility to change the path in TLB [50]
    --TLBBetterPathRTT:           RTT Threshold used to judge one path is better than another in TLB [1]
    --TLBT1:                      The path aging time interval in TLB [100]
    --TLBECNPortionLow:           The ECN portion used in judging a good path in TLB [0.1]
    --TLBRunMode:                 The running mode of TLB, 0 for minimize counter, 1 for minimize RTT, 2 for random, 11 for RTT counter, 12 for RTT DRE [0]
    --TLBProbingEnable:           Whether the TLB probing is enable [true]
    --TLBProbingInterval:         Probing interval for TLB probing [50]
    --TLBSmooth:                  Whether the RTT calculation is smooth [true]
    --TLBRerouting:               Whether the rerouting is enabled in TLB [true]
    --TLBDREMultiply:             DRE multiply factor in TLB [5]
    --TLBS:                       The S used to judge a whether a flow should change path in TLB [64000]
    --TLBReverseACK:              Whether to enable the TLB reverse ACK path selection [false]
    --quantifyRTTBase:            The quantify RTT base in TLB [10]
    --TLBFlowletTimeout:          The TLB flowlet timeout [500]
    --TcpPause:                   Whether TCP will pause in TLB & FlowBender [false]
    --applicationPauseThresh:     How many packets can pass before we have delay, 0 for disable [0]
    --applicationPauseTime:       The time for a delay, in MicroSeconds [1000]
    --cloveFlowletTimeout:        Flowlet timeout for Clove [500]
    --cloveRunMode:               Clove run mode, 0 for edge flowlet, 1 for ECN, 2 for INT (not yet implemented) [0]
    --cloveHalfRTT:               Half RTT used in Clove ECN [40]
    --cloveDisToUncongestedPath:  Whether Clove will distribute the weight to uncongested path (no ECN) or all paths [false]
    --congaFlowletTimeout:        Flowlet timeout in Conga [50]
    --letFlowFlowletTimeout:      Flowlet timeout in LetFlow [50]
    --enableRandomDrop:           Whether the Spine-0 to other leaves has the random drop problem [false]
    --randomDropRate:             The random drop rate when the random drop is enabled [0.005]```
    --blackHoleMode:              The packet black hole mode, 0 to disable, 1 src, 2 dest, 3 src/dest pair [0]
    --blackHoleSrcAddr:           The packet black hole source address [10.1.1.1]
    --blackHoleSrcMask:           The packet black hole source mask [255.255.255.240]
    --blackHoleDestAddr:          The packet black hole destination address [10.1.2.0]
    --blackHoleDestMask:          The packet black hole destination mask [255.255.255.0]
    --congaAwareAsym:             Whether Conga is aware of the capacity of asymmetric path capacity [true]
```
