# Notes: what HFT firms use, what I can get, and how to fake the rest

Written 2026-10-18. Plain-language notes for myself.

## How the big firms get data and send orders

They put their computers in the same building as the exchange's computers.
The exchange sends them a stream of every order as it happens, in a compact
binary format. They send their orders back on another direct connection.
Every message has a sequence number so you can tell if one got lost.

Names worth knowing, because I will see them in specs and interviews:

- **ITCH**: Nasdaq's market data format. One message per order event
  (add, cancel, execute).
- **MoldUDP64**: the packaging ITCH travels in. Groups messages into
  network packets with sequence numbers.
- **OUCH**: Nasdaq's format for sending orders.
- **MDP 3.0**: CME's market data format for futures.
- **iLink 3**: CME's format for sending orders.
- **Multicast**: a way to send one network packet to many listeners at once.
  Exchanges use it for data.
- **Kernel bypass**: letting a program read packets straight from the network
  card instead of going through the operating system. Saves microseconds.

All of these formats are written up in free PDFs on the exchanges' websites.
Anyone can write code that reads them.

## What I can actually get

**Free**

- 15 full days of real Nasdaq ITCH data, from 2018 to 2020.
  https://emi.nasdaq.com/ITCH/ (about 5 GB per day, compressed)
- Daily recordings of the IEX exchange's feed, last 12 months.
  https://www.iexexchange.io/products/market-data-connectivity
- Every protocol spec.
- The kernel-bypass software firms use. It is open source:
  https://github.com/Xilinx-CNS/onload
- Alpaca: live prices for 30 stocks, a practice account, and real trading
  with no commissions. https://alpaca.markets
- WRDS, through UMD: decades of stock prices and company financials.
  I already have access.

**Cheap**

- CME futures history from Databento. 125 USD free credit, then about
  0.50 USD per GB. https://databento.com/futures
- A used Solarflare network card, the kind firms use, for 50 to 150 USD.
  Lets me run the real kernel-bypass software at home.
- Cloud credits from the GitHub Student Developer Pack, enough for a Linux
  server for the year. https://education.github.com/pack

**Not available to me at any price**

- The live Nasdaq order stream. Only sold to companies.
- Direct order connections to any exchange. You need to be a member.
- Exchange test environments. Same reason.

## Live data

I can get live data, just not the colo feed. Everything live comes over the
internet, so it arrives in milliseconds, not microseconds. That is fine. Live
data is for running the strategy for real. The microsecond numbers come from
replaying recorded data.

- **Now, free**: Alpaca's live feed. Top-of-book prices for 30 stocks.
- **Later, 199 USD/month**: Databento's live CME futures feed. Every order,
  with sequence numbers, in a binary format, with a C++ client. The closest
  thing to a real exchange feed I can buy. One or two months of this, once my
  CME code exists, lets me say I ran it on live exchange data.
  https://databento.com/live
- **Optional, about 15 USD/month**: full Nasdaq depth through a broker like
  IBKR. Real depth, but slowed down and bundled by the broker.

## How to fake what I can't get

This is how the firms test their own systems, so it is not cheating.

1. **Build a fake exchange.** Read the free ITCH file and broadcast it on my
   own network exactly the way Nasdaq does: same packet format, same sequence
   numbers. My real code listens and can't tell the difference. I can also
   drop or shuffle packets on purpose to test my recovery code.
2. **Build a fake order gateway.** A small program that accepts orders in
   Nasdaq's real OUCH format and matches them against the recorded stream.
   My orders wait in line behind the real historical orders.
3. **One used network card** in a cheap Linux box, running the open-source
   kernel-bypass software. Measure before and after.
4. **My own test scripts** that do what exchange certification does: place,
   cancel, replace, handle a gap.

## Code worth reading first

- https://github.com/paritytrading/parity: a whole small exchange in Java.
  The blueprint for my fake exchange.
- https://github.com/paritytrading/nassau: MoldUDP64, small and clean.
- https://github.com/penberg/libtrading: ITCH and OUCH readers in C.
  Closest to what I will write.

## Student perks to chase, in order

1. GitHub Student Developer Pack. Five minutes. Covers the server.
2. Check which datasets my WRDS login includes. Want: CRSP (prices),
   Compustat (financials), TAQ (intraday ticks), OptionMetrics (options).
3. Ask the Smith School business librarian whether UMD has LOBSTER, which
   is Nasdaq order books rebuilt from ITCH. Useful to check my own book code.
4. Email Nasdaq and ask if students get recent ITCH files free. One source
   says yes; Nasdaq's page doesn't say.
5. UMD's Zaratan cluster for big replay jobs, through a professor.

## What this changes in my plan

- Keep the replay-first design. It is the right design, not a fallback.
- Have my order code speak real OUCH to the fake gateway.
- Add the IEX recordings in month 2 as a second data format.
- Budget a Linux box plus one used network card in month 3, 250 to 400 USD.
- Use WRDS for stage 3 data instead of free sources.
