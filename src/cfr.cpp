#include "../include/cfr.h"
#include <iostream>
#include <random>

float CFRTrainer::cfr(const std::string& history, Card p1Card, Card p2Card, float p1Prob, float p2Prob) {
    int plays = history.length();
    int player = plays % 2; // 0 for Player 1, 1 for Player 2

    // 1. Terminal State Check: If the game is over, return the payoff
    if (KuhnGame::isTerminal(history)) {
        float payoff = KuhnGame::getPayoff(history, p1Card, p2Card);
        // If it's Player 2's perspective, flip the zero-sum payoff
        return player == 0 ? payoff : -payoff;
    }

    // 2. Identify the Information Set
    // e.g., "0P" means we hold a Jack (0) and the history is Pass (P)
    Card myCard = (player == 0) ? p1Card : p2Card;
    std::string infoSetKey = std::to_string(static_cast<int>(myCard)) + history;

    // If we haven't seen this situation before, add it to the brain
    if (nodeMap.find(infoSetKey) == nodeMap.end()) {
        nodeMap.insert({infoSetKey, InfoSet(2)});
    }
    InfoSet& node = nodeMap[infoSetKey];

    // 3. Get Current Strategy
    float realizationWeight = (player == 0) ? p1Prob : p2Prob;
    std::vector<float> strategy = node.getStrategy(realizationWeight);

    // 4. Traverse the Tree (Recursive Forward Pass)
    std::vector<float> actionUtils(2, 0.0f);
    float nodeUtil = 0.0f;

    for (int a = 0; a < 2; a++) {
        std::string nextHistory = history + (a == 0 ? "P" : "B");
        
        // Recurse down the tree, multiplying the probability by the chosen action
        if (player == 0) {
            actionUtils[a] = -cfr(nextHistory, p1Card, p2Card, p1Prob * strategy[a], p2Prob);
        } else {
            actionUtils[a] = -cfr(nextHistory, p1Card, p2Card, p1Prob, p2Prob * strategy[a]);
        }
        nodeUtil += strategy[a] * actionUtils[a];
    }

    // 5. Update Regrets (Backward Pass)
    for (int a = 0; a < 2; a++) {
        float regret = actionUtils[a] - nodeUtil;
        float counterfactualProb = (player == 0) ? p2Prob : p1Prob;
        node.regretSum[a] += counterfactualProb * regret;
    }

    return nodeUtil;
}

void CFRTrainer::train(int iterations) {
    std::cout << "Training AI for " << iterations << " iterations...\n";
    Card deck[3] = {Card::JACK, Card::QUEEN, Card::KING};

    for (int i = 0; i < iterations; i++) {
        // Deal cards: A very basic shuffle for Kuhn Poker
        for (int j = 0; j < 3; j++) {
            int swapIdx = rand() % 3;
            std::swap(deck[j], deck[swapIdx]);
        }
        
        // Start the recursive CFR traversal from the root node (empty history)
        cfr("", deck[0], deck[1], 1.0f, 1.0f);
    }
    std::cout << "Training Complete!\n\n";
}

void CFRTrainer::printStrategy() {
    std::cout << "--- AI OPTIMAL STRATEGY (NASH EQUILIBRIUM) ---\n";
    for (auto& pair : nodeMap) {
        std::vector<float> avgStrategy = pair.second.getAverageStrategy();
        std::cout << "State [" << pair.first << "] -> ";
        std::cout << "Pass: " << (avgStrategy[0] * 100) << "% | Bet: " << (avgStrategy[1] * 100) << "%\n";
    }
}