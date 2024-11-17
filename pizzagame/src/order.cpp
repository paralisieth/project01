#include "order.hpp"

Order::Order(int orderNumber) 
    : orderNumber(orderNumber), completed(false) {}

void Order::addPizza(const Pizza& pizza) {
    pizzas.push_back(pizza);
}

double Order::getTotalPrice() const {
    double total = 0.0;
    for (const auto& pizza : pizzas) {
        total += pizza.getPrice();
    }
    return total;
}

bool Order::isComplete() const {
    return completed;
}

void Order::complete() {
    completed = true;
}

int Order::getOrderNumber() const {
    return orderNumber;
}

const std::vector<Pizza>& Order::getPizzas() const {
    return pizzas;
}

std::string Order::getStatus() const {
    return completed ? "Completed" : "Pending";
}
