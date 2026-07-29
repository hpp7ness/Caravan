#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <fstream>
#include <cmath>
#include <cctype>

// Rules: https://fallout.fandom.com/wiki/How_to_play_Caravan

// TODO:
// 1. Break up main function into mini functions
// 2. Bot AI (Easy, forgets cards that get discarded, chooses non-best game state moves)
// 3. Win, lose, tie 
// 4. Preset decks (Bot)
// 5. Placing on enemy's caravans
// 6. Bot AI (Explore other difficulties | Medium remember cards that get discarded, Hard tries to use all information to narrow down possible cards in the player's hand, extreme, hard + best game state moves)
// #? Visual/non-text/non-terminal interface (SFML? OpenGL? SDL?)
// #? Online multiplayer

struct Card {
    int rank;
    std::string suit;

    bool operator==(const Card& other) const {
    return rank == other.rank &&
           suit == other.suit;
    }
};

enum class Direction {
    none,
    up,
    down
};

enum class caravanID {
    left = 0,
    center = 1,
    right = 2,
};

struct Caravan {
    std::vector<Card> cards;
    Direction direction;
    caravanID ID;
    int val;
    std::string name;
};

struct GameState {
    std::vector<Card> player_hand;
    std::vector<Card> bot_hand;

    std::vector<Card> player_deck;
    std::vector<Card> bot_deck;

    std::vector<Card> player_discard;
    std::vector<Card> bot_discard;

    Caravan player_caravan1;
    Caravan player_caravan2;
    Caravan player_caravan3;

    Caravan bot_caravan1;
    Caravan bot_caravan2;
    Caravan bot_caravan3;
};


void bot_actions(Caravan& caravan, const Card& card) {
    // Implement bot actions based on the card and caravan state
}

void save(int caps) {
    std::cout << "Caps: " << caps << std::endl;

    std::ofstream outfile;

    outfile.open("caps.txt");

    if (outfile.is_open()) {
        outfile << caps;
        outfile.close();
    }
}

// does x contain y
bool contains(const std::vector<std::string>& vec, const std::string& target) {
    int cnt = std::count(vec.begin(), vec.end(), target);

    // Check if the target value was found
    if (cnt > 0)
        return true;
    else
        return false;
}

std::vector<int> last_two(const Caravan& caravan) {
    std::vector<int> last_nums;

    for (int i = caravan.cards.size() - 1; i >= 0; --i) {
        if (caravan.cards[i].rank <= 10) {
            last_nums.push_back(i);
            if (last_nums.size() >= 2) {
                break;
            }
        }   
    }

    return last_nums;
}

void direct(Caravan& caravan) {
    if (caravan.cards.size() < 2) {
        caravan.direction = Direction::none;
        return;
    }

    std::vector<int> last = last_two(caravan);

    if (caravan.cards[last[0]].rank > caravan.cards[last[1]].rank) {
        caravan.direction = Direction::down;
    } else {
        caravan.direction = Direction::up;
    }

    int queens = 0;
    for (int i = last[0]; i <= caravan.cards.size() - 1; i++) {
        if (caravan.cards[i].rank == 12) {
            queens += 1;
        }
    }
    for (int i = 1; i <= queens; i++) {
        if (caravan.direction == Direction::up) {
            caravan.direction = Direction::down;
        } else {
               caravan.direction = Direction::up;
        }
    }
}


int value(Caravan& caravan) {
    int total = 0;
    int last = 0;
    int kings = 0;

    int index;
    for (auto& card : caravan.cards) {
        index += 1;
        if (card.rank <= 10) {
            total += card.rank;
            last = card.rank;
        } else if (card.rank == 12 || card.rank == 11) { // Handle Jacks seperetly? Maybe when Jack is played just put the Jack and card it was played on in discard, value is then updated?
            continue;
        } else if (card.rank == 13) {
            for (int i = index; i >= 0; i--)
                if (caravan.cards[i].rank == 13){
                    kings += 1;
                } else {
                    break;
                }    
            total -= std::pow(last, kings-1);
            total += std::pow(last, kings);
        }
    }
    caravan.val = total;
    return total;
}

void disband (Caravan& caravan, std::vector<Card>& discard) {
    for (auto& card : caravan.cards) {
            discard.push_back(card);
        }
        caravan.cards.clear();
}

