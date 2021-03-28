/*
@author: Anurag Kumar
@brief desc:
This is the program for
CS4328 Programming project 1 for the Spring 2021
semester at Texas State University
It models a game called Pair War where the objective of the
game is to get pairs of cards
*/

#include <iostream>
#include <algorithm>
#include <vector>
#include <random>
#include <string>
#include <pthread.h>
#include <fstream>
#include <assert.h>

#define NUMBEROFPLAYERS 3

int randomSeed;
int currentRound = 1, currentTurn = 0;
bool hasRoundBeenWon = false, didThisThreadWin = false;

std::vector<int> deckOfCards;
std::vector<int> p1hand, p2hand, p3hand;
std::uniform_int_distribution<int> distribution(0, 1);
std::default_random_engine generator;
std::string consoleOutput[4];

pthread_t dealer_ID, p1_ID, p2_ID, p3_ID;
pthread_mutex_t mutexDeck_or_Hand = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

//thread functions
void* dealer(void* param);
void* player1(void* param);
void* player2(void* param);
void* player3(void* param);

//utlity functions to be shared between threads
void shuffleDeck();
void showDeck();
void takeACard(std::vector<int>& hand);
void putCardAtBottom(std::vector<int>& hand);
void dealCards();
void addToLogFile(std::string appendable);
void exitRound(std::vector<int>& hand);
bool isWinningHand(std::vector<int>& hand);
std::string genDeckString();

/// <summary>
/// generates a string representation of the deck
/// </summary>
/// <returns>
/// returns the string representation of the deck
///</returns>
std::string genDeckString() {
	std::string retVal;
	retVal.append("deckOfCards contains:");
	for (std::vector<int>::iterator it = deckOfCards.begin(); it != deckOfCards.end(); ++it) {
		retVal.append(" ");
		retVal.append(std::to_string(*it));
	}
	retVal.append("\n");

	return retVal;
}

/// <summary>
/// adds the passed string to a log file
/// </summary>
void addToLogFile(std::string appendable) {
	std::ofstream myfile;
	myfile.open("CS4328_AnuragKumar_2021_Program1.log", std::ios::app);
	myfile << appendable;
	myfile.close();
}

/// <summary>
/// logs the hand passed to the function
/// </summary>
/// <param name="hand"></param>
void logHand(std::vector<int>& hand) {
	addToLogFile("hand contains: ");
	for (std::vector<int>::iterator it = hand.begin(); it != hand.end(); ++it) {
		addToLogFile(" ");
		addToLogFile(std::to_string(*it));
	}
	addToLogFile("\n");
}

/// <summary>
/// randomizes the order of cards in the deck
/// </summary>
void shuffleDeck() {
	addToLogFile("Deck Shuffled \n");
	std::shuffle(deckOfCards.begin(), deckOfCards.end(), generator);
}

/// <summary>
/// dumps the contents of the deck to console
/// </summary>
void showDeck() {
	std::cout << "deckOfCards contains:" << "";
	for (std::vector<int>::iterator it = deckOfCards.begin(); it != deckOfCards.end(); ++it)
		std::cout << ' ' << *it;
	std::cout << std::endl;
}

/// <summary>
/// takes a card from the deck and puts it into
/// the hand that was passed as an argument
/// </summary>
/// <param name="hand"></param>
void takeACard(std::vector<int>& hand) {
	addToLogFile("draws ");
	addToLogFile(std::to_string(deckOfCards.back()));
	addToLogFile(" \n");
	hand.push_back(deckOfCards.back());
	deckOfCards.pop_back();
}

/// <summary>
/// Takes a card from the hand that was passed and 
/// puts it at the bottom of the deck
/// </summary>
/// <param name="hand">required that hand is size 2</param>
void putCardAtBottom(std::vector<int>& hand) {
	assert(hand.size() == 2);
	int cardToPut = distribution(generator);
	addToLogFile("discards ");
	addToLogFile(std::to_string(hand[cardToPut]));
	addToLogFile(" \n");
	deckOfCards.insert(deckOfCards.begin(), hand[cardToPut]);
	if (cardToPut == 0)
		hand.erase(hand.begin());
	else
		hand.pop_back();

}

