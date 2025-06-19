//---------------------------------------------------------------------------------------------------------------------
// This program implements a card-based game called "Entertaining Spice Pretending," 
// where two players take turns playing cards from their hands, drawing from a shared 
// deck, and issuing challenges based on claimed card properties (value or spice type).
// 
// The program features functionalities for reading a configuration file to initialize 
// the card deck, distributing cards to players, managing game rounds with challenges, 
// and determining the winner based on the players' scores.
//
// Tutor: Hannah Weberbauer>
//
// Milos Dukaric: <12225387>
//---------------------------------------------------------------------------------------------------------------------
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

typedef struct Card
{
  int value;
  char spice;
  struct Card *next;
} Card;

typedef struct Player
{
  Card *hand;
  int score;
  Card *latest_card;  
  Card *claimed_card; 
} Player;

//---------------------------------------------------------------------------------------------------------------------
/// Checks whether the correct number of command-line arguments has been provided.
/// @param argc The number of arguments passed to the program from the command line.
/// @return 0 if the correct number of arguments is provided; 
/// 1 if an incorrect number of arguments is provided.
int parseArg(int argc)
{
  if (argc != 2)
  {
    printf("Usage: ./a4-csf <config file>\n"); 
    return 1;
  }
  return 0;
}

//---------------------------------------------------------------------------------------------------------------------
/// Checks the validity of the configuration file provided at the given path.
/// Validates that the file exists and contains the correct header.
/// @param config_path The path to the configuration file to be checked.
/// @return 0 if the file is valid and can be used; 
/// 2 if the file cannot be opened; 
/// 3 if the file has an invalid format.
int checkConfig(const char *config_path)
{
  FILE *config_file = fopen(config_path, "r");
  if (!config_file)
  {
    printf("Error: Cannot open file: %s\n", config_path);
    return 2;
  }

  char file_header[5];
  if (fgets(file_header, sizeof(file_header), config_file) == NULL || strcmp(file_header, "ESP\n") != 0)
  {
    printf("Error: Invalid file: %s\n", config_path);
    fclose(config_file);
    return 3;
  }

  fclose(config_file);
  return 0;
}

Card *parse_cards(const char *config_path)
{
  FILE *config_file = fopen(config_path, "r");
  char line[10];
  Card *head = NULL;
  Card *tail = NULL;

  fgets(line, sizeof(line), config_file);

  while (fgets(line, sizeof(line), config_file))
  {
    int value;
    char spice;

    if (sscanf(line, "%d_%c\n", &value, &spice) != 2 || value < 1 || value > 10 || (spice != 'w' && spice != 'c' && spice != 'p'))
    {
      printf("Error: Invalid card format: %s", line);
      fclose(config_file);
      return NULL;
    }

    Card *new_card = malloc(sizeof(Card));
    if (!new_card)
    {
      printf("Error: Out of memory\n");
      fclose(config_file);
      return NULL;
    }

    new_card->value = value;
    new_card->spice = spice;
    new_card->next = NULL;

    if (!head)
    {
      head = new_card;
      tail = new_card;
    }
    else
    {
      tail->next = new_card;
      tail = new_card;
    }
  }

  fclose(config_file);
  return head;
}

Card *reverse_cards(Card *head)
{
  Card *prev = NULL;
  Card *current = head;
  Card *next = NULL;

  while (current)
  {
    next = current->next;
    current->next = prev;
    prev = current;
    current = next;
  }

  return prev;
}

Card *sort_cards(Card *head)
{
  if (!head || !head->next)
    return head;

  Card *sorted = NULL;

  while (head)
  {
    Card *current = head;
    head = head->next;

    if (!sorted || (current->spice < sorted->spice) ||
        (current->spice == sorted->spice && current->value < sorted->value))
    {
      current->next = sorted;
      sorted = current;
    }
    else
    {
      Card *temp = sorted;
      while (temp->next && (temp->next->spice < current->spice ||
                            (temp->next->spice == current->spice && temp->next->value < current->value)))
      {
        temp = temp->next;
      }
      current->next = temp->next;
      temp->next = current;
    }
  }

  return sorted;
}

