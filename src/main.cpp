#include <iostream>
#include <ctime>
#include "../include/mccfr.h"

int main() {
    srand(static_cast<unsigned>(time(0)));

    std::cout << "--- Quant Leduc Hold'em Engine Initialized ---\n\n";

    MCCFRTrainer ai;
    std::string modelFile = "leduc_model.csv";

    // Attempt to load a pre-trained brain
    if (ai.loadModel(modelFile)) {
        std::cout << "\nSkipping training phase. Booting directly into inference...\n";
    } else {
        std::cout << "No pre-trained model found. Initiating Heavy Training Phase...\n";
        
        // Train 10,000,000 hands split across 8 parallel threads!
        ai.trainMultiThreaded(10000000, 8);
        
        // Save the brain to the hard drive so we never have to wait again
        ai.saveModel(modelFile);
    }
    
    // Enter the live execution environment
    ai.playAgainstHuman();

    return 0;
}