/// <summary>
/// takes a card from the top of the deck
/// and gives it to each player
/// </summary>
void dealCards() {
	addToLogFile("DEALER: ");
	shuffleDeck();
	addToLogFile("DEALER: P1: ");
	takeACard(p1hand);
	addToLogFile("DEALER: P2: ");
	takeACard(p2hand);
	addToLogFile("DEALER: P2: ");
	takeACard(p3hand);
}

/// <summary>
/// Inelegant turn advancing logic solution
/// </summary>
void turnAdvance() {
	if (hasRoundBeenWon == false) {
		switch (currentRound) {
		case 1:
			if (currentTurn == 0)
				currentTurn = 1;
			else if (currentTurn == 3)
				currentTurn = 1;
			else
				++currentTurn;
			break;
		case 2:
			if (currentTurn == 0)
				currentTurn = 2;
			else if (currentTurn == 3)
				currentTurn = 1;
			else
				++currentTurn;
			break;
		case 3:
			if (currentTurn == 0)
				currentTurn = 3;
			else if (currentTurn == 3)
				currentTurn = 1;
			else
				++currentTurn;
			break;
		default:
			std::cerr << "something's wrong here turn iter" << std::endl;
			std::cerr << "currentRound: " << std::to_string(currentRound) << std::endl;
			std::cerr << "currentTurn: " << std::to_string(currentTurn) << std::endl;

			break;
		}
	}
	else {
		switch (currentTurn) {
		case 1:
			if (p2hand.empty())
				currentTurn = 0;
			else
				++currentTurn;
			break;
		case 2:
			if (p3hand.empty())
				currentTurn = 0;
			else
				++currentTurn;
			break;
		case 3:
			if (p1hand.empty())
				currentTurn = 0;
			else
				currentTurn = 1;
			break;
		default:
			std::cerr << "something's wrong here" << std::endl;
			std::cerr << "currentRound: " << std::to_string(currentRound) << std::endl;
			std::cerr << "currentTurn: " << std::to_string(currentTurn) << std::endl;

			break;
		}

	}
}

/// <summary>
/// Simulates exiting a round by putting all of the
/// cards that are in the passed hand into the deck
/// and clearing the hand
/// </summary>
/// <param name="hand"></param>
void exitRound(std::vector<int>& hand) {
	addToLogFile("exits round \n");
	for (std::vector<int>::iterator it = hand.begin(); it != hand.end(); ++it)
		deckOfCards.push_back(*it);
	hand.clear();
}

/// <summary>
/// Checks if the hand passed is a winning hand
/// </summary>
/// <param name="hand">the hand to be checked, must be of size 2</param>
/// <returns>whether hand passed is winning</returns>
bool isWinningHand(std::vector<int>& hand) {
	assert(hand.size() == 2);
	if (hand.front() == hand.back())
		return true;
	else
		return false;
}

void* dealer(void* param) {
	pthread_mutex_lock(&mutexDeck_or_Hand);
	while (currentRound != 4) {
		//wait for dealer's turn
		while (currentTurn != 0) {
			pthread_cond_signal(&cond);
			pthread_cond_wait(&cond, &mutexDeck_or_Hand);
		}
		if (currentTurn == 0) {
			if (!hasRoundBeenWon) {
				//code to start a new round
				dealCards();
				turnAdvance();
			}
			else {
				//code to end the current round
				//and put round results to console
				++currentRound;
				if (currentRound != 4)
					hasRoundBeenWon = false;
				for (int iter = 0; iter < 4; ++iter) {
					std::cout << consoleOutput[iter];
				}
			}
		}
	}
	pthread_mutex_unlock(&mutexDeck_or_Hand);
	pthread_cond_signal(&cond);
	pthread_exit(0);
	return NULL;
}


