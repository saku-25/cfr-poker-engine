#pragma once
#include <cstdint>
#include <string>

// Quant Flex: Using int8_t instead of standard int
enum class Card : int8_t {
    JACK = 0,
    QUEEN = 1,
    KING = 2
};

enum class Action : int8_t {
    PASS = 0,
    BET = 1
};

class KuhnGame {
public:
    // Checks if the current sequence of actions ends the game
    static bool isTerminal(const std::string& history);

    // Calculates the payoff (Expected Value) at the end of a hand
    static float getPayoff(const std::string& history, Card p1_card, Card p2_card);
};