// refactored for bot / game  state
bool valid(const Card& played, const Caravan& caravan) {
    if (caravan.cards.size() < 2) { 
        if (played.rank > 10) {
            if (caravan.name == "player") {
                std::cout << "Cannot add a face card during a caravan's start-up (first two added cards)!" << std::endl;
                std::cout << " " << std::endl;
            }
            return false;
        }
        return true;
    }

    std::vector<int> last = last_two(caravan);
    // identical rules (Faces unaffected
    // If not face, and have == rank, then invalid
    if (!(caravan.cards.back().rank > 10) && !(played.rank > 10)) { 
        if (caravan.cards[last[0]].rank == played.rank) {
            if (caravan.name == "player") {
                std::cout << "Cards of the same numerical value cannot be played in sequence!" << std::endl;
                std::cout << " " << std::endl;
            }
            return false;
        }
    }
        
    // Same suit validation!
    // Could add suit to caravan struct, and then just check if played.suit == caravan.suit, then return true, else false
    // Would still need rank check, idk if that'd be any better, or would be more efficient
    if (caravan.direction == Direction::up) {
        if (caravan.cards[last[0]].rank < played.rank) { // correct direct, up
            return true;
        } else if (caravan.cards[last[0]].suit == played.suit) { // correct suit
            return true;
        }
    } else {
        if (caravan.cards[last[0]].rank > played.rank) { // correct direct, down
            return true;
        } else if (caravan.cards[last[0]].suit == played.suit) { // correct suit
            return true;
        }
    }
    if (caravan.name == "player") {
        std::cout << "Invalid card played! Card must be of the same suit as the last numbered card, or in the correct direction!" << std::endl;
        std::cout << " " << std::endl;
    }
    return false;
}

void caravan_add (std::vector<Card>& hand, Caravan& caravan, const int& card, std::vector<Card>& discard) {

    std::cout << " " << std::endl;

    Card played = hand[card];

    std::string temp;
    switch (caravan.ID) {
        case caravanID::left:
            temp = "Left";
            break;
        case caravanID::center:
            temp = "Center";
            break;
        case caravanID::right:
            temp = "Right";
            break;
    }

    if (!valid(played, caravan)) { // could take out of function and just place in main
        return;
    }

    caravan.cards.push_back(played);
    hand.erase(hand.begin() + card);
    
    if (caravan.cards.size() >= 2) {
        direct(caravan);
    }

    if (played.rank == 11) { // could put in own function and just call in main
            discard.push_back(played);
            discard.push_back(caravan.cards.back());
            caravan.cards.pop_back();
        }

    std::cout << temp << " caravan: " << std::endl;
    for (const auto& c : caravan.cards) {
        std::cout << "Rank: " << c.rank << ", Suit: " << c.suit << std::endl;
    }

    value(caravan);
    // Overburdon
    if (caravan.val > 26) { // could make own function
        for (auto& c : caravan.cards) {
            discard.push_back(c);
        }
        caravan.cards.clear();
        std::cout << "Caravan became overburdoned! All cards in caravan moved to discard pile!" << std::endl;
        std::cout << " " << std::endl;
    }
    std::cout << temp << " caravan value: " << caravan.val << std::endl;
}

std::string lowercase(std::string str) {
    for (char& c : str) {
        c = std::tolower(static_cast<unsigned char>(c));
    }
    return str;
}

std::string normalize_suit(std::string suit) {
    suit = lowercase(suit);
    
    if (suit == "hearts" || suit == "h") {
        suit = "Hearts";
    } else if (suit == "diamonds" || suit == "d") {
        suit = "Diamonds";
    } else if (suit == "clubs" || suit == "c") {
        suit = "Clubs";
    } else if (suit == "spades" || suit == "s") {
        suit = "Spades";
    }
    return suit;
}

std::string normalize_rank(std::string rank) {
    rank = lowercase(rank);

    if (rank == "ace" || rank == "a") {
        return "1";
    } else if (rank == "jack" || rank == "j") {
        return "11";
    } else if (rank == "queen" || rank == "q") {
        return "12";
    } else if (rank == "king" || rank == "k") {
        return "13";
    }
    return rank;
}

int distinct(std::vector<Card> deck) {
    int dist = 0;
    if (deck.size() > 0) {
        std::vector<Card> copy = deck;
        sort(copy.begin(), copy.end(),     
            [](const Card& a, const Card& b) {
                return a.rank < b.rank;
            }
        );
        // Move all duplicates to last of vector
        auto it = unique(copy.begin(), copy.end());

        // Remove all duplicates
        copy.erase(it, copy.end());
        for (Card card : copy) {
            if (card.rank <= 10 && count_if(deck.begin(), deck.end(),
                [&](const Card& c) {
                    return c.rank == card.rank;
                }) >= 3) {
                dist += 1;
            }
        }
    }
    return dist;
}

