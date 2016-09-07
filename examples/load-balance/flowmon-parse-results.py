from __future__ import division
import sys
import os
try:
    from xml.etree import cElementTree as ElementTree
except ImportError:
    from xml.etree import ElementTree

def parse_time_ns(tm):
    if tm.endswith('ns'):
        return long(tm[:-4])
    raise ValueError(tm)

class FiveTuple(object):
    __slots__ = ['sourceAddress', 'destinationAddress', 'protocol', 'sourcePort', 'destinationPort']
    def __init__(self, el):
        self.sourceAddress = el.get('sourceAddress')
        self.destinationAddress = el.get('destinationAddress')
        self.sourcePort = int(el.get('sourcePort'))
        self.destinationPort = int(el.get('destinationPort'))
        self.protocol = int(el.get('protocol'))

class Histogram(object):
    __slots__ = 'bins', 'nbins', 'number_of_flows'
    def __init__(self, el=None):
        self.bins = []
        if el is not None:
            #self.nbins = int(el.get('nBins'))
            for bin in el.findall('bin'):
                self.bins.append( (float(bin.get("start")), float(bin.get("width")), int(bin.get("count"))) )

class Flow(object):
    __slots__ = ['flowId', 'delayMean', 'packetLossRatio', 'rxBitrate', 'txBitrate',
                 'fiveTuple', 'packetSizeMean', 'probe_stats_unsorted',
                 'hopCount', 'flowInterruptionsHistogram', 'rx_duration',
                 'fct', 'txBytes', 'txPackets', 'rxPackets', 'rxBytes', 'lostPackets']
    def __init__(self, flow_el):
        self.flowId = int(flow_el.get('flowId'))
        rxPackets = long(flow_el.get('rxPackets'))
        txPackets = long(flow_el.get('txPackets'))
        tx_duration = float(long(flow_el.get('timeLastTxPacket')[:-4]) - long(flow_el.get('timeFirstTxPacket')[:-4]))*1e-9
        rx_duration = float(long(flow_el.get('timeLastRxPacket')[:-4]) - long(flow_el.get('timeFirstRxPacket')[:-4]))*1e-9
        fct = float(long(flow_el.get('timeLastRxPacket')[:-4]) - long(flow_el.get('timeFirstTxPacket')[:-4]))*1e-9
        txBytes = long(flow_el.get('txBytes'))
        rxBytes = long(flow_el.get('rxBytes'))
        self.txBytes = txBytes
        self.txPackets = txPackets
        self.rxBytes = rxBytes
        self.rxPackets = rxPackets
        self.rx_duration = rx_duration
        if fct > 0:
            self.fct = fct
        else:
            self.fct = None
        self.probe_stats_unsorted = []
        if rxPackets:
            self.hopCount = float(flow_el.get('timesForwarded')) / rxPackets + 1
        else:
            self.hopCount = -1000
        if rxPackets:
            self.delayMean = float(flow_el.get('delaySum')[:-4]) / rxPackets * 1e-9
            self.packetSizeMean = float(flow_el.get('rxBytes')) / rxPackets
        else:
            self.delayMean = None
            self.packetSizeMean = None
        if rx_duration > 0:
            self.rxBitrate = long(flow_el.get('rxBytes'))*8 / rx_duration
        else:
            self.rxBitrate = None
        if tx_duration > 0:
            self.txBitrate = long(flow_el.get('txBytes'))*8 / tx_duration
        else:
            self.txBitrate = None
        lost = float(flow_el.get('lostPackets'))
        self.lostPackets = lost
        #print "rxBytes: %s; txPackets: %s; rxPackets: %s; lostPackets: %s" % (flow_el.get('rxBytes'), txPackets, rxPackets, lost)
        if rxPackets == 0:
            self.packetLossRatio = None
        else:
            self.packetLossRatio = (lost / (rxPackets + lost))

        interrupt_hist_elem = flow_el.find("flowInterruptionsHistogram")
        if interrupt_hist_elem is None:
            self.flowInterruptionsHistogram = None
        else:
            self.flowInterruptionsHistogram = Histogram(interrupt_hist_elem)


class ProbeFlowStats(object):
    __slots__ = ['probeId', 'packets', 'bytes', 'delayFromFirstProbe']

