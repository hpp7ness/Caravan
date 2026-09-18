#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <fstream>
#include <cmath>
#include <cctype>
#include <string>

/*
Rules: https://fallout.fandom.com/wiki/How_to_play_Caravan

TODO:
    1. Improve repeated code (ex: code that only differs by player_caravan1, player_caravan2, player_caravan3)
    2. Break up caravan_add's responsibilities/refactor caravan_add.
    3. Add way to go through add, discard, and disband with nums
    4. Refactor string cin input loops (use botton quit loop as reference!)
    4. Bot AI (Easy, forgets cards that get discarded, chooses non-best game state moves)
    5. Preset decks (Bot)
    6. Keep heuristic ai as baby or tutorial mode?
    7. Add automated rule tests (test to make sure kings and faces, status, direct, etc works properly) !!! Move up? !!!
    8. Placing on enemy's caravans
    9. Bot AI (Explore other difficulties | Medium remember cards that get discarded, Hard tries to use all information to narrow down possible cards in the player's hand, extreme, hard + best game state moves)
    #? Make discard function? (would take a lot of going through old functions and replacing old scripts), optional
    #? Visual/non-text/non-terminal interface (SFML? OpenGL? SDL?)
    #? Online multiplayer 

    graphics refactor advice: https://chatgpt.com/share/6a6ecac2-55c4-83ea-9b07-ea8d90c2ade1
*/

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
    std::string status;
    std::string name;
};

struct GameState {
    std::vector<Card> player_deck;
    std::vector<Card> bot_deck;

    std::vector<Card> player_hand;
    std::vector<Card> bot_hand;

    std::vector<Card> player_discard;
    std::vector<Card> bot_discard;

    std::vector<Caravan> player_caravans;

    std::vector<Caravan> bot_caravans;

    int caps;
    int bet;
};

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

    int index = 0;
    for (auto& card : caravan.cards) {
        index += 1;
        if (card.rank <= 10) {
            total += card.rank;
            last = card.rank;
            kings = 0;
        } else if (card.rank == 12 || card.rank == 11) { // Handle Jacks seperetly? Maybe when Jack is played just put the Jack and card it was played on in discard, value is then updated?
            continue;
        } else if (card.rank == 13) {
            for (int i = index; i >= 0; i--)
                if (caravan.cards[i].rank == 13){
                    kings += 1;
                } else {
                    break;
                }    
            total -= last * std::pow(2, kings-1);
            total += last * std::pow(2, kings);
        }
    }
    caravan.val = total;
    return total;
}

