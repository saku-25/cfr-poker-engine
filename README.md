# C++ Algorithmic Game Theory & CFR Trading Engine

A high-performance C++ engine that solves imperfect-information stochastic games (Kuhn Poker, Leduc Hold'em) by mathematically deriving the Nash Equilibrium from scratch. 

Built entirely without standard ML libraries, this project implements **Counterfactual Regret Minimization (CFR)** and **External-Sampling Monte Carlo CFR (MCCFR)**. It is heavily optimized for low-latency execution and memory efficiency, mirroring the architecture of high-frequency trading (HFT) systems.

## 🧠 Core Mathematical Architecture
* **Counterfactual Regret Minimization (CFR):** Iteratively updates probability distributions over actions based on accumulated positive regret to converge on a theoretical Nash Equilibrium.
* **Monte Carlo Chance Sampling (MCCFR):** Bypasses exponential tree explosion by stochastically sampling "Nature" nodes (card deals) during the traversal phase, significantly accelerating training speed.
* **Zero-Sum Expected Value (EV):** Implements strict U1 = -U2 mathematical dynamics to halve computational overhead during terminal state evaluations.

## ⚡ Low-Level System Optimizations
To handle the state-space explosion inherent in Poker environments, the engine implements several C++ specific optimizations:
* **Isomorphic State-Space Abstraction:** Maps mathematically identical states (e.g., holding a Jack on a Queen board vs. a Jack of a different suit) to a single `InfoSet` bucket, compressing the Hash Map memory footprint by 50%.
* **Bitwise State Representation:** Replaced dynamic `std::string` allocations with hardware-level bitwise operations. Entire game histories are packed into a single `uint16_t` integer via left-shift and OR operations, eliminating dynamic heap allocations and CPU cache misses.
* **Thread-Safe Parallelization:** Distributed the Monte Carlo training workload across 8 parallel CPU cores using the `<thread>` library. Mitigated race conditions and memory corruption via `std::mutex` locking on the shared Hash Map.
* **Production Serialization Pipeline:** Decoupled the "Research" environment from the "Live Execution" environment. The engine distills millions of regret calculations and exports only the final probability arrays to a lightweight `.csv`, enabling microsecond loading in the production interface.

## 🚀 Build Instructions
This project uses CMake for cross-platform compilation.

**1. Clone the repository:**
`git clone https://github.com/yourusername/cfr-poker-engine.git`
`cd cfr-poker-engine`

**2. Configure and Build (Release Mode for Maximum Speed):**
`cmake -DCMAKE_BUILD_TYPE=Release ..`
`cmake --build . --config Release`

**3. Run the Engine:**
`.\Release\cfr_poker.exe`

## 📊 Example Output (Nash Equilibrium)
When trained on Kuhn Poker, the engine naturally discovers advanced game theory concepts without any hardcoded rules, such as balancing its range by bluffing with the worst card 20-30% of the time, and calling with a medium-strength card ~33% of the time to make the opponent's EV of bluffing exactly zero.

State [0] (Jack)  -> Pass: 77.29% | Bet: 22.70%  <- (Optimal Bluffing Range)
State [2] (King)  -> Pass: 30.59% | Bet: 69.40%  <- (Value Betting)
State [1B] (Queen facing bet) -> Pass: 65.45% | Call: 34.54% <- (Point of Indifference)
