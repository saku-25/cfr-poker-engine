#include "../include/game.h"

bool KuhnGame::isTerminal(const std::string& history) {
    // Terminal states in Kuhn Poker:
    // "PP"  -> Both pass (Showdown)
    // "BB"  -> Both bet (Showdown)
    // "BP"  -> P1 bets, P2 passes (P2 folds)
    // "PBP" -> P1 passes, P2 bets, P1 passes (P1 folds)
    // "PBB" -> P1 passes, P2 bets, P1 bets (Showdown)
    
    return history == "PP" || history == "BB" || history == "BP" || 
           history == "PBP" || history == "PBB";
}

float KuhnGame::getPayoff(const std::string& history, Card p1_card, Card p2_card) {
    // Cast to int8_t to easily compare card strengths (0 = Jack, 1 = Queen, 2 = King)
    bool p1_has_higher = (static_cast<int8_t>(p1_card) > static_cast<int8_t>(p2_card));
    
    // 1. Fold Cases (No showdown)
    if (history == "BP") return 1.0f;       // P1 bet, P2 folded. P1 wins the 1 ante.
    if (history == "PBP") return -1.0f;     // P2 bet, P1 folded. P1 loses the 1 ante.
    
    // 2. Showdown Cases
    // If "PP", the pot is 1 chip. If it contains a "B", both players bet, so pot is 2 chips.
    float amount = (history == "PP") ? 1.0f : 2.0f;
    
    // Return positive amount if P1 wins, negative if P1 loses
    return p1_has_higher ? amount : -amount;
}