int main() {
    // DEBUG
    bool debug = true;
    bool tutorial = true;

    std::string input;
    std::cout << "Welcome to Caravan!" << std::endl;
    std::cout << "Turn on tutorial mode? y/n ";
    while (!(std::cin >> input) && !(lowercase(input) == "y" || lowercase(input) == "n" || lowercase(input) == "yes" || lowercase(input) == "no")) {
        std::cout << "Invalid input. Please enter 'y' or 'n': ";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    if (lowercase(input) == "y" || lowercase(input) == "yes") {
        tutorial = true;
    } else {
        tutorial = false;
    }
    if (tutorial) {
        std::cout << " " << std::endl;
        std::cout << "You have turned on tutorial mode! This will provide you with additional information and guidance throughout the game." << std::endl;
        std::cout << "In tutorial mode, you will also not be penitalized for losing! But you also cant gain from winning..." << std::endl;
        std::cout << " " << std::endl;
        std::cout << "Now, as a quick summary of the game: " << std::endl;
        std::cout << "You want to sell your caravans, which means to get them to a value of 21 to 26, and have more of said sold caravans than your opponent!" << std::endl;
        std::cout << "You can also check out the rules on fallout's Fandom Wiki page: https://fallout.fandom.com/wiki/How_to_play_Caravan" << std::endl;
        std::cout << " " << std::endl;
    }
    int caps = 5;
    std::ifstream infile("caps.txt");

    if (infile.is_open()) {
        infile >> caps;
        infile.close();
    }

    std::cout << "You have " << caps << " caps." << std::endl;
    std::cout << " " << std::endl;

    std::cout << "Please enter your bet (positive integer): ";
    int bet;

    if (tutorial) {
        std::cout << " " << std::endl;
        std::cout << "Please also keep in mind that you can neither bet more than you have, zero, or less than zero." << std::endl;
        std::cout << " " << std::endl;
    }

    bool betting = true;
    while (betting) {
        while (!(std::cin >> bet)) {
            std::cout << "Invalid input. Please enter a positive integer: ";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        betting = false;
    }
    std::cout << "You have betted " << bet << " caps." << std::endl;

    std::vector<Card> player_deck;

    std::vector<std::string> suits = {"Hearts", "Diamonds", "Clubs", "Spades"};
    std::vector<std::string> ranks = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13"};
    
    if (tutorial) {
        std::cout << " " << std::endl;
        std::cout << "You will now build your deck. You will need at least 30 cards in your deck to play, and at least 6 non-face cards split between 2 different values (eg: three 8s and three 6s)!" << std::endl;
        std::cout << " " << std::endl;
        std::cout << "Number cards are just numbers, with aces worth one. HOWEVER, face cards have unique abilities." << std::endl;
        std::cout << " " << std::endl;
        std::cout << " 1. Kings double the value of the card they are placed on. This effect is multiplicative!" << std::endl;
        std::cout << " 2. Jacks will remove the card they are played on." << std::endl;
        std::cout << " 3. Queens will reverse the direction of the caravan they are played on." << std::endl;
        std::cout << " " << std::endl;
        std::cout << "Btw, if you're unsure about how to build a deck, just say yes when prompted about presets, and choose on of the 5 preset decks!" << std::endl;
        std::cout << " " << std::endl;
    }


    // Random seed!!
    std::random_device rd;
    std::mt19937 rng(rd());

    int dex = 0;
    // player_deck building
    while (true) {
        std::cout << " " << std::endl;
        int dist = distinct(player_deck);
        if (dex == 0) {
            bool preset = false;
            std::cout << "Would you like to use a preset deck? y/n ";
            std::string input;
            while (!(std::cin >> input) || !(lowercase(input) == "y" || lowercase(input) == "n" || lowercase(input) == "yes" || lowercase(input) == "no")) {
                std::cout << "Invalid input. Please enter 'y' or 'n': ";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
            if (lowercase(input) == "y" || lowercase(input) == "yes") {
                preset = true;
                if (tutorial) {
                    std::cout << " " << std::endl;
                    std::cout << "Presets, in order, are: " << std::endl;
                    std::cout << " " << std::endl;
                    std::cout << "1. Basic" << std::endl;
                    std::cout << "2. Meta" << std::endl;
                    std::cout << "3. Aggressive" << std::endl;
                    std::cout << "4. Passive" << std::endl;
                    std::cout << "5. Random" << std::endl;
                    std::cout << " " << std::endl;
                }
                std::cout << "Choose a preset deck (1-5): ";
                std::vector<std::string> presets = {"1", "2", "3", "4", "5", "basic", "meta", "aggressive", "passive", "random", "b", "m", "a", "p", "r"};
                std::string choice;
                while (!(std::cin >> choice) || !(contains(presets, lowercase(choice)))) {
                    std::cout << "Invalid choice. Please enter a number between 1 and 5: ";
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                }
                choice = lowercase(choice);
                int c;
                try {
                    c = stoi(choice);
                } catch (std::invalid_argument) {
                    if (choice == "basic" || choice == "b") {
                        c = 1;
                    } else if (choice == "meta" || choice == "m") {
                        c = 2;
                    } else if (choice == "aggressive" || choice == "a") {
                        c = 3;
                    } else if (choice == "passive" || choice == "p") {
                        c = 4;
                    } else if (choice == "random" || choice == "r") {
                        c = 5;
                    }
                }
 

                // Initialize the player_deck with the chosen preset
                switch (c) {
                    case 1:
                        // Basic preset
                        for (std::string suit : suits) {
                            for (int rank = 1; rank <= 13; rank++) {
                                player_deck.push_back({rank, suit});
                            }
                        }
                        break;
                    case 2:
                        // Meta preset
                        while (player_deck.size() < 30) {
                            player_deck.push_back({10, suits[0]}); // 10 of hearts
                            player_deck.push_back({9, suits[0]}); // 9 of hearts
                            player_deck.push_back({7, suits[0]}); // 7 of hearts
                        }                        
                        break;
                    case 3:
                        // Aggressive preset
                        // Meta deck with face cards
                        while (player_deck.size() < 30) {
                            player_deck.push_back({10, suits[0]});
                            player_deck.push_back({9, suits[0]});
                            player_deck.push_back({7, suits[0]});
                        } 
                        for (int i = 0; i < 6; i++) {
                            player_deck.push_back({11, suits[0]});
                            player_deck.push_back({12, suits[0]});
                            player_deck.push_back({13, suits[0]});
                        }
                        break;
                    case 4:
                        // Passive preset
                        // Basic preset without face cards
                        for (std::string suit : suits) {
                            for (int rank = 1; rank <= 10; rank++) {
                                player_deck.push_back({rank, suit});
                            }
                        }                        
                        break;
                    case 5:
                        // Random preset
                        while (player_deck.size() < 30) {
                            std::uniform_int_distribution<int> rank_dist(1, 13);
                            std::uniform_int_distribution<int> suit_dist(0, suits.size() - 1);

                            player_deck.push_back({rank_dist(rng), suits[suit_dist(rng)]});
                        }
                        int non_faces = 0;
                        non_faces = count_if(player_deck.begin(), player_deck.end(),
                            [&](const Card& c) {
                                return c.rank <= 10;
                            });
                        while (distinct(player_deck) < 2 || non_faces < 6) {
                            player_deck.push_back({rand() % 10 + 1, suits[rand() % suits.size()]});
                            non_faces = count_if(player_deck.begin(), player_deck.end(),
                                [&](const Card& c) {
                                    return c.rank <= 10;
                                });
                        }
                        break;
                }
                break; // Exit the loop after selecting a preset
            }
            if (!preset) {
                std::cout << "Enter 'Confirm' during suit or rank selection to finish building your deck." << std::endl;
                std::cout << " " << std::endl;
            }
        }
        dex += 1;
        std::string want_suit;
        std::string want_rank;
        int amount;

        std::cout << "Enter the suit you want (Hearts, Diamonds, Clubs, Spades): ";
        while (!(std::cin >> want_suit) || contains(suits, normalize_suit(want_suit)) == false && lowercase(want_suit) != "confirm") {
            std::cout << "Invalid suit. Ensure the first letter is captitalized! Please enter a valid suit (Hearts, Diamonds, Clubs, Spades): ";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        want_suit = normalize_suit(want_suit);
       
        if (want_suit != "confirm") {
            std::cout << "Enter the rank you want (aces, twos, threes, etc.): ";
            while (!(std::cin >> want_rank) || contains(ranks, normalize_rank(want_rank)) == false && lowercase(want_rank) != "confirm") {
                std::cout << "Invalid rank. Please enter a valid rank (1-13): ";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        }
        want_rank = normalize_rank(want_rank);

        if (want_rank == "confirm" || want_suit == "confirm") {
            std::cout << " " << std::endl;
            std::cout << " " << std::endl;
            if (dist < 2) {
                std::cout << "You need at least 6 non-face cards split between 2 different values (eg: three 8s and three 6s) in your player_deck to play! " << std::endl;
            }
            if (player_deck.size() < 30) {
                std::cout << "You need at least 30 cards in your deck to play! You currently have: " << player_deck.size() << std::endl;
            }
            if (!(dist < 2)) {
                if (!(player_deck.size() < 30)) {
                    break;
                }
            }
        }

        if (want_rank != "confirm" && want_suit != "confirm") {
            std::cout << "Enter the amount of cards you want for rank " << want_rank << " of suit " << want_suit << ": ";
            while (!(std::cin >> amount) || amount < 1) {
                std::cout << "Invalid input. Please enter a positive integer: ";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        }

        for (int i = 0; i < amount; ++i) {
            try {
            player_deck.push_back({stoi(want_rank), want_suit});
            } catch (std::invalid_argument) {
                std::cout << "STOI ERROR AT LINE 244! This is for debugging purposes only. If found please report to developer." << std::endl;
                std::string temp;
                std::cin >> temp;
                if (temp.empty()) {
                    return 0;
                }
            }
        }
        std::cout << " " << std::endl;
    }

    // Shuffle the player_deck
    std::shuffle(player_deck.begin(), player_deck.end(), rng);

    // Display the shuffled player_deck
    // DEBUG
    if (debug) {
        std::cout << "Shuffled player_deck:" << std::endl;
        for (const auto& card : player_deck) {
            std::cout << "Rank: " << card.rank << ", Suit: " << card.suit << std::endl;
        }
    }


    // Deals to player
    std::vector<Card> player_hand;
    for (const auto& card : player_deck) {
        if (player_hand.size() < 8) {
            player_hand.push_back(card);
            player_deck.pop_back(); 
        } else {
            break;
        }
    }


    // Bot deck 
    // Increased difficulties == better decks!
    // Add deck types, like Aggressive, Passive, Meta, Random, etc. Could mix with difficulty levels, or just have difficulty levels be the deck types. 
    std::vector<Card> bot_deck;
    for (std::string suit : suits) {
        for (int rank = 1; rank <= 13; rank++) {
            bot_deck.push_back({rank, suit});
        }
    }
    // Shuffles bot deck
    std::shuffle(bot_deck.begin(), bot_deck.end(), rng);

    if (debug) {
        std::cout << "Shuffled bot_deck:" << std::endl;
        for (const auto& card : bot_deck) {
            std::cout << "Rank: " << card.rank << ", Suit: " << card.suit << std::endl;
        }
    }

    // Deals to bot
    std::vector<Card> bot_hand;
    for (const auto& card : bot_deck) {
        if (bot_hand.size() < 8) {
            bot_hand.push_back(card);
            bot_deck.pop_back();
        } else {
            break;
        }
    }


    // make this neater somehow?
    Caravan player_caravan1;
    Caravan player_caravan2;
    Caravan player_caravan3;

    player_caravan1.ID = caravanID::left;
    player_caravan1.name = "player";
    player_caravan2.ID = caravanID::center;
    player_caravan2.name = "player";
    player_caravan3.ID = caravanID::right;
    player_caravan3.name = "player";

    Caravan bot_caravan1;
    Caravan bot_caravan2;
    Caravan bot_caravan3;
    
    bot_caravan1.ID = caravanID::left;
    bot_caravan1.name = "bot";
    bot_caravan2.ID = caravanID::center;
    bot_caravan2.name = "bot";
    bot_caravan3.ID = caravanID::right;
    bot_caravan3.name = "bot";

    std::vector<Card> player_discard;
    std::vector<Card> bot_discard;
    if (tutorial) {
        std::cout << " " << std::endl;
        std::cout << "One more thing. Ive told about sold caravans, but caravans also have two other properties/states." << std::endl;
        std::cout << " " << std::endl;
        std::cout << "These states are 'overburdened' and 'active'. Overburdened caravans are ones that have gone past the max sold value of 26." << std::endl;
        std::cout << "Active caravans are ones that are still being built, and have not yet been sold or overburdened." << std::endl;
        std::cout << "Note, sold caravans can still be active, but not overburdened ones. You can, however, still disband them to start over." << std::endl;
        std::cout << " " << std::endl;
        std::cout << "As for properties, there are value and direction." << std::endl;
        std::cout << " " << std::endl;
        std::cout << "Value is the total value of the caravan, and direction is whether the caravan is going up or down in value." << std::endl;
        std::cout << "Direction is determined by the last two non-face cards in the caravan, and can be reversed by playing a queen." << std::endl;
        std::cout << "Note, direction can be ignored by using the same suit as the last numbered card." << std::endl;
        std::cout << " " << std::endl;
    }

    // Main game loop
    bool playing = true;
    while (playing) {    
        std::cout << " " << std::endl;  
        // Display player hand
        std::cout << "\nPlayer Hand:" << std::endl;
        for (const auto& card : player_hand) {
            std::cout << "Rank: " << card.rank << ", Suit: " << card.suit << std::endl;
        }
        
        std::cout << "Add, discard, or disband: " << std::endl;
        std::string action;
        while (!(std::cin >> action) &&  !(action == "Add") || !(action == "Discard") || !(action == "Disband") ) {
            std::cout << "Invalid input. Ensure the first letter is captitalized! Please pick 'Add', 'Discard', 'Disband': ";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');            
        }

        // ADD DISCARD 
        int want_card;
        if (action == "Discard" || action == "Add") {
            std::cout << "Please select a card (card 1, 2, etc...): ";
            
            while (!(std::cin >> want_card) || !(want_card > 0 && want_card <= 8)) {
                std::cout << "Invalid input. Please use a positive interger betweemn 1 and 8: ";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
            want_card -= 1;
            if (action == "Discard") {
                player_discard.push_back(player_hand[want_card]);
                player_hand.erase(player_hand.begin() + want_card);
            }
        }



        
        // ADD DISPAND
        int input;
        if (action == "Add" || action == "Disband") {
            if (action == "Add") {
                std::cout << "Please select a caravan to add " << player_hand[want_card].rank << " of " << player_hand[want_card].suit << " to. (caravan 1, 2, 3, etc...): ";
            } else {
                std::cout << "Please select a caravan to add disband: ";
            }
            
            
            while (!(std::cin >> input) || !(input > 0 && input <= 3)) {
                std::cout << "Invalid input. Please use a positive interger betweemn 1 and 3: ";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
            input -= 1;
            caravanID want_caravan = static_cast<caravanID>(input);
            // ADD OPPONENT PLACING

            switch (want_caravan) {
                case caravanID::left:
                    // Split disband from action, remove action from functions
                    if (action == "Add") {
                        caravan_add(player_hand, player_caravan1, want_card, player_discard);
                    
                    } else if (action == "Disband") {
                        disband(player_caravan1, player_discard);
                    }
                    break;
                case caravanID::center:
                    if (action == "Add") {
                        caravan_add(player_hand, player_caravan2, want_card, player_discard);
                    
                    } else if (action == "Disband") {
                        disband(player_caravan2, player_discard);
                    }
                    break;
                case caravanID::right:
                    if (action == "Add") {
                        caravan_add(player_hand, player_caravan3, want_card, player_discard);
                    
                    } else if (action == "Disband") {
                        disband(player_caravan3, player_discard);
                    }
                    break;     
            }                
        }

        if (player_deck.size() == 0) {
            for (auto& card : player_discard) {
                player_deck.push_back(card);
            }
            player_discard.clear();
        }
        // Player turn needs own loop so they can be sent back up in case of wrong placement (Ex: doesn't follow direction, identical number cards, empty face)
        player_hand.push_back(player_deck.back());
        player_deck.pop_back();

        // End of player turn, bot turn starts

        

    }
    
    // Make sure every thing else is taken out of main, and game is placed in own game function!
    bool running = true;
    while (running) {
        // Game function goes here

        std::cout << " "  << std::endl;
        std::string quit = "N/A"; 
        while (quit != "N" && quit != "Y" && quit != "n" && quit != "y") { // Closes app based on user input
            std::cout << "Continue? y/n " << std::endl;
            std::cin >> quit;

            if (quit == "Y" || quit == "y" || quit == "Yes" || quit == "yes") {
                break;
            } else if (quit == "N" || quit == "n" || quit == "No" || quit == "no") {
                return 0;
            } else {
                std::cout << "Invalid input!" << std::endl;
            }
        }
    }
    return 0;
}