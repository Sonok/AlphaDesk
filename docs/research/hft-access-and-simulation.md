# What colocated HFT firms use, what an individual can access, and how to simulate the rest

**Date:** 2026-10-18
**Purpose:** Decide which real exchange interfaces AlphaDesk should implement, which it can
only simulate, and what that simulation should look like to be faithful.

---

## 1. What colocated firms actually run

| Layer | US equities (Nasdaq, NYSE, Cboe) | CME futures | Options |
|---|---|---|---|
| Market data, direct | Nasdaq TotalView-ITCH over MoldUDP64 (UDP multicast), with SoupBinTCP or a retransmission server for recovery. NYSE Pillar Integrated Feed (XDP). Cboe Multicast PITCH. | MDP 3.0: Simple Binary Encoding (SBE) messages over UDP multicast, A and B feeds, TCP replay channel, separate snapshot channel for recovery. | OPRA (SIAC), tens of multicast channels, terabytes per day. |
| Market data, consolidated | SIP feeds (CTA, UTP). Slower; what retail APIs mostly derive from. | n/a | n/a |
| Order entry | Nasdaq OUCH over SoupBinTCP. NYSE Pillar binary gateway. Cboe BOE (Binary Order Entry). FIX as the slow fallback. | iLink 3: SBE over TCP, with mandatory AutoCert+ certification. | Exchange-specific binary gateways; FIX. |
| Transport and NIC | Solarflare (now AMD) X2/X3 or 8000-series with Onload or ef_vi; NVIDIA ConnectX with VMA or DPDK; FPGA NICs for the fastest tier. | Same. | Same. |
| Network | Cross-connects in Nasdaq Carteret, NYSE Mahwah, Cboe at Equinix NY5, CME at CyrusOne Aurora. Layer-1 switches. Microwave or millimetre-wave between Aurora and New Jersey. | | |
| Time | PTP or White Rabbit; hardware timestamps on the NIC; nanosecond clocks on every message. | | |
| Testing | Exchange test environments: Nasdaq Test Facility (NTF), CME Certification and New Release environments, Cboe certification. Plus internal exchange simulators and pcap replay rigs. | | |

Two points matter for this project. First, every one of these protocols has a public
specification: ITCH, MoldUDP64, SoupBinTCP, OUCH, MDP 3.0 and its SBE schema, iLink 3,
PITCH, BOE, Pillar, IEX-TP. Anyone can implement them. Second, the pattern is the same
everywhere: a binary message protocol, carried over UDP multicast for data and TCP for
orders, with sequence numbers and a recovery channel. Learn it once on ITCH and the
rest is details.

## 2. What an individual is allowed to access

| Thing | Access for an individual | Notes |
|---|---|---|
| Protocol specifications | Yes, free | All public PDFs on nasdaqtrader.com, cmegroup.com client wiki, cboe.com, iexexchange.io. |
| Nasdaq TotalView-ITCH, recorded full days | Yes, free | 15 full days, 2018 to 2020, plus BX and PSX feeds, at emi.nasdaq.com/ITCH. Order-level, every symbol. The single most valuable resource here. |
| IEX DEEP and TOPS, recorded daily pcaps | Yes, free | T+1, last 12 months. Real pcaps of the multicast feed in IEX-TP framing, so they can be replayed packet for packet. |
| LOBSTER (reconstructed Nasdaq L3 by level) | Sample files free; full data paid or academic | Good for checking your own book reconstruction. |
| Nasdaq TotalView-ITCH, live | No | Databento carries it from NY4 but only for business entities with a Nasdaq license. Not available to individuals. |
| CME MDP 3.0, recorded | Yes, paid | Databento GLBX.MDP3 as normalised MBO from 0.50 USD/GB with 125 USD signup credit. Raw sample pcaps by request to CME Data Sales after creating an account. |
| CME MDP 3.0, live | Paid, plan required | Databento Standard plan around 199 USD/month includes CME licence fees. Not needed for this project. |
| Cboe PITCH, recorded | Paid | DataShop, around 1000 USD/month/exchange. Out of budget; the spec is free. |
| OPRA, recorded | Paid, usage-based | Databento; a few underlyings for a few days fits in the credit. Research only. |
| Consolidated or broker-derived live data | Yes, free to cheap | Alpaca free tier (IEX source, websocket, 30 symbols). SIP upgrade 99 USD/month. Not low latency: hundreds of microseconds to milliseconds. |
| Order entry: OUCH, iLink 3, BOE | No | Requires exchange membership or sponsored access through a member, and certification. |
| Exchange test environments (NTF, CME CERT) | No in practice | NTF needs a firm MPID and ports around 300 USD/month after a free 30 days; CME CERT needs a Globex customer relationship. |
| Broker APIs for execution | Yes | Alpaca (REST, websocket, free, commission-free equities). IBKR (TWS and Client Portal APIs; equities, options, futures; paper free). |
| Lower-latency retail futures APIs | Yes, paid | Rithmic R|API+ in C++ through an FCM such as AMP, Ironbeam or Optimus: roughly 25 USD/month connection plus 0 to 100 USD/month API fee, latency quoted around 250 us. Trading Technologies similar. Closest an individual gets to a real futures gateway. |
| Colocation-adjacent hosting | Yes, paid | Retail VPS providers in Aurora and Secaucus for 30 to 100 USD/month. Not a cross-connect, but tens of microseconds to the broker gateway rather than milliseconds. |
| Kernel-bypass hardware | Yes, cheap used | Solarflare SFN8522 (10GbE, used, roughly 50 to 150 USD) runs the open-source OpenOnload and ef_vi on current kernels. Older SFN7122F is cheaper but not in the current Onload support list. Mellanox ConnectX-4 Lx runs open-source libvma. Intel X520 or any XDP-capable NIC runs AF_XDP with no vendor stack. |
| Kernel-bypass software | Yes, free | OpenOnload (GitHub Xilinx-CNS/onload), libvma, DPDK, AF_XDP in mainline Linux, io_uring. |