//---------------------------------------------------------------------------------------------------------------------
/// Distributes cards from the deck to two players and a draw pile.
/// Each player is initially dealt 6 cards, alternating between players.
/// Remaining cards are added to the draw pile.
/// @param deck The linked list of cards to be distributed.
/// @param player1 The first player to receive cards.
/// @param player2 The second player to receive cards.
/// @param draw_pile A pointer to the draw pile where remaining cards will be stored.
void distributeCards(Card *deck, Player *player1, Player *player2, Card **draw_pile)
{
  int player1_cards = 0, player2_cards = 0;
  Card *current = deck;
  int turn = 1;

  while (current)
  {
    Card *next_card = current->next;

    if (player1_cards < 6 && turn == 1)
    {
      current->next = player1->hand;
      player1->hand = current;
      player1_cards++;
      turn = 2;
    }
    else if (player2_cards < 6 && turn == 2)
    {
      current->next = player2->hand;
      player2->hand = current;
      player2_cards++;
      turn = 1; 
    }
    else
    {
      current->next = *draw_pile;
      *draw_pile = current;
    }

    current = next_card;
  }

  player1->hand = reverse_cards(player1->hand);
  player2->hand = reverse_cards(player2->hand);
  *draw_pile = reverse_cards(*draw_pile);
}


//---------------------------------------------------------------------------------------------------------------------
/// Frees the memory allocated for a linked list of cards.
/// This function traverses the list and deallocates memory for each card.
/// @param head The head of the linked list to be freed.
void freeCards(Card *head)
{
  while (head)
  {
    Card *temp = head;
    head = head->next;
    free(temp);
  }
}

//---------------------------------------------------------------------------------------------------------------------
/// Prints a header indicating the start of a new round.
/// This function outputs a visually distinct separator and a "ROUND START" message.
void printStart(void)
{
  printf("\n-------------------\n");
  printf("ROUND START\n");
  printf("-------------------\n");
}



//---------------------------------------------------------------------------------------------------------------------
/// Resets the round by deallocating the latest and claimed cards for both players.
/// This function ensures all dynamically allocated cards for the current round are freed and resets the cards played count.
/// @param player1 A pointer to the first player's data structure.
/// @param player2 A pointer to the second player's data structure.
/// @param cards_played_this_round A pointer to the counter tracking the number of cards played in the current round.
void resetRound(Player *player1, Player *player2, int *cards_played_this_round)
{
  if (player1->latest_card)
  {
    free(player1->latest_card);
    player1->latest_card = NULL;
  }

  if (player1->claimed_card)
  {
    free(player1->claimed_card);
    player1->claimed_card = NULL;
  }

  if (player2->latest_card)
  {
    free(player2->latest_card);
    player2->latest_card = NULL;
  }

  if (player2->claimed_card)
  {
    free(player2->claimed_card);
    player2->claimed_card = NULL;
  }

  *cards_played_this_round = 0;
}


