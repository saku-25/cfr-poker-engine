#include "../include/mccfr.h"
#include <iostream>
#include <random>
#include <iomanip>
#include <fstream>
#include <sstream>

void MCCFRTrainer::train(int iterations) {
    std::cout << "Starting Monte Carlo CFR for " << iterations << " iterations...\n";

    for (int i = 0; i < iterations; i++) {
        // 1. MONTE CARLO CHANCE SAMPLING
        int8_t deck[6] = {0, 1, 2, 3, 4, 5}; 

        for (int j = 0; j < 6; j++) {
            int swapIdx = rand() % 6;
            std::swap(deck[j], deck[swapIdx]);
        }

        int8_t p1Card = deck[0];
        int8_t p2Card = deck[1];
        int8_t boardCard = deck[2];

        // 2. TRAVERSE
        BitHistory emptyHistory;
        mccfr(emptyHistory, p1Card, p2Card, boardCard, 1.0f, 1.0f, false);
        
        if (i > 0 && i % 100000 == 0) {
            std::cout << "Completed " << i << " iterations. Brain size: " 
                      << nodeMap.size() << " Information Sets.\n";
        }
    }
    std::cout << "\nMCCFR Training Complete! The AI has solved Leduc Hold'em.\n\n";
}

float MCCFRTrainer::mccfr(BitHistory history, int8_t p1Card, int8_t p2Card, int8_t boardCard, 
                          float p1Prob, float p2Prob, bool isPostFlop) {
    
    int plays = history.getLength();
    int player = plays % 2; 
    int8_t myCard = (player == 0) ? p1Card : p2Card;
    int8_t oppCard = (player == 0) ? p2Card : p1Card;

    // --- 1. TERMINAL STATE CHECKS ---
    bool isFold = false;
    bool isShowdown = false;
    float pot = isPostFlop ? 2.0f : 1.0f; // Bets double on the flop

    if (plays >= 2) {
        uint8_t lastAction = history.getAction(plays - 1);
        uint8_t prevAction = history.getAction(plays - 2);

        if (lastAction == 0 && prevAction == 1) isFold = true;       // Bet -> Pass (Fold)
        else if (lastAction == 1 && prevAction == 1) isShowdown = true; // Bet -> Bet (Call)
        else if (lastAction == 0 && prevAction == 0) isShowdown = true; // Pass -> Pass
    }

    if (isFold) return 1.0f; // Active player wins the previous bets

    // --- 2. THE FLOP & SHOWDOWN MATH ---
    if (isShowdown) {
        if (!isPostFlop) {
            // THE FLOP: Round 1 ends. Reset history and start Round 2 with the board card.
            BitHistory postFlopHistory;
            return mccfr(postFlopHistory, p1Card, p2Card, boardCard, p1Prob, p2Prob, true);
        } else {
            // THE SHOWDOWN: Compare Isomorphic Ranks
            int8_t myRank = LeducAbstraction::getRank(myCard);
            int8_t oppRank = LeducAbstraction::getRank(oppCard);
            int8_t boardRank = LeducAbstraction::getRank(boardCard);

            bool iPair = (myRank == boardRank);
            bool oppPair = (oppRank == boardRank);

            if (iPair && !oppPair) return pot;
            if (!iPair && oppPair) return -pot;
            if (myRank > oppRank) return pot;
            if (myRank < oppRank) return -pot;
            return 0.0f; // Tie
        }
    }

    // --- 3. ABSTRACTION & INFORMATION SET LOOKUP ---
    std::string bucket = LeducAbstraction::getBucket(myCard, isPostFlop ? boardCard : -1);
    std::string infoSetKey = bucket + "_" + std::to_string(history.getRawHistory());

    // ---> THIS IS THE MISSING LINE WE NEED TO ADD BACK <---
    float realizationWeight = (player == 0) ? p1Prob : p2Prob;

    mapMutex.lock();  // <-- LOCK THE MEMORY (Red Light)
    if (nodeMap.find(infoSetKey) == nodeMap.end()) {
        nodeMap.insert({infoSetKey, InfoSet(2)});
    }
    // We copy the strategy out of the map so we can release the lock safely
    std::vector<float> strategy = nodeMap[infoSetKey].getStrategy(realizationWeight);
    mapMutex.unlock(); // <-- UNLOCK BEFORE RECURSING (Green Light)

    // --- 4 & 5. RECURSIVE FORWARD PASS ---
    std::vector<float> actionUtils(2, 0.0f);
    float nodeUtil = 0.0f;

    for (int a = 0; a < 2; a++) {
        BitHistory nextHistory = history;
        nextHistory.addAction(a);

        if (player == 0) {
            actionUtils[a] = -mccfr(nextHistory, p1Card, p2Card, boardCard, p1Prob * strategy[a], p2Prob, isPostFlop);
        } else {
            actionUtils[a] = -mccfr(nextHistory, p1Card, p2Card, boardCard, p1Prob, p2Prob * strategy[a], isPostFlop);
        }
        nodeUtil += strategy[a] * actionUtils[a];
    }

    // --- RECURSIVE BACKWARD PASS (Update Regrets) ---
    mapMutex.lock(); // <-- LOCK THE MEMORY TO WRITE (Red Light)
    for (int a = 0; a < 2; a++) {
        float regret = actionUtils[a] - nodeUtil;
        float counterfactualProb = (player == 0) ? p2Prob : p1Prob;
        nodeMap[infoSetKey].regretSum[a] += counterfactualProb * regret;
    }
    mapMutex.unlock(); // <-- UNLOCK (Green Light)

    return nodeUtil;
}

