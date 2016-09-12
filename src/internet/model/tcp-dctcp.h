#ifndef TCP_DCTCP_H
#define TCP_DCTCP_H

#include "tcp-congestion-ops.h"
#include "ns3/traced-value.h"
#include "ns3/nstime.h"
#include "ns3/sequence-number.h"

namespace ns3 {

class TcpDCTCP : public TcpNewReno
{
public:

    static TypeId GetTypeId (void);

    TcpDCTCP ();
    TcpDCTCP (const TcpDCTCP &sock);

    ~TcpDCTCP ();

    std::string GetName () const;

    virtual void PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time &rtt, bool withECE,
            SequenceNumber32 headSeq, SequenceNumber32 ackNumber);
    void UpdateAlpha();
    virtual void CwndEvent(Ptr<TcpSocketState> tcb, TcpCongEvent_t ev, Ptr<TcpSocketBase> socket);
    virtual void IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
    virtual uint32_t GetSsThresh(Ptr<TcpSocketState> tcb, uint32_t bytesInFlight);
    virtual uint32_t GetCwnd(Ptr<TcpSocketState> tcb);

    virtual Ptr<TcpCongestionOps> Fork ();

protected:
    double                            m_g;
    TracedValue<double>               m_alpha;
    bool                              m_isCE;
    bool                              m_hasDelayedACK;

    uint32_t                          m_bytesAcked;
    uint32_t                          m_ecnBytesAcked;

    SequenceNumber32                  m_highTxMark;
};

}

#endif
