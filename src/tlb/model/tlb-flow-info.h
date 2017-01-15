#ifndef TLB_FLOW_INFO_H
#define TLB_FLOW_INFO_H

#include "ns3/nstime.h"

namespace ns3 {

class TLBFlowInfo
{
public:
  uint32_t flowId;
  uint32_t path;
  uint32_t destTor;
  uint32_t size;
  uint32_t ecnSize;
  uint32_t sendSize;
  uint32_t retransmissionSize;
  uint32_t timeoutCount;
  Time timeStamp;
  Time tryChangePath;
  Time liveTime;

  // Added at Dec 23rd
  Time activeTime;
  // --

  // Added at Jan 12nd
//  Time tlbFlowletActiveTime;
  // --

  // Added at Jan 11st
  /*
  uint32_t epAckSize;
  uint32_t epEcnSize;
  double epEcnPortion;
  Time epTimeStamp;
  */
  // --

};

}

#endif


