#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <thread>  // <-- NEW: Multi-threading library
#include <mutex>   // <-- NEW: Memory locking library
#include "leduc.h"
#include "node.h"

class MCCFRTrainer {
private:
    std::unordered_map<std::string, InfoSet> nodeMap;
    std::mutex mapMutex; // <-- NEW: The traffic light for our Hash Map

    float mccfr(BitHistory history, int8_t p1Card, int8_t p2Card, int8_t boardCard, 
                float p1Prob, float p2Prob, bool isPostFlop);

public:
    void train(int iterations);
    void trainMultiThreaded(int totalIterations, int numThreads); // <-- NEW!
    void printStrategy();
    void playAgainstHuman();
    int sampleAIAction(std::vector<float> strategy);

    void saveModel(const std::string& filename);
    bool loadModel(const std::string& filename);
};