//---------------------------------------------------------------------------------------------------------------------
/// Handles the "play" command from a player by verifying the card input and updating the game state.
/// The function validates the player's input, removes the specified card from their hand, 
/// and updates the latest and claimed card details.
/// @param current_player A pointer to the player executing the command.
/// @param opponent A pointer to the opponent player (used for context if needed).
/// @param cards_played_this_round A pointer to the counter tracking the number of cards played in the current round.
/// @param latest_card_number A pointer to the variable storing the value of the latest card played.
/// @param latest_card_spice A pointer to the variable storing the spice of the latest card played.
/// @return 0 if the play command is successful; 
/// 1 if the input is invalid or the specified card is not in the player's hand.
int handlePlayCommand(Player *current_player, Player *opponent, int *cards_played_this_round, int *latest_card_number, char *latest_card_spice)
{
  char hand_card_input[10];
  char claimed_card_input[10];

  if (scanf("%s %s", hand_card_input, claimed_card_input) != 2)
  {
    printf("Please enter the correct number of parameters!\n");
    return 1;
  }

  int hand_value, claimed_value;
  char hand_spice, claimed_spice;

  if (sscanf(hand_card_input, "%d_%c", &hand_value, &hand_spice) != 2 ||
      sscanf(claimed_card_input, "%d_%c", &claimed_value, &claimed_spice) != 2)
  {
    printf("Please enter the cards in the correct format!\n");
    return 1;
  }

  Card *prev = NULL, *current = current_player->hand;

  while (current)
  {
    if (current->value == hand_value && current->spice == hand_spice)
    {
      if (prev)
        prev->next = current->next;
      else
        current_player->hand = current->next;

      current->next = NULL;
      current_player->latest_card = current;

      if (current_player->claimed_card)
      {
        free(current_player->claimed_card);
      }

      current_player->claimed_card = malloc(sizeof(Card));
      if (!current_player->claimed_card)
      {
        printf("Error: Out of memory\n");
        return 1;
      }

      *latest_card_number = claimed_value;
      *latest_card_spice = claimed_spice;

      current_player->claimed_card->value = claimed_value;
      current_player->claimed_card->spice = claimed_spice;
      current_player->claimed_card->next = NULL;

      (*cards_played_this_round)++;
      return 0;
    }
    prev = current;
    current = current->next;
  }

  printf("Please enter a card in your hand cards!\n");
  return 1;
}


//---------------------------------------------------------------------------------------------------------------------
/// Handles a challenge made by one player against their opponent's claimed card.
/// Validates the challenge based on the specified type (value or spice) and updates scores, 
/// hands, and the draw pile accordingly.
/// @param challenger A pointer to the player issuing the challenge.
/// @param opponent A pointer to the player whose claimed card is being challenged.
/// @param type The type of challenge ("value" or "spice"). Case insensitive.
/// @param cards_played_this_round A pointer to the counter tracking the number of cards played in the current round.
/// @param draw_pile A pointer to the draw pile where cards are stored.
/// @param latest_played_card A pointer to the variable storing the value of the latest card played.
/// @param latest_card_spice A pointer to the variable storing the spice of the latest card played.
void handleChallenge(Player *challenger, Player *opponent, const char *type, int *cards_played_this_round, Card **draw_pile, int *latest_played_card, char *latest_card_spice)
{
  if (!opponent->claimed_card || !opponent->latest_card)
  {
    printf("Challenge cannot be used at the moment, because the opponent did not play a card on their latest turn.\n");
    return;
  }

  int challenge_successful = 0;

  if (strcasecmp(type, "value") != 0 && strcasecmp(type, "spice") != 0)
  {
    printf("Please choose SPICE or VALUE!\n");
    return;
  }

  if (strcasecmp(type, "value") == 0)
  {
    if (opponent->claimed_card->value != opponent->latest_card->value)
    {
      challenge_successful = 1;
    }
  }
  else if (strcasecmp(type, "spice") == 0)
  {
    if (opponent->claimed_card->spice != opponent->latest_card->spice)
    {
      challenge_successful = 1;
    }
  }

  if (challenge_successful)
  {
    printf("Challenge successful: %d_%c's %s does not match the real card %d_%c.\n",
           *latest_played_card, *latest_card_spice, type,
           opponent->latest_card->value, opponent->latest_card->spice);

    challenger->score += *cards_played_this_round;
    printf("Player %d gets %d points.\n", (challenger == opponent ? 2 : 1), *cards_played_this_round);

    for (int i = 0; i < 2 && *draw_pile; i++)
    {
      Card *new_card = *draw_pile;
      *draw_pile = new_card->next;
      new_card->next = opponent->hand;
      opponent->hand = new_card;
    }

    if (!opponent->hand)
    {
      printf("Player %d gets 10 bonus points (last card).\n", (challenger == opponent ? 1 : 2));
      challenger->score += 10;
    }
  }
  else
  {
    printf("Challenge failed: %d_%c's %s matches the real card %d_%c.\n",
           *latest_played_card, *latest_card_spice, type,
           opponent->latest_card->value, opponent->latest_card->spice);

    opponent->score += *cards_played_this_round;
    printf("Player %d gets %d points.\n", (challenger == opponent ? 1 : 2), *cards_played_this_round);

    for (int i = 0; i < 2 && *draw_pile; i++)
    {
      Card *new_card = *draw_pile;
      *draw_pile = new_card->next;
      new_card->next = challenger->hand;
      challenger->hand = new_card;
    }
  }

  resetRound(challenger, opponent, cards_played_this_round);
}



