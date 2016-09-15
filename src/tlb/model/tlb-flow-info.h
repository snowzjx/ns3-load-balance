#ifndef TLB_FLOW_INFO_H
#define TLB_FLOW_INFO_H

namespace ns3 {

class TLBFlowInfo
{
public:
  uint32_t flowId;
  uint32_t path;
  uint32_t size;
  uint32_t ecnSize;
  uint32_t sendSize;
  uint32_t retransmissionSize;
  bool isTimeout;
};

}

#endif


