/******************************************************************************
 * File: order.hpp
 * Author: Pizza Game Development Team
 * Created: 2024
 * 
 * Description:
 * Order class that manages individual pizza orders in the game.
 * Handles order tracking, completion status, and price calculation.
 * 
 * Copyright (c) 2024. All rights reserved.
 *****************************************************************************/

#ifndef ORDER_HPP
#define ORDER_HPP

#include "pizza.hpp"
#include <memory>
#include <string>
#include <vector>

/**
 * @class Order
 * @brief Manages individual customer orders
 * 
 * Tracks pizzas in an order, calculates total price,
 * and manages order completion status.
 */
class Order {
public:
    /** Constructor creates a new order with given number
     *  @param orderNumber Unique identifier for this order */
    explicit Order(int orderNumber);
    
    // Order Management
    /** @brief Add a pizza to this order
     *  @param pizza Pizza to add */
    void addPizza(const Pizza& pizza);
    /** @brief Mark order as complete */
    void complete();
    
    // Getters
    /** @return Total price of all pizzas in order */
    double getTotalPrice() const;
    /** @return Whether order is completed */
    bool isComplete() const;
    /** @return Order's unique number */
    int getOrderNumber() const;
    /** @return Reference to pizzas in this order */
    const std::vector<Pizza>& getPizzas() const;
    /** @return String representation of order status */
    std::string getStatus() const;

private:
    int orderNumber;              ///< Unique order identifier
    std::vector<Pizza> pizzas;    ///< Pizzas in this order
    bool completed;               ///< Order completion status
};

#endif // ORDER_HPP