class Simulation(object):
    def __init__(self, simulation_el):
        self.flows = []
        FlowClassifier_el, = simulation_el.findall("Ipv4FlowClassifier")
        flow_map = {}
        for flow_el in simulation_el.findall("FlowStats/Flow"):
            flow = Flow(flow_el)
            flow_map[flow.flowId] = flow
            self.flows.append(flow)
        for flow_cls in FlowClassifier_el.findall("Flow"):
            flowId = int(flow_cls.get('flowId'))
            flow_map[flowId].fiveTuple = FiveTuple(flow_cls)

        for probe_elem in simulation_el.findall("FlowProbes/FlowProbe"):
            probeId = int(probe_elem.get('index'))
            for stats in probe_elem.findall("FlowStats"):
                flowId = int(stats.get('flowId'))
                s = ProbeFlowStats()
                s.packets = int(stats.get('packets'))
                s.bytes = long(stats.get('bytes'))
                s.probeId = probeId
                if s.packets > 0:
                    s.delayFromFirstProbe =  parse_time_ns(stats.get('delayFromFirstProbeSum')) / float(s.packets)
                else:
                    s.delayFromFirstProbe = 0
                flow_map[flowId].probe_stats_unsorted.append(s)

def main(argv):
    file_obj = open(argv[1])
    print "Reading XML file ",

    sys.stdout.flush()
    level = 0
    sim_list = []
    for event, elem in ElementTree.iterparse(file_obj, events=("start", "end")):
        if event == "start":
            level += 1
        if event == "end":
            level -= 1
            if level == 0 and elem.tag == 'FlowMonitor':
                sim = Simulation(elem)
                sim_list.append(sim)
                elem.clear() # won't need this any more
                sys.stdout.write(".")
                sys.stdout.flush()
    print " done."

    total_fct = 0
    flow_count = 0
    large_flow_total_fct = 0
    large_flow_count = 0
    small_flow_total_fct = 0
    small_flow_count = 0

    total_lost_packets = 0
    total_packets = 0
    total_rx_packets = 0

    max_small_flow_id = 0
    max_small_flow_fct = 0

    for sim in sim_list:
        for flow in sim.flows:
            if flow.fct == None or flow.txBitrate == None or flow.rxBitrate == None:
                continue
            if flow.txBytes == 52 * flow.txPackets + 4:
                continue
            flow_count += 1
            total_fct += flow.fct
	    total_packets += flow.txPackets 
            total_rx_packets += flow.rxPackets
	    total_lost_packets += flow.lostPackets
            if flow.txBytes > 10000000:
                large_flow_count += 1
                large_flow_total_fct += flow.fct
            if flow.txBytes < 100000:
                small_flow_count += 1
                small_flow_total_fct += flow.fct
                if flow.fct > max_small_flow_fct:
                    max_small_flow_id = flow.flowId
                    max_small_flow_fct = flow.fct
            t = flow.fiveTuple
            proto = {6: 'TCP', 17: 'UDP'} [t.protocol]
            print "FlowID: %i (%s %s/%s --> %s/%i)" % \
                (flow.flowId, proto, t.sourceAddress, t.sourcePort, t.destinationAddress, t.destinationPort)
            print "\tTX bitrate: %.2f kbit/s" % (flow.txBitrate*1e-3,)
            print "\tRX bitrate: %.2f kbit/s" % (flow.rxBitrate*1e-3,)
            print "\tMean Delay: %.2f ms" % (flow.delayMean*1e3,)
            #print "\tPacket Loss Ratio: %.2f %%" % (flow.packetLossRatio*100)
            print "\tFlow size: %i bytes, %i packets" % (flow.txBytes, flow.txPackets)
            print "\tRx %i bytes, %i packets" % (flow.rxBytes, flow.rxPackets)
            print "\tDevice Lost %i packets" % (flow.lostPackets)
            print "\tReal Lost %i packets" % (flow.txPackets - flow.rxPackets)
            print "\tFCT: %.4f" % (flow.fct)

    print "Avg FCT: %.4f" % (total_fct / flow_count)
    if large_flow_count == 0:
	print "No large flows"
    else:
        print "Large Flow Avg FCT: %.4f" % (large_flow_total_fct / large_flow_count)

    if small_flow_count == 0:
   	print "No small flows"
    else:
	print "Small Flow Avg FCT: %.4f" % (small_flow_total_fct / small_flow_count)

    print "Total TX Packets: %i" % total_packets
    print "Total RX Packets: %i" % total_rx_packets
    print "Total Lost Packets: %i" % total_lost_packets
    print "Max Small flow Id: %i" % max_small_flow_id

if __name__ == '__main__':
    main(sys.argv)
