/******************************************************************************
 * File: pizza.hpp
 * Author: Pizza Game Development Team
 * Created: 2024
 * 
 * Description:
 * Pizza class that represents a customizable pizza in the game.
 * Manages pizza size, toppings, and price calculations.
 * 
 * Copyright (c) 2024. All rights reserved.
 *****************************************************************************/

#ifndef PIZZA_HPP
#define PIZZA_HPP

#include <string>
#include <vector>
#include <unordered_map>

/**
 * @class Pizza
 * @brief Represents a customizable pizza
 * 
 * Manages pizza attributes including size and toppings,
 * and provides functionality for price calculation and
 * pizza customization.
 */
class Pizza {
public:
    /** Available pizza sizes */
    enum class Size { 
        Small,      ///< Small pizza size
        Medium,     ///< Medium pizza size
        Large       ///< Large pizza size
    };
    
    /** Available pizza toppings */
    enum class Topping {
        Cheese,         ///< Basic cheese topping
        Pepperoni,      ///< Pepperoni slices
        Mushrooms,      ///< Sliced mushrooms
        Onions,         ///< Diced onions
        Sausage,        ///< Italian sausage
        Bacon,          ///< Crispy bacon bits
        GreenPeppers,   ///< Sliced green peppers
        Olives          ///< Black olives
    };

    // Static price constants
    static const std::unordered_map<Size, double> BASE_PRICES;  ///< Base price for each size
    static const double TOPPING_PRICE;                          ///< Price per topping

    /** Constructor creates a new pizza with specified size
     *  @param size Initial pizza size (default: Medium) */
    explicit Pizza(Size size = Size::Medium);
    
    // Topping Management
    /** @brief Add a topping to the pizza
     *  @param topping Topping to add */
    void addTopping(Topping topping);
    /** @brief Remove a topping from the pizza
     *  @param topping Topping to remove */
    void removeTopping(Topping topping);
    /** @brief Check if pizza has specific topping
     *  @param topping Topping to check
     *  @return true if topping is present */
    bool hasTopping(Topping topping) const;
    
    // Getters
    /** @return Total price of pizza with toppings */
    double getPrice() const;
    /** @return Detailed description of the pizza */
    std::string getDescription() const;
    
    // Static Helpers
    /** @brief Convert topping enum to string
     *  @param topping Topping to convert
     *  @return String representation of topping */
    static std::string toppingToString(Topping topping);
    /** @brief Convert size enum to string
     *  @param size Size to convert
     *  @return String representation of size */
    static std::string sizeToString(Size size);

private:
    Size size;                    ///< Pizza size
    std::vector<Topping> toppings; ///< List of toppings
};

#endif // PIZZA_HPP
