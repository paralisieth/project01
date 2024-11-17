/******************************************************************************
 * File: player.hpp
 * Author: Pizza Game Development Team
 * Created: 2024
 * 
 * Description:
 * Player class that manages the player's state in the pizza game simulation.
 * Tracks player's money and reputation throughout the game.
 * 
 * Copyright (c) 2024. All rights reserved.
 *****************************************************************************/

#ifndef PLAYER_HPP
#define PLAYER_HPP

/**
 * @class Player
 * @brief Manages player state and resources
 * 
 * Handles player's money and reputation, providing methods to
 * modify and retrieve these values during gameplay.
 */
class Player {
public:
    /** Constructor initializes player's starting state */
    Player();
    
    // Getters
    /** @return Current amount of money player has */
    double getMoney() const;
    /** @return Current reputation level */
    int getReputation() const;
    
    // Resource Management
    /** @brief Add money to player's account
     *  @param amount Amount to add */
    void addMoney(double amount);
    /** @brief Remove money from player's account
     *  @param amount Amount to subtract */
    void subtractMoney(double amount);
    /** @brief Increase player's reputation
     *  @param amount Amount to add */
    void addReputation(int amount);

private:
    double money;     ///< Player's current money
    int reputation;   ///< Player's current reputation
};

#endif // PLAYER_HPP
