#pragma once
#include <string>
#include <vector>

class InfoSet {
public:
    std::string key;
    std::vector<float> regretSum;
    std::vector<float> strategy;
    std::vector<float> strategySum;

    // Constructor: Kuhn Poker has 2 actions (Pass=0, Bet=1)
    InfoSet(int numActions = 2) {
        regretSum.resize(numActions, 0.0f);
        strategy.resize(numActions, 0.0f);
        strategySum.resize(numActions, 0.0f);
    }

    // Regret Matching Math: Converts accumulated regrets into a probability distribution
    std::vector<float> getStrategy(float realizationWeight) {
        float normalizingSum = 0.0f;
        
        // Only consider positive regrets
        for (size_t i = 0; i < strategy.size(); i++) {
            strategy[i] = regretSum[i] > 0 ? regretSum[i] : 0.0f;
            normalizingSum += strategy[i];
        }

        // Normalize to create a probability distribution that sums to 1.0
        for (size_t i = 0; i < strategy.size(); i++) {
            if (normalizingSum > 0) {
                strategy[i] /= normalizingSum;
            } else {
                // If there is no positive regret yet, pick an action uniformly at random
                strategy[i] = 1.0f / strategy.size();
            }
            // Track the strategy sum to compute the final Nash Equilibrium later
            strategySum[i] += realizationWeight * strategy[i];
        }
        return strategy;
    }

    // Returns the final optimal strategy after all iterations are done
    std::vector<float> getAverageStrategy() {
        std::vector<float> avgStrategy(strategy.size(), 0.0f);
        float normalizingSum = 0.0f;
        
        for (float sum : strategySum) normalizingSum += sum;
        
        for (size_t i = 0; i < strategy.size(); i++) {
            if (normalizingSum > 0) {
                avgStrategy[i] = strategySum[i] / normalizingSum;
            } else {
                avgStrategy[i] = 1.0f / strategy.size();
            }
        }
        return avgStrategy;
    }
};