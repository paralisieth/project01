#include "pizza.hpp"
#include <algorithm>

const std::unordered_map<Pizza::Size, double> Pizza::BASE_PRICES = {
    {Size::Small, 8.99},
    {Size::Medium, 10.99},
    {Size::Large, 13.99}
};

const double Pizza::TOPPING_PRICE = 1.50;

Pizza::Pizza(Size size) : size(size) {
    // Start with cheese as default topping
    addTopping(Topping::Cheese);
}

void Pizza::addTopping(Topping topping) {
    if (!hasTopping(topping)) {
        toppings.push_back(topping);
    }
}

void Pizza::removeTopping(Topping topping) {
    toppings.erase(
        std::remove(toppings.begin(), toppings.end(), topping),
        toppings.end()
    );
}

bool Pizza::hasTopping(Topping topping) const {
    return std::find(toppings.begin(), toppings.end(), topping) != toppings.end();
}

double Pizza::getPrice() const {
    return BASE_PRICES.at(size) + (toppings.size() - 1) * TOPPING_PRICE;
}

std::string Pizza::getDescription() const {
    std::string desc = sizeToString(size) + " Pizza with ";
    for (size_t i = 0; i < toppings.size(); ++i) {
        if (i > 0) {
            desc += (i == toppings.size() - 1) ? " and " : ", ";
        }
        desc += toppingToString(toppings[i]);
    }
    return desc;
}

std::string Pizza::toppingToString(Topping topping) {
    switch (topping) {
        case Topping::Cheese: return "Cheese";
        case Topping::Pepperoni: return "Pepperoni";
        case Topping::Mushrooms: return "Mushrooms";
        case Topping::Onions: return "Onions";
        case Topping::Sausage: return "Sausage";
        case Topping::Bacon: return "Bacon";
        case Topping::GreenPeppers: return "Green Peppers";
        case Topping::Olives: return "Olives";
        default: return "Unknown";
    }
}

std::string Pizza::sizeToString(Size size) {
    switch (size) {
        case Size::Small: return "Small";
        case Size::Medium: return "Medium";
        case Size::Large: return "Large";
        default: return "Unknown";
    }
}