void disband(Caravan& caravan, std::vector<Card>& discard) {
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

bool caravan_add (std::vector<Card>& hand, Caravan& caravan, const int& card, std::vector<Card>& discard) {

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
        return false;
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

    if (caravan.name == "player") {
        std::cout << temp << " caravan: " << std::endl;
        for (const auto& c : caravan.cards) {
            std::cout << "Rank: " << c.rank << ", Suit: " << c.suit << std::endl;
        }
    }
    

    value(caravan);
    // Overburdon
    if (caravan.val > 26) { // could make own function
        for (auto& c : caravan.cards) {
            discard.push_back(c);
        }
        caravan.cards.clear();
        if (caravan.name == "player") {
            std::cout << "Caravan became overburdoned! All cards in caravan moved to discard pile!" << std::endl;
            std::cout << " " << std::endl;
        }
    }
    if (caravan.name == "player") {
        std::cout << temp << " caravan value: " << caravan.val << std::endl;
    }

    return true;
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

void status_update(std::vector<Caravan>& pCaravans, std::vector<Caravan>& bCaravans) {
    std::vector<int> pVals;
    for (auto caravan : pCaravans) {
        pVals.push_back(caravan.val);
    }
    std::vector<int> bVals;
    for (auto caravan : bCaravans) {
        bVals.push_back(caravan.val);
    }
    
    int dex = 0;
    for (int val : pVals) {
        if (val > 26)  {
            pCaravans[dex].status = "overburnden";
        } else if ((val >= 21) && (val <= 26)) {
            pCaravans[dex].status = "sold";
        } else {
            pCaravans[dex].status = "active";
        }
        dex++;
    }

    dex = 0;
    for (int val : bVals) {
        if (val > 26)  {
            bCaravans[dex].status = "overburnden";
        } else if ((val >= 21) && (val <= 26)) {
            bCaravans[dex].status = "sold";
        } else {
            bCaravans[dex].status = "active";
        }
        dex++;
    }

    for (int i; i < 3; i++) {
        if (pCaravans[i].status == "sold") {
            if (pCaravans[i].status == bCaravans[i].status) {
                pCaravans[i].status = "tie";
                bCaravans[i].status = "tie";
            }
        }
    }
}

void display(const std::vector<Card>& item, const std::string& tag) {
    std::cout << tag << ":" << std::endl;
    for (const auto& card : item) {
         std::cout << "Rank: " << card.rank << ", Suit: " << card.suit << std::endl;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Globals
std::random_device rd; // Random seed!!
std::mt19937 rng(rd());

bool debug = true;
bool tutorial = true;

GameState cState = {
    {}, // player_deck
    {}, // bot_deck
    {}, // player_hand
    {}, // bot_hand
    {}, // player_discard
    {}, // bot_discard
    {
        { {}, Direction::none, caravanID::left, 0, "active", "player" },
        { {}, Direction::none, caravanID::center, 0, "active", "player" },
        { {}, Direction::none, caravanID::right, 0, "active", "player" }
    }, // player_caravans
    {
        { {}, Direction::none, caravanID::left, 0, "active", "bot" },
        { {}, Direction::none, caravanID::center, 0, "active", "bot" },
        { {}, Direction::none, caravanID::right, 0, "active", "bot" }
    }, // bot_caravans
    0, // caps
    0  // bet
};

std::vector<std::string> suits = {"Hearts", "Diamonds", "Clubs", "Spades"};
std::vector<std::string> ranks = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13"};
// Globals

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Old main function broken down
void setupGameState() {

    for (auto caravan : cState.player_caravans) {
        caravan.val = value(caravan);
    }
    for (auto caravan : cState.bot_caravans) {
        caravan.val = value(caravan);
    }

    status_update(cState.player_caravans, cState.bot_caravans);
}

void intro() {

    std::string input;
    std::cout << "Welcome to Caravan!" << std::endl;
    std::cout << "Turn on tutorial mode? y/n ";

    // Improve
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
}

void loadCaps() {
    std::ifstream infile("caps.txt");

    if (infile.is_open()) {
        infile >> cState.caps;
        infile.close();
    }
    if (debug) {
        std::cout << "Loaded caps: " << cState.caps << std::endl;
    } 

    if (cState.caps <= 0) {
        cState.caps = 5;
    }

    setupGameState();
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

void startBet() {
    std::cout << "Please enter your bet (positive integer): ";

    if (tutorial) {
        std::cout << " " << std::endl;
        std::cout << "Please also keep in mind that you can't bet more than you have, zero, or less than zero." << std::endl;
        std::cout << " " << std::endl;
    }

    bool betting = true;
    while (betting) {
        while (!(std::cin >> cState.bet) || cState.bet > cState.caps || cState.bet < 0 || (!(tutorial) && cState.bet == 0)) {
            std::cout << "Invalid input. Please enter a positive integer: ";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        betting = false;
    }
    std::cout << "You have betted " << cState.bet << " caps." << std::endl;

    setupGameState();
}

void buildDeck() {
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

    int dex = 0;
    // player_deck building
    while (true) {
        std::cout << " " << std::endl;
        int dist = distinct(cState.player_deck);
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
                                cState.player_deck.push_back({rank, suit});
                            }
                        }
                        break;
                    case 2:
                        // Meta preset
                        while (cState.player_deck.size() < 30) {
                            cState.player_deck.push_back({10, suits[0]}); // 10 of hearts
                            cState.player_deck.push_back({9, suits[0]}); // 9 of hearts
                            cState.player_deck.push_back({7, suits[0]}); // 7 of hearts
                        }                        
                        break;
                    case 3:
                        // Aggressive preset
                        // Meta deck with face cards
                        while (cState.player_deck.size() < 30) {
                            cState.player_deck.push_back({10, suits[0]});
                            cState.player_deck.push_back({9, suits[0]});
                            cState.player_deck.push_back({7, suits[0]});
                        } 
                        for (int i = 0; i < 6; i++) {
                            cState.player_deck.push_back({11, suits[0]});
                            cState.player_deck.push_back({12, suits[0]});
                            cState.player_deck.push_back({13, suits[0]});
                        }
                        break;
                    case 4:
                        // Passive preset
                        // Basic preset without face cards
                        for (std::string suit : suits) {
                            for (int rank = 1; rank <= 10; rank++) {
                                cState.player_deck.push_back({rank, suit});
                            }
                        }                        
                        break;
                    case 5:
                        // Random preset
                        while (cState.player_deck.size() < 30) {
                            std::uniform_int_distribution<int> rank_dist(1, 13);
                            std::uniform_int_distribution<int> suit_dist(0, suits.size() - 1);

                            cState.player_deck.push_back({rank_dist(rng), suits[suit_dist(rng)]});
                        }
                        int non_faces = 0;
                        non_faces = count_if(cState.player_deck.begin(), cState.player_deck.end(),
                            [&](const Card& c) {
                                return c.rank <= 10;
                            });
                        while (distinct(cState.player_deck) < 2 || non_faces < 6) {
                            cState.player_deck.push_back({rand() % 10 + 1, suits[rand() % suits.size()]});
                            non_faces = count_if(cState.player_deck.begin(), cState.player_deck.end(),
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
        int amount = 0;

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
            if (cState.player_deck.size() < 30) {
                std::cout << "You need at least 30 cards in your deck to play! You currently have: " << cState.player_deck.size() << std::endl;
            }
            if (!(dist < 2)) {
                if (!(cState.player_deck.size() < 30)) {
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
            cState.player_deck.push_back({stoi(want_rank), want_suit});
        std::cout << " " << std::endl;
        }
    }

    setupGameState();
}

void botDeck() {
    // Bot deck 
    // Increased difficulties == better decks!
    // Add deck types, like Aggressive, Passive, Meta, Random, etc. Could mix with difficulty levels, or just have difficulty levels be the deck types.
    for (std::string suit : suits) {
        for (int rank = 1; rank <= 13; rank++) {
            cState.bot_deck.push_back({rank, suit});
        }
    }
}

void debugShuffle() {
    // Display the shuffled player_deck
    // DEBUG
    if (debug) {
        display(cState.player_deck, "Shuffled player_deck");
        display(cState.bot_deck, "Shuffled bot_deck");
    }
}

void deal() {
    // Deals to player
    for (const auto& card : cState.player_deck) {
        if (cState.player_hand.size() < 8) {
            cState.player_hand.push_back(card);
            cState.player_deck.pop_back(); 
        } else {
            break;
        }
    }
    
    // Deals to bot
    for (const auto& card : cState.bot_deck) {
        if (cState.bot_hand.size() < 8) {
            cState.bot_hand.push_back(card);
            cState.bot_deck.pop_back();
        } else {
            break;
        }
    }

    setupGameState();
}

bool playerAction() {
    bool val = false;

    std::cout << " " << std::endl;  
    // Display player hand
    display(cState.player_hand, "\nPlayer Hand");
    
    std::cout << "Add, discard, or disband: " << std::endl;
    std::string action;
    while (!(std::cin >> action) ||  !(lowercase(action) == "add") && !(lowercase(action) == "discard") && !(lowercase(action) == "disband") ) {
        std::cout << "Invalid input. Ensure the first letter is captitalized! Please pick 'Add', 'Discard', 'Disband': ";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');            
    }

    action = lowercase(action);

    int want_card;
    if (action == "discard" || action == "add") {
        std::cout << "Please select a card (card 1, 2, etc...): ";
        
        while (!(std::cin >> want_card) || !(want_card > 0 && want_card <= 8)) {
            std::cout << "Invalid input. Please use a positive interger betweemn 1 and 8: ";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        want_card -= 1;
        if (action == "discard") {
            cState.player_discard.push_back(cState.player_hand[want_card]);
            cState.player_hand.erase(cState.player_hand.begin() + want_card);
        }
    }

    int input;
    if (action == "add" || action == "disband") {
        if (action == "add") {
            std::cout << "Please select a caravan to add " << cState.player_hand[want_card].rank << " of " << cState.player_hand[want_card].suit << " to. (caravan 1, 2, 3, etc...): ";
        } else {
            std::cout << "Please select a caravan to add disband: ";
        }
        
        
        while (!(std::cin >> input) || !(input > 0 && input <= 3)) {
            std::cout << "Invalid input. Please use a positive interger betweemn 1 and 3: ";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        input -= 1;
        int want_caravan = input;
        // ADD OPPONENT PLACING

        std::cout << "Want_caravan: " << want_caravan << std::endl;
        std::cout << "cState.player_caravans length: " << cState.player_caravans.size() << std::endl;
        // REFACTOR
        if (action == "add") {
            if (caravan_add(cState.player_hand, cState.player_caravans[want_caravan], want_card, cState.player_discard)) {
                val = true;
            }
        } else if (action == "disband") {
            disband(cState.player_caravans[want_caravan], cState.player_discard);
            val = true;
        }
    }

    if (action == "disband" || action == "discard") { 
        std::cout << " " << '\n';
        std::cout << " " << '\n';
        display(cState.player_discard, "Discard pile \n");
    }

    if (cState.player_deck.size() == 0) {
        for (auto& card : cState.player_discard) {
            cState.player_deck.push_back(card);
        }
        cState.player_discard.clear();
        display(cState.player_deck, "Your discard pile has been move to your deck!");       
    }

    // Player turn needs own loop so they can be sent back up in case of wrong placement (Ex: doesn't follow direction, identical number cards, empty face)
    cState.player_hand.push_back(cState.player_deck.back());
    cState.player_deck.pop_back();
    
    return val;
}

void botAction() {

    bool played = false;

    for (Caravan& caravan : cState.bot_caravans) {
        for (size_t i = 0; i < cState.bot_hand.size(); i++) {
            if (valid(cState.bot_hand[i], caravan)) {
                caravan_add(cState.bot_hand, caravan, static_cast<int>(i), cState.bot_discard);
                played = true;
                break;
            }
        }
        if (played) {
            break; // one card per bot turn, same as the player
        }
    }

    if (!played && !cState.bot_hand.empty()) {
        cState.bot_discard.push_back(cState.bot_hand[0]);
        cState.bot_hand.erase(cState.bot_hand.begin());
    }

    setupGameState();
}

std::string winCheck() {
    std::vector<int> sold_amount = {0,0};

    status_update(cState.player_caravans, cState.bot_caravans);
    setupGameState();

    for (auto caravan : cState.player_caravans) {
        if (caravan.status == "sold") {
            sold_amount[0]++;
        }
    }
    for (auto caravan : cState.bot_caravans) {
        if (caravan.status == "sold") {
            sold_amount[1]++;
        }
    }

    if (sold_amount[0] > 2) {
        return "player wins";
    } else if (sold_amount[1] > 2) {
        return "bot wins";
    }

    return "neither";
}
// Old main function broken down


// Game function for better looping and saves/loads
int gameFunction() {
    intro();
    
    loadCaps();

    std::cout << "You have " << cState.caps << " caps." << std::endl;
    std::cout << " " << std::endl;

    startBet();

    buildDeck();

    // Shuffle the player_deck
    std::shuffle(cState.player_deck.begin(), cState.player_deck.end(), rng);

    display(cState.player_deck, "Your deck");

    botDeck();
    
    // Shuffles bot deck
    std::shuffle(cState.bot_deck.begin(), cState.bot_deck.end(), rng);

    deal();


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
        while (!(playerAction())) {
        }
        
        botAction();
        std::cout << "Bot caravans: " << std::endl;
        int dex = 0;
        std::string temp =  "na";
        for (const auto& caravan : cState.bot_caravans) {
            if (dex == 0) { 
                temp = "Left";
            } else if (dex == 1) {
                temp = "Center";
            } else {
                temp = "Right";
            }

            std::cout << temp << " caravan: " << std::endl;
            for (const auto& c : caravan.cards) {
                std::cout << "Rank: " << c.rank << ", Suit: " << c.suit << std::endl;
            }
            std::cout << temp << " caravan value: " << caravan.val << std::endl;

            std::cout << " " << std::endl;
            
            dex++;
        }

        std::string condition = winCheck();
        if (condition != "neither") {
            if (condition == "player wins") {
                cState.caps += cState.bet;

                std::cout << "You win!" << '\n';

                playing = false;
                break;
            } else {
                cState.caps -= cState.bet;

                std::cout << "Bot wins!" << '\n';

                playing = false;
                break;
            }
        }
    }

    return cState.caps;
}

int main() {
    // Make sure every thing else is taken out of main, and game is placed in own game function!
    bool running = true;
    while (running) {
        save(gameFunction());
        // Game function goes here

        std::cout << " "  << std::endl;
        std::string quit = "N/A";
        while (quit != "n" || quit != "y" || quit == "yes" || quit == "no") { // Closes app based on user input
            std::cout << "Continue? y/n " << std::endl;
            std::cin >> quit;
            quit = lowercase(quit);

            if (quit == "y" || quit == "yes") {
                break;
            } else if (quit == "n" || quit == "no") {
                return 0;
            } else if (!(quit == "N/A")) {
                std::cout << "Invalid input!" << std::endl;
            }            
        }
    }
    return 0;
}
