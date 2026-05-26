#pragma once
#include <unordered_map>
#include <string>
#include "game.h"
#include "node.h"

class CFRTrainer {
private:
    // This Hash Map is the AI's "brain". It maps a situation (e.g., "0P") to an InfoSet
    std::unordered_map<std::string, InfoSet> nodeMap;

    // The recursive function that traverses the game tree
    float cfr(const std::string& history, Card p1Card, Card p2Card, float p1Prob, float p2Prob);

public:
    // Runs the algorithm for N iterations
    void train(int iterations);
    
    // Prints the final optimal strategy
    void printStrategy();
};