void* player1(void* param) {
	std::string whichPlayer = "PLAYER 1: ";
	int playerIndex = 1;
	std::vector<int>* whichHand;
	whichHand = &p1hand;

	pthread_mutex_lock(&mutexDeck_or_Hand);
	while (currentRound != 4) {
		while (currentTurn != playerIndex) {
			if (currentRound != 4) {
				pthread_cond_signal(&cond);
				pthread_cond_wait(&cond, &mutexDeck_or_Hand);
			}
			else {
				break;
			}
		}
		//This is to handle the in-round behavior
		//The player can only do anything when it's their turn
		//And in all other cases they are locked out of action
		if (currentTurn == playerIndex && hasRoundBeenWon == false) {
			addToLogFile(whichPlayer);
			logHand(*whichHand);

			addToLogFile(whichPlayer);
			takeACard(*whichHand);

			addToLogFile(whichPlayer);
			logHand(*whichHand);

			if (isWinningHand(*whichHand)) {
				addToLogFile(whichPlayer + "wins \n");
				hasRoundBeenWon = true;
				didThisThreadWin = true;
			}
			else {
				addToLogFile(whichPlayer);
				putCardAtBottom(*whichHand);

				addToLogFile(whichPlayer);
				logHand(*whichHand);
				addToLogFile(genDeckString());
				turnAdvance();
			}
		}
		//this logic is to handle the round end condition
		//a string for the output is generated
		//then the player exits the round
		if (currentTurn == playerIndex && hasRoundBeenWon == true) {

			if (didThisThreadWin == true) {
				didThisThreadWin = false;

				std::string tmpString = whichPlayer + "\n" + "HAND " +
					std::to_string(whichHand->front()) + " " +
					std::to_string(whichHand->back()) +
					" \n" + "WIN: yes \n";
				consoleOutput[playerIndex - 1] = tmpString;
				consoleOutput[3] = genDeckString();
			}
			else {

				std::string tmpString = whichPlayer + "\n" + "HAND " +
					std::to_string(whichHand->front()) + " \n" +
					"WIN: no \n";
				consoleOutput[playerIndex - 1] = tmpString;
			}

			addToLogFile(whichPlayer);
			exitRound(*whichHand);
			turnAdvance();
		}

	}

	pthread_mutex_unlock(&mutexDeck_or_Hand);
	pthread_cond_signal(&cond);
	pthread_exit(0);
	return NULL;
}


void* player2(void* param) {
	std::string whichPlayer = "PLAYER 2: ";
	int playerIndex = 2;
	std::vector<int>* whichHand;
	whichHand = &p2hand;

	pthread_mutex_lock(&mutexDeck_or_Hand);
	while (currentRound != 4) {
		while (currentTurn != playerIndex) {
			if (currentRound != 4) {
				pthread_cond_signal(&cond);
				pthread_cond_wait(&cond, &mutexDeck_or_Hand);
			}
			else {
				break;
			}
		}
		//This is to handle the in-round behavior
		//The player can only do anything when it's their turn
		//And in all other cases they are locked out of action	
		if (currentTurn == playerIndex && hasRoundBeenWon == false) {

			addToLogFile(whichPlayer);
			logHand(*whichHand);

			addToLogFile(whichPlayer);
			takeACard(*whichHand);

			addToLogFile(whichPlayer);
			logHand(*whichHand);

			if (isWinningHand(*whichHand)) {
				addToLogFile(whichPlayer + "wins \n");
				hasRoundBeenWon = true;
				didThisThreadWin = true;
			}
			else {
				addToLogFile(whichPlayer);
				putCardAtBottom(*whichHand);

				addToLogFile(whichPlayer);
				logHand(*whichHand);
				addToLogFile(genDeckString());
				turnAdvance();
			}
		}
		//this logic is to handle the round end condition
		//a string for the output is generated
		//then the player exits the round	
		if (currentTurn == playerIndex && hasRoundBeenWon == true) {

			if (didThisThreadWin == true) {
				didThisThreadWin = false;

				std::string tmpString = whichPlayer + "\n" + "HAND " +
					std::to_string(whichHand->front()) + " " +
					std::to_string(whichHand->back()) +
					" \n" + "WIN: yes \n";
				consoleOutput[playerIndex - 1] = tmpString;
				consoleOutput[3] = genDeckString();
			}
			else {

				std::string tmpString = whichPlayer + "\n" + "HAND " +
					std::to_string(whichHand->front()) + " \n" +
					"WIN: no \n";
				consoleOutput[playerIndex - 1] = tmpString;
			}

			addToLogFile(whichPlayer);
			exitRound(*whichHand);
			turnAdvance();

		}
	}
	pthread_mutex_unlock(&mutexDeck_or_Hand);
	pthread_cond_signal(&cond);
	pthread_exit(0);
	return NULL;
}


