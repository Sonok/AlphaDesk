# AlphaDesk

A platform that reads real exchange data, replays it through a simulator, finds
and tests alphas, builds a portfolio from the good ones, and trades it with a
small amount of real money. Built in C++ and Python as a way to learn what
quant researchers and quant developers do.

## The plan

Goal: trading live, small, by the end of June 2027. Total budget 1,000 USD.

| When | Build |
|---|---|
| Now to end of Dec 2026 | C++: decode a Nasdaq ITCH file and build the order book |
| January | C++: simulator that replays the file through a strategy |
| February | Python: first alphas on daily stock data, plus a testing kit |
| March | Alphas from my own order book data, tested in my simulator |
| April | Combine the alphas with ML and build a portfolio under limits |
| May | Paper trade every day |
| June | Go live, small |

After June, when there is more time: an ITCH reader on an FPGA, and letting
other people submit alphas.

The full plan, with the budget and the rules for real money, is in
[docs/AlphaDesk-plan.pdf](docs/AlphaDesk-plan.pdf).

## Docs

- [docs/DESIGN.md](docs/DESIGN.md): the four stages of the platform.
- [docs/research/hft-access-and-simulation.md](docs/research/hft-access-and-simulation.md):
  what data I can get and how firms work.
