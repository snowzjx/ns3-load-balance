#ifndef TLB_PATH_INFO_H
#define TLB_PATH_INFO_H

#include "ns3/nstime.h"

namespace ns3 {

class TLBPathInfo
{
public:
  uint32_t pathId;
  uint32_t size;
  uint32_t ecnSize;
  Time minRtt;
  bool isRetransmission;
  bool isHighRetransmission;
  bool isTimeout;
  bool isVeryTimeout;
  bool isProbingTimeout;
  uint32_t flowCounter;
  Time timeStamp1;
  Time timeStamp2;
};

}

#endif