The short version: you can get the exact data formats, the exact specs, and the exact
software stack that firms use. You cannot get live direct feeds or direct order entry.
Those two are what you simulate.

## 3. How to simulate the parts you cannot access

### 3.1 Market data: your own exchange simulator

Replay recorded data over real UDP multicast with the real transport framing, so the
feed handler is indistinguishable from one running in colo.

- **Nasdaq:** read an ITCH day file, wrap messages in MoldUDP64 packets (session,
  sequence number, message count, length-prefixed blocks), send on a multicast group
  at recorded or scaled timing. Implement the MoldUDP64 retransmission request on a
  second UDP socket, and optionally SoupBinTCP for the glimpse or snapshot path.
- **IEX:** replay the DEEP pcap directly with udpreplay or tcpreplay, which gives you a
  second equity protocol (IEX-TP plus DEEP) for free and forces the normaliser to be
  general.
- **CME:** decode Databento MBO or a CME sample pcap and re-emit as MDP 3.0 incremental
  refresh packets using the public SBE schema, with A and B feeds and a snapshot
  channel.
- **Fault injection:** drop, duplicate, reorder, and delay packets on demand. This is
  the whole reason to own the simulator; live data never breaks when you want it to.

### 3.2 Order entry: your own matching engine speaking a real protocol

Nobody outside a member firm sends OUCH. But OUCH over SoupBinTCP is a small, public
spec, and implementing the exchange side is a weekend of work once the book exists.

- Matching engine with price-time priority, fed by the replayed ITCH stream, so your
  simulated orders sit in the real historical queue and get filled when real volume
  trades through them.
- Accepts OUCH enter, replace, and cancel over SoupBinTCP; returns accepted, executed,
  cancelled. Your OMS then speaks a real exchange protocol end to end.
- Later, swap the wire protocol for iLink 3 (SBE over TCP) to show the OMS is
  protocol-agnostic.

### 3.3 Network stack: practise kernel bypass on cheap hardware

Three tiers, pick by budget and interest:

1. **No hardware:** Linux sockets tuned properly (busy poll, SO_BUSY_POLL, pinned
   cores, isolcpus, huge pages), then AF_XDP on any XDP-capable NIC including a VPS
   virtio adapter in some cases. Teaches the ideas.
2. **One used card, roughly 100 USD:** Solarflare SFN8522 in a small Linux box with
   two ports looped back, OpenOnload for transparent acceleration and ef_vi for the
   true zero-copy API. This is literally the stack many firms run.
3. **Alternative card:** Mellanox ConnectX-4 Lx with libvma or DPDK.

Measure before and after with the same histogram harness: receive timestamp to
book updated. The before and after numbers are the resume line.

### 3.4 Time: PTP on the home network

linuxptp between two machines, hardware timestamps if the NIC supports them. Cheap,
and it is how every nanosecond timestamp in the industry is produced.

### 3.5 Certification: write your own conformance tests

Exchanges certify with scripted scenarios: enter, partial fill, cancel-replace,
reject on bad price, behaviour across a gap. Write the same scenarios as integration
tests against your matching engine. It is the same discipline without the port fee.

## 4. Open-source references worth reading (not copying)