//---------------------------------------------------------------------------------------------------------------------
/// Determines the winner of the game by comparing the scores of the two players.
/// Outputs the scores of both players and announces the winner or a tie if the scores are equal.
/// @param player1 A pointer to the first player's data structure, containing their score.
/// @param player2 A pointer to the second player's data structure, containing their score.
void determineWinner(Player *player1, Player *player2)
{
  if (player1->score > player2->score)
  {
    printf("\nPlayer 1: %d points\n", player1->score);
    printf("Player 2: %d points\n", player2->score);
    printf("\nCongratulations! Player 1 wins the game!\n");
  }
  else if (player2->score > player1->score)
  {
    printf("\nPlayer 2: %d points\n", player2->score);
    printf("Player 1: %d points\n", player1->score);
    printf("\nCongratulations! Player 2 wins the game!\n");
  }
  else
  {
    printf("\nPlayer 1: %d points\n", player1->score);
    printf("Player 2: %d points\n", player2->score);
    printf("\nCongratulations! Player 1 wins the game!\n");
    printf("Congratulations! Player 2 wins the game!\n");
  }
}



//---------------------------------------------------------------------------------------------------------------------
/// Handles a single turn for the current player in the game.
/// The player can issue commands such as "play," "challenge," "quit," or "draw," and the game state updates accordingly.
/// Validates commands, executes actions, and checks for end-of-game conditions.
/// @param player_number The identifier of the current player (e.g., 1 or 2).
/// @param current_player A pointer to the current player's data structure.
/// @param opponent A pointer to the opponent player's data structure.
/// @param cards_played_this_round A pointer to the counter tracking the number of cards played in the current round.
/// @param draw_pile A pointer to the draw pile of cards.
/// @param game_over A pointer to the flag indicating if the game has ended.
/// @param latest_played_card A pointer to the variable storing the value of the latest card played.
/// @param latest_card_spice A pointer to the variable storing the spice of the latest card played.
void playerTurn(int player_number, Player *current_player, Player *opponent, int *cards_played_this_round, Card **draw_pile, int *game_over, int *latest_played_card, int *latest_card_spice)
{
  printf("\nPlayer %d:\n", player_number);

  if (opponent->claimed_card)
  {
    printf("    latest played card: %d_%c\n", *latest_played_card, *latest_card_spice);
  }
  else
  {
    printf("    latest played card:\n");
  }

  printf("    cards played this round: %d\n", *cards_played_this_round);

  printf("    hand cards:");
  current_player->hand = sort_cards(current_player->hand);
  Card *current = current_player->hand;
  while (current)
  {
    printf(" %d_%c", current->value, current->spice);
    current = current->next;
  }

  int wrong_input = 1;

  while (wrong_input)
  {
    wrong_input = 0;
    printf("\nP%d > ", player_number);

    char command[10];
    scanf("%s", command);
    if (strcasecmp(command, "play") == 0)
    {
      while (handlePlayCommand(current_player, opponent, cards_played_this_round, latest_played_card, latest_card_spice) != 0)
      {
        printf("P%d > ", player_number);
      }
    }
    else if (strcasecmp(command, "challenge") == 0)
    {
      char type[10];
      scanf("%s", type);

      if (strcasecmp(type, "value") != 0 && strcasecmp(type, "spice") != 0)
      {
        printf("Invalid challenge type! Please choose 'value' or 'spice'.\n");
      }
      else
      {
        handleChallenge(current_player, opponent, type, cards_played_this_round, draw_pile, latest_played_card, latest_card_spice);

        if (!*draw_pile)
        {
          *game_over = 1;
          determineWinner(current_player, opponent);
          return;
        }

        printStart();
      }
    }
    else if (strcasecmp(command, "quit") == 0)
    {
      char extra_input[10];
      if (scanf("%s", extra_input) != EOF)
      {
        printf("Please enter the correct number of parameters!");
        wrong_input = 1;
      } else {
        *game_over = 1;
      }
    }
    else if (strcasecmp(command, "draw") == 0)
    {
      if (!*draw_pile)
      {
        *game_over = 1;
        determineWinner(current_player, opponent);
        return; 
      }
      else
      {
        Card *drawn_card = *draw_pile;
        *draw_pile = drawn_card->next;
        drawn_card->next = current_player->hand;
        current_player->hand = drawn_card;

        if (!*draw_pile)
        {
          *game_over = 1;
          determineWinner(current_player, opponent);
          return;
        }
      }
    }
    else
    {
      printf("Please enter a valid command!\n");
    }

    int c;
    while ((c = getchar()) != '\n' && c != EOF)
    {
    }
  }
}