void* player3(void* param) {
	std::string whichPlayer = "PLAYER 3: ";
	int playerIndex = 3;
	std::vector<int>* whichHand;
	whichHand = &p3hand;

	pthread_mutex_lock(&mutexDeck_or_Hand);
	while (currentRound != 4) {
		//This is to handle the in-round behavior
		//The player can only do anything when it's their turn
		//And in all other cases they are locked out of action
		while (currentTurn != playerIndex) {
			if (currentRound != 4) {
				pthread_cond_signal(&cond);
				pthread_cond_wait(&cond, &mutexDeck_or_Hand);
			}
			else {
				break;
			}
		}
		if (currentTurn == playerIndex && hasRoundBeenWon == false) {

			addToLogFile(whichPlayer);
			logHand(*whichHand);

			addToLogFile(whichPlayer);
			takeACard(*whichHand);

			addToLogFile(whichPlayer);
			logHand(*whichHand);

			if (isWinningHand(*whichHand)) {
				addToLogFile(whichPlayer + "wins \n");
				hasRoundBeenWon = true;
				didThisThreadWin = true;
			}
			else {
				addToLogFile(whichPlayer);
				putCardAtBottom(*whichHand);

				addToLogFile(whichPlayer);
				logHand(*whichHand);
				addToLogFile(genDeckString());
				turnAdvance();
			}
		}
		//this logic is to handle the round end condition
		//a string for the output is generated
		//then the player exits the round		
		if (currentTurn == playerIndex && hasRoundBeenWon == true) {

			if (didThisThreadWin == true) {
				didThisThreadWin = false;

				std::string tmpString = whichPlayer + "\n" + "HAND " +
					std::to_string(whichHand->front()) + " " +
					std::to_string(whichHand->back()) +
					" \n" + "WIN: yes \n";
				consoleOutput[playerIndex - 1] = tmpString;
				consoleOutput[3] = genDeckString();
			}
			else {
				std::string tmpString = whichPlayer + "\n" + "HAND " +
					std::to_string(whichHand->front()) + " \n" +
					"WIN: no \n";
				consoleOutput[playerIndex - 1] = tmpString;
			}

			addToLogFile(whichPlayer);
			exitRound(*whichHand);
			turnAdvance();
		}
	}
	pthread_mutex_unlock(&mutexDeck_or_Hand);
	pthread_cond_signal(&cond);
	pthread_exit(0);
	return NULL;
}



int main(int argc, char* argv[]) {
	pthread_attr_t attr;
	pthread_mutex_init(&mutexDeck_or_Hand, NULL);
	//handling the seed the user might or might not input
	if (argc != 2) {
	std::cerr << "This program takes one integer as an argument" << std::endl;
		return 1;
	}
	randomSeed = std::atoi(argv[argc - 1]);
	//randomSeed = 18;
	generator = std::default_random_engine(randomSeed);

	//filling the vector to make it look like a real deck of cards (no jokers of course)
	for (int i = 1; i < 14; ++i) {
		for (int j = 0; j < 3; ++j) {
			deckOfCards.push_back(i);
		}
	}
	addToLogFile("GAME START\n");
	std::cout << "GAME START" << std::endl;
	pthread_create(&dealer_ID, NULL, dealer, NULL);
	pthread_create(&p1_ID, NULL, player1, NULL);
	pthread_create(&p2_ID, NULL, player2, NULL);
	pthread_create(&p3_ID, NULL, player3, NULL);
	pthread_join(dealer_ID, NULL);
	pthread_join(p1_ID, NULL);
	pthread_join(p2_ID, NULL);
	pthread_join(p3_ID, NULL);

	pthread_mutex_destroy(&mutexDeck_or_Hand);
	addToLogFile("GAME END\n");
	std::cout << "GAME END" << std::endl;

	return 0;
}