| Project | Language | What it shows |
|---|---|---|
| paritytrading/nassau | Java | MoldUDP64, SoupBinTCP, BinaryFILE implementations. Small and clean. |
| paritytrading/juncture | Java | ITCH, OUCH, PITCH message codecs. |
| paritytrading/parity | Java | A complete trading venue: matching engine, order entry gateway, market data publisher. The reference architecture for your exchange simulator. |
| penberg/libtrading | C | ITCH, OUCH, PITCH, FIX, FAST codecs in C. Closest to what you will write. |
| real-logic/simple-binary-encoding | Java, C++ | The reference SBE codec generator; use it to generate CME MDP 3.0 and iLink 3 decoders from the official schema. |
| Xilinx-CNS/onload | C | OpenOnload and ef_vi. Read the ef_vi examples to see what a zero-copy receive loop looks like. |
| Mellanox/libvma | C | The NVIDIA equivalent. |
| nkaz001/hftbacktest | Rust, Python | L3 backtesting with queue-position fills and feed and order latency models. Reads Databento MBO. Useful for validating your matching simulator's fill logic. |
| LOBSTER samples | data | Independently reconstructed Nasdaq books to diff against your own. |

## 5. What this changes in the AlphaDesk design

1. **Keep the replay-first hot path.** Confirmed: live L3 equity data is not available to
   individuals at any price, so the exchange simulator is not a compromise, it is the
   correct design.
2. **Add OUCH over SoupBinTCP as the order-entry protocol** between the OMS and the
   matching simulator, instead of an internal interface. The OMS then speaks a real
   exchange protocol, which is a stronger interview story and a cleaner boundary.
3. **Add IEX DEEP pcap replay** in month 2 as the second equity protocol. It is free,
   recent, and arrives as real packets, so the feed handler sees two different framings
   before CME in month 4.
4. **Make the month 3 hardware choice explicit:** a used Solarflare SFN8522 plus a
   small Linux box, roughly 250 to 400 USD total, instead of or in addition to a VPS.
   It is the one purchase that turns "I read about kernel bypass" into "I measured it."
5. **CME samples:** request the sample pcap from CME Data Sales early, since it takes
   human turnaround, and keep Databento MBO as the fallback.
6. **Leave Rithmic out** unless month 8 wants a faster futures paper venue than IBKR.

## 6. Sources

- Databento live data and latency: https://databento.com/live, https://databento.com/latency
- Databento Nasdaq TotalView live (business entities only): https://databento.com/blog/nasdaq-totalview-live
- Databento equities and futures: https://databento.com/equities, https://databento.com/futures
- Nasdaq free ITCH days: https://emi.nasdaq.com/ITCH/
- Nasdaq Test Facility guide: https://www.nasdaq.com/Nasdaq_Test_Facility_NTF_Guide
- Nasdaq OUCH 5.0 FAQ: https://m.nasdaqtrader.com/content/productsservices/trading/OUCH_5.0_FAQ.pdf
- CME packet capture dataset and sample pcap process: https://cmegroupclientsite.atlassian.net/wiki/display/EPICSANDBOX/Packet+Capture+Dataset
- CME iLink 3 migration and certification: https://cmegroupclientsite.atlassian.net/wiki/spaces/EPICSANDBOX/pages/46215155/Mandatory+Migration+-+iLink+3+Binary+Order+Entry+Protocol+for+CGW
- Cboe Multicast PITCH spec: https://cdn.cboe.com/resources/membership/US_EQUITIES_OPTIONS_MULTICAST_PITCH_SPECIFICATION.pdf
- Cboe DataShop PITCH pricing: https://datashop.cboe.com/cboe-us-equities-pitch
- IEX market data and HIST: https://www.iexexchange.io/products/market-data-connectivity
- Rithmic via FCMs: https://www.ampfutures.com/trading-platform/rithmic-r-api, https://www.ironbeam.com/?p=10677
- OpenOnload: https://github.com/Xilinx-CNS/onload
- libvma: https://github.com/Mellanox/libvma
- AF_XDP latency study: https://ar5iv.labs.arxiv.org/html/2402.10513
- paritytrading: https://github.com/paritytrading/nassau, https://github.com/paritytrading/juncture, https://github.com/paritytrading/parity
- libtrading: https://github.com/penberg/libtrading
- Simple Binary Encoding: https://github.com/real-logic/simple-binary-encoding
- hftbacktest: https://github.com/nkaz001/hftbacktest
- LOBSTER samples: https://lobsterdata.com/info/DataSamples.php
- udpreplay: https://github.com/pkarneliuk/udpreplay
