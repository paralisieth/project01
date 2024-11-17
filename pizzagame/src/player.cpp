#include "player.hpp"

Player::Player() : money(100.0), reputation(0) {}

double Player::getMoney() const {
    return money;
}

void Player::addMoney(double amount) {
    money += amount;
}

void Player::subtractMoney(double amount) {
    money -= amount;
}

int Player::getReputation() const {
    return reputation;
}

void Player::addReputation(int amount) {
    reputation += amount;
}