void MCCFRTrainer::printStrategy() {
    std::cout << "--- LEDUC HOLD'EM SAMPLE STRATEGY ---\n";
    std::cout << "(Showing a few key Post-Flop scenarios)\n\n";
    
    // Let's just look at what it does when holding a Pair of Kings (Highest possible hand)
    std::string targetBucket = "22"; // King + King
    
    for (auto& pair : nodeMap) {
        if (pair.first.substr(0, 2) == targetBucket) {
            std::vector<float> avgStrategy = pair.second.getAverageStrategy();
            std::cout << "State [" << std::setw(6) << std::left << pair.first << "] -> ";
            std::cout << "Pass: " << std::fixed << std::setprecision(2) << (avgStrategy[0] * 100) 
                      << "% | Bet: " << (avgStrategy[1] * 100) << "%\n";
        }
    }
}

// Helper function to make the CLI readable
std::string getCardName(int8_t card) {
    if (card <= 1) return "Jack";
    if (card <= 3) return "Queen";
    return "King";
}

// Samples an action using the AI's probability distribution
// Samples an action using the AI's probability distribution
int MCCFRTrainer::sampleAIAction(std::vector<float> strategy) {
    float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    float cumulativeProbability = 0.0f;
    for (size_t a = 0; a < strategy.size(); a++) {
        cumulativeProbability += strategy[a];
        // FIX: Cast the size_t 'a' to an int
        if (r < cumulativeProbability) return static_cast<int>(a); 
    }
    // FIX: Cast the size_t to an int
    return static_cast<int>(strategy.size() - 1); 
}

void MCCFRTrainer::playAgainstHuman() {
    std::cout << "\n==========================================\n";
    std::cout << "      ENTERING THE ARENA: HUMAN VS AI       \n";
    std::cout << "==========================================\n";

    while (true) {
        // 1. Deal Cards
        int8_t deck[6] = {0, 1, 2, 3, 4, 5};
        for (int j = 0; j < 6; j++) {
            int swapIdx = rand() % 6;
            std::swap(deck[j], deck[swapIdx]);
        }
        int8_t humanCard = deck[0];
        int8_t aiCard = deck[1];
        int8_t boardCard = deck[2];

        std::cout << "\n--- NEW HAND ---\n";
        std::cout << "You are dealt: [" << getCardName(humanCard) << "]\n";
        
        BitHistory history;
        bool isPostFlop = false;
        bool handOver = false;

        // 2. The Game Loop
        while (!handOver) {
            int plays = history.getLength();
            int actingPlayer = plays % 2; // 0 = Human, 1 = AI

            // Check for Terminal States (Folds or Showdowns)
            if (plays >= 2) {
                uint8_t lastAction = history.getAction(plays - 1);
                uint8_t prevAction = history.getAction(plays - 2);
                
                if (lastAction == 0 && prevAction == 1) { // Bet -> Pass (Fold)
                    std::cout << (actingPlayer == 0 ? "AI Folds! You win." : "You Fold! AI wins.") << "\n";
                    handOver = true;
                    break;
                } else if ((lastAction == 1 && prevAction == 1) || (lastAction == 0 && prevAction == 0)) { // Call or Check
                    if (!isPostFlop) {
                        std::cout << "\n--- THE FLOP ---\n";
                        std::cout << "Community Card revealed: [" << getCardName(boardCard) << "]\n";
                        isPostFlop = true;
                        history = BitHistory(); // Reset history for Post-Flop
                        continue;
                    } else {
                        // SHOWDOWN
                        std::cout << "\n--- SHOWDOWN ---\n";
                        std::cout << "AI reveals: [" << getCardName(aiCard) << "]\n";
                        
                        int8_t hRank = LeducAbstraction::getRank(humanCard);
                        int8_t aRank = LeducAbstraction::getRank(aiCard);
                        int8_t bRank = LeducAbstraction::getRank(boardCard);
                        
                        bool hPair = (hRank == bRank);
                        bool aPair = (aRank == bRank);
                        
                        if (hPair && !aPair) std::cout << "You win with a Pair!\n";
                        else if (!hPair && aPair) std::cout << "AI wins with a Pair!\n";
                        else if (hRank > aRank) std::cout << "You win with High Card!\n";
                        else if (hRank < aRank) std::cout << "AI wins with High Card!\n";
                        else std::cout << "It's a Tie!\n";
                        
                        handOver = true;
                        break;
                    }
                }
            }

            // 3. Take Action
            if (actingPlayer == 0) { // Human's Turn
                int action;
                std::cout << "Your Turn -> Enter 0 to Pass/Check, 1 to Bet/Call: ";
                std::cin >> action;
                history.addAction(action);
            } else { // AI's Turn
                std::string bucket = LeducAbstraction::getBucket(aiCard, isPostFlop ? boardCard : -1);
                std::string infoSetKey = bucket + "_" + std::to_string(history.getRawHistory());
                
                std::vector<float> aiStrategy;
                if (nodeMap.find(infoSetKey) != nodeMap.end()) {
                    aiStrategy = nodeMap[infoSetKey].getAverageStrategy();
                } else {
                    aiStrategy = {0.5f, 0.5f}; // Fallback if state unseen
                }
                
                int aiAction = sampleAIAction(aiStrategy);
                std::cout << "AI chooses to: " << (aiAction == 0 ? "Pass/Check" : "Bet/Call") << "\n";
                history.addAction(aiAction);
            }
        }
        
        std::string playAgain;
        std::cout << "\nPlay another hand? (y/n): ";
        std::cin >> playAgain;
        if (playAgain != "y") break;
    }
}


