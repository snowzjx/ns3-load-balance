ns3-load-balance
===
![Build Status](https://travis-ci.com/snowzjx/ns3-load-balance.svg?token=h9rZZxytGHrsS5Xgsb6n&branch=master)

We have implemented the following transportation protocol and load balance scheme.

Transport Protocol
---
1. [DCTCP](http://simula.stanford.edu/~alizade/Site/DCTCP_files/dctcp-final.pdf)

Load Balance Scheme
---
1. Per flow ECMP
2. [CONGA](https://people.csail.mit.edu/alizadeh/papers/conga-sigcomm14.pdf)
3. [DRB](http://conferences.sigcomm.org/co-next/2013/program/p49.pdf)
4. [Presto](http://pages.cs.wisc.edu/~akella/papers/presto-sigcomm15.pdf)
5. [FlowBender](http://conferences2.sigcomm.org/co-next/2014/CoNEXT_papers/p149.pdf) 
6. TLB 
7. [CLOVE](https://www.cs.princeton.edu/~nkatta/papers/clove-hotnets16.pdf)
8. [DRILL](http://conferences.sigcomm.org/hotnets/2015/papers/ghorbani.pdf)
9. LetFlow

Routing 
---
1. [XPath](http://www.cse.ust.hk/~kaichen/papers/xpath-nsdi15.pdf)

Monitor
---
1. LinkMonitor


Program
--
You can use `./waf --run conga-simulation-large` to run most of the test cases.

```

Program Arguments:
    --ID:                         Running ID [0]
    --StartTime:                  Start time of the simulation [0]
    --EndTime:                    End time of the simulation [0.25]
    --FlowLaunchEndTime:          End time of the flow launch period [0.1]
    --runMode:                    Running mode of this simulation: Conga, Conga-flow, Presto, DRB, FlowBender, ECMP, Clove, DRILL, LetFlow [Conga]
    --randomSeed:                 Random seed, 0 for random generated [0]
    --cdfFileName:                File name for flow distribution []
    --load:                       Load of the network, 0.0 - 1.0 [0]
    --transportProt:              Transport protocol to use: Tcp, DcTcp [Tcp]
    --linkLatency:                Link latency, should be in MicroSeconds [10]
    --resequenceBuffer:           Whether enabling the resequence buffer [false]
    --resequenceInOrderTimer:     In order queue timeout in resequence buffer [5]
    --resequenceOutOrderTimer:    Out order queue timeout in resequence buffer [500]
    --resequenceInOrderSize:      In order queue size in resequence buffer [100]
    --resequenceBufferLog:        Whether enabling the resequence buffer logging system [false]
    --asymCapacity:               Whether the capacity is asym, which means some link will have only 1/10 the capacity of others [false]
    --asymCapacityPoss:           The possibility that a path will have only 1/10 capacity [40]
    --flowBenderT:                The T in flowBender [0.05]
    --flowBenderN:                The N in flowBender [1]
    --serverCount:                The Server count [8]
    --spineCount:                 The Spine count [4]
    --leafCount:                  The Leaf count [4]
    --linkCount:                  The Link count [1]
    --spineLeafCapacity:          Spine <-> Leaf capacity in Gbps [10]
    --leafServerCapacity:         Leaf <-> Server capacity in Gbps [10]
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
    --quantifyRTTBase:            The quantify RTT base in TLB [10]
    --TcpPause:                   Whether TCP will pause in TLB & FlowBender [false]
    --applicationPauseThresh:     How many packets can pass before we have delay, 0 for disable [0]
    --applicationPauseTime:       The time for a delay, in MicroSeconds [1000]
    --cloveFlowletTimeout:        Flowlet timeout for Clove [500]
    --cloveRunMode:               Clove run mode, 1 for edge flowlet, 2 for ECN, 3 for INT (not yet implemented) [0]
    --cloveHalfRTT:               Half RTT used in Clove ECN [40]
    --cloveDisToUncongestedPath:  Whether Clove will distribute the weight to uncongested path (no ECN) or all paths [false]
    --enableLargeDupAck:          Whether to set the ReTxThreshold to a very large value to mask reordering [false]
    --congaFlowletTimeout:        Flowlet timeout in Conga [50]
    --letFlowFlowletTimeout:      Flowlet timeout in LetFlow [50]
```