//---------------------------------------------------------------------------------------------------------------------
/// Conducts a game round with challenges, alternating turns between two players.
/// The round continues until the draw pile and both players' hands are empty or the game ends due to a quit command.
/// Validates and processes each player's turn and determines the winner if the game ends.
/// @param player1 A pointer to the first player's data structure.
/// @param player2 A pointer to the second player's data structure.
/// @param draw_pile A pointer to the draw pile of cards.
void roundWithChallenges(Player *player1, Player *player2, Card **draw_pile)
{
  int game_over = 0; 
  int latest_card_number = 0;
  int latest_card_spice = '\0';

  while (!game_over && (*draw_pile || player1->hand || player2->hand))
  {
    printStart();
    int cards_played_this_round = 0;

    while (!game_over && (*draw_pile || player1->hand || player2->hand))
    {
      playerTurn(1, player1, player2, &cards_played_this_round, draw_pile, &game_over, &latest_card_number, &latest_card_spice);

      if (game_over)
      {
        break;
      }
      playerTurn(2, player2, player1, &cards_played_this_round, draw_pile, &game_over, &latest_card_number, &latest_card_spice);
    }

    if (!*draw_pile && !player1->hand && !player2->hand && !game_over)
    {
      determineWinner(player1, player2);
    }
  }
}


//---------------------------------------------------------------------------------------------------------------------
/// The main entry point for the game "Entertaining Spice Pretending."
/// This function initializes the game by parsing arguments, validating the configuration file, 
/// creating the card deck, distributing cards to players, and starting the game rounds.
/// Cleans up allocated memory and handles errors appropriately.
/// @param argc The number of arguments passed to the program from the command line.
/// @param argv An array of strings representing the command-line arguments.
/// @return 0 if the program executes successfully; 
/// non-zero error codes for invalid arguments, configuration file errors, or other issues.
int main(int argc, char *argv[])
{
  int result = parseArg(argc);
  if (result != 0)
  {
    return result;
  }

  result = checkConfig(argv[1]);
  if (result != 0)
  {
    return result;
  }

  printf("Welcome to Entertaining Spice Pretending!\n");

  Card *deck = parse_cards(argv[1]);
  if (!deck)
  {
    return 3;
  }

  Player player1 = {NULL, 0, NULL, NULL};
  Player player2 = {NULL, 0, NULL, NULL};
  Card *draw_pile = NULL;

  distributeCards(deck, &player1, &player2, &draw_pile);

  roundWithChallenges(&player1, &player2, &draw_pile);

  freeCards(player1.hand);
  freeCards(player2.hand);
  freeCards(draw_pile);

  return 0;
}
