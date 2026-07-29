# Caravan

A C++ recreation of **Caravan**, the card game from *Fallout: New Vegas*. The project is currently terminal-based and aims to faithfully implement the original game's mechanics while remaining modular enough to support future AI opponents and graphical interfaces.

## Features

* Terminal-based gameplay
* Interactive tutorial mode for new players
* Custom deck builder
* Multiple preset deck options
* Validation of Caravan rules
* Card effects:

  * **King** – Doubles the value of the attached card (stackable)
  * **Queen** – Reverses caravan direction
  * **Jack** – Removes the targeted card
* Caravan value and direction tracking
* Persistent caps system between sessions
* Randomized shuffling and card dealing

## Planned Features

* AI using Information Set Monte Carlo Tree Search (ISMCTS)
* Multiple AI difficulty levels
* Win/loss conditions and complete match flow
* Opponent caravan interactions
* Additional preset decks
* Graphical interface (SDL3 planned)
* Online multiplayer
* Improved code organization and refactoring

## Building

Compile using any C++17-compatible compiler.

Example using **g++**:

```bash
g++ Caravan.cpp -std=c++17 -O2 -o Caravan
```

Run the executable:

```bash
./Caravan
```

On Windows:

```cmd
Caravan.exe
```

## Project Status

This project is currently under active development. Core gameplay systems are being implemented first, followed by AI, code refactoring, and eventually a graphical interface.

## Inspiration

This project is inspired by **Caravan**, the in-game card game featured in *Fallout: New Vegas*. The goal is to recreate the experience as faithfully as possible while serving as a learning project for C++, game architecture, and AI development.

_Project is solely made and maintained by me_

_AI was only used in creation of README file_
