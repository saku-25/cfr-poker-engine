#pragma once
#include <cstdint>
#include <iostream>
#include <string>

// Quant Flex: Packing the entire game history into 3 bytes of memory
class BitHistory {
private:
    uint16_t actions; // Stores the actual sequence of actions as bits
    uint8_t length;   // Tracks how many actions have been taken

public:
    // Initialize empty hand
    BitHistory() : actions(0), length(0) {}

    // action: 0 for Pass, 1 for Bet
    void addAction(uint8_t action) {
        // Left shift the action to the correct bit position, then OR it into the integer
        actions |= (action << length);
        length++;
    }

    // Retrieve what happened at a specific turn (0-indexed)
    uint8_t getAction(uint8_t turn) const {
        // Shift the requested bit back to the right and isolate it with an AND 1 mask
        return (actions >> turn) & 1;
    }

    uint8_t getLength() const { return length; }

    // Used for hashing in the CFR node map later
    uint16_t getRawHistory() const { return actions; }
};

// 6-Card Deck:
// 0 = Jack 1,  1 = Jack 2
// 2 = Queen 1, 3 = Queen 2
// 4 = King 1,  5 = King 2

class LeducAbstraction {
public:
    // Takes the raw card index and returns its Rank (0=J, 1=Q, 2=K)
    static int8_t getRank(int8_t card) {
        return card / 2;
    }

    // Maps a specific hand to an isomorphic bucket
    // boardCard = -1 means we are currently in the Pre-Flop betting round
    static std::string getBucket(int8_t playerCard, int8_t boardCard) {
        int8_t pRank = getRank(playerCard);
        std::string bucket = std::to_string(pRank);

        // If a community card has been dealt, append it to the bucket
        if (boardCard != -1) {
            int8_t bRank = getRank(boardCard);
            bucket += std::to_string(bRank);
        }

        return bucket;
    }
};