void MCCFRTrainer::trainMultiThreaded(int totalIterations, int numThreads) {
    std::cout << "\n--- INITIATING HFT PARALLEL TRAINING ---\n";
    std::cout << "Spawning " << numThreads << " threads for " << totalIterations << " iterations...\n";

    // This is the job each thread will do
    auto worker = [&](int iters) {
        // Quant Flex: Standard C++ rand() is NOT thread-safe! 
        // We must initialize a dedicated random number generator for each CPU core.
        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, 5);

        for (int i = 0; i < iters; i++) {
            int8_t deck[6] = {0, 1, 2, 3, 4, 5}; 
            for (int j = 0; j < 6; j++) {
                int swapIdx = dist(rng);
                std::swap(deck[j], deck[swapIdx]);
            }
            BitHistory emptyHistory;
            mccfr(emptyHistory, deck[0], deck[1], deck[2], 1.0f, 1.0f, false);
        }
    };

    std::vector<std::thread> threads;
    int itersPerThread = totalIterations / numThreads;

    // Spawn the threads
    for (int t = 0; t < numThreads; t++) {
        threads.push_back(std::thread(worker, itersPerThread));
    }

    // Wait for all threads to finish their workload before continuing
    for (auto& th : threads) {
        th.join();
    }

    std::cout << "Parallel Training Complete! Brain size: " << nodeMap.size() << " Information Sets.\n";
}

// Quant Flex: Exporting only the final probabilities (Model Distillation)
void MCCFRTrainer::saveModel(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file for saving!\n";
        return;
    }

    // Write the CSV Header
    file << "StateKey,PassProbability,BetProbability\n";

    for (auto& pair : nodeMap) {
        // We only save the final Average Strategy, stripping away all the heavy training data
        std::vector<float> avgStrat = pair.second.getAverageStrategy();
        file << pair.first << "," << avgStrat[0] << "," << avgStrat[1] << "\n";
    }

    file.close();
    std::cout << "Model successfully saved to " << filename << "!\n";
}

bool MCCFRTrainer::loadModel(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false; // File doesn't exist yet

    std::string line, key, token;
    float passProb, betProb;

    // Skip the CSV header line
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        
        std::getline(ss, key, ',');
        
        std::getline(ss, token, ',');
        passProb = std::stof(token);
        
        std::getline(ss, token, ',');
        betProb = std::stof(token);

        // Reconstruct the node in the Hash Map
        InfoSet node(2);
        // We trick the engine by injecting the saved probabilities directly into the strategySum
        node.strategySum[0] = passProb;
        node.strategySum[1] = betProb;
        
        nodeMap[key] = node;
    }

    file.close();
    std::cout << "Pre-trained model loaded from " << filename << "!\n";
    std::cout << "Brain size: " << nodeMap.size() << " Information Sets.\n";
    return true;
}