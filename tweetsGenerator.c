#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_WORDS_IN_SENTENCE_GENERATION 20
#define MAX_WORD_LENGTH 100
#define MAX_SENTENCE_LENGTH 1000
#define VALID_ARGC_4 4
#define VALID_ARGC_5 5
#define BASE 10
#define USAGE_MSG "Usage: <seed><tweets><path><optional: number of words>\n"
#define FILE_MSG "Error: file path is invalid\n"
#define ALLOC_ERROR "Allocation failure: memory allocation was failed\n"
#define SPLIT " \n"
#define DOT '.'

FILE *words_file; // will be pointer to the file we read - global in order to
// be able to close it while allocation error.


typedef struct WordStruct {
    char *word;
    struct WordProbability *prob_list;
    int prob_list_size;
    int appears_in_txt;
} WordStruct;

typedef struct WordProbability {
    struct WordStruct *word_struct_ptr;
    int appears_after_word;
} WordProbability;

/**** LINKED LIST ****/
typedef struct Node {
    WordStruct *data;
    struct Node *next;
} Node;

typedef struct LinkList {
    Node *first;
    Node *last;
    int size;
} LinkList;

/**
 * Add data to new node at the end of the given link list.
 * @param link_list Link list to add data to
 * @param data pointer to dynamically allocated data
 * @return 0 on success, 1 otherwise
 */
int add (LinkList *link_list, WordStruct *data)
{
  Node *new_node = malloc (sizeof (Node));
  if (new_node == NULL)
    {
      return 1;
    }
  *new_node = (Node) {data, NULL};

  if (link_list->first == NULL)
    {
      link_list->first = new_node;
      link_list->last = new_node;
    }
  else
    {
      link_list->last->next = new_node;
      link_list->last = new_node;
    }

  link_list->size++;
  return 0;
}
/*************/

/**
 * Get random number between 0 and max_number [0, max_number).
 * @param max_number
 * @return Random number
 */
int get_random_number (int max_number)
{
  int rnd_num;
  rnd_num = rand () % max_number;
  return rnd_num;
}

/**
 * Choose randomly the next word from the given dictionary, drawn uniformly.
 * The function won't return a word that end's in full stop '.' (Nekuda).
 * @param dictionary Dictionary to choose a word from
 * @return WordStruct of the chosen word
 */
WordStruct *get_first_random_word (LinkList *dictionary)
{
  WordStruct *chosen_node;
  char *chosen_word;
  while (1) // endless loop that stop when we choose valid word- that do not
    // ends with '.'
    {
      int num = get_random_number (dictionary->size); // rand number between 0
      // to the dictionary size.
      int i = 0;
      Node *node = dictionary->first;
      while (i < num)
        {
          node = node->next;
          i++;
        }
      chosen_node = node->data;
      chosen_word = node->data->word;
      int len = (int) strlen (chosen_word);
      if (chosen_word[len - 1] == DOT)
        {
          continue;
        }
      else // if the word do not ends with '.' - we done
        break;
    }
  return chosen_node;
}

/**
 * Choose randomly the next word. Depend on it's occurrence frequency
 * in word_struct_ptr->WordProbability.
 * @param word_struct_ptr WordStruct to choose from
 * @return WordStruct of the chosen word
 */
WordStruct *get_next_random_word (WordStruct *word_struct_ptr)
{
  int rnd_num = get_random_number (word_struct_ptr->appears_in_txt); // rand
  // number between 0 to the word's prob list size.
  struct WordStruct *chosen_word;
  rnd_num += 1;
  for (int j = 0; j < word_struct_ptr->prob_list_size; j++)
    {
      if (rnd_num <= word_struct_ptr->prob_list[j].appears_after_word)
        {
          // if the rnd_num is smaller than the num of appears - the current
          // word is the word to choose
          chosen_word = word_struct_ptr->prob_list[j].word_struct_ptr;
          return chosen_word;
        }
        // otherwise - we continue on the prob list and less the current
        // word's num of appears from the rnd number
      else
        rnd_num -= word_struct_ptr->prob_list[j].appears_after_word;
    }
  return NULL; // will never happen
}

/**
 * Receive dictionary, generate and print to stdout random sentence out of it.
 * The sentence most have at least 2 words in it.
 * @param dictionary Dictionary to use
 * @return Amount of words in printed sentence
 */
int generate_sentence (LinkList *dictionary)
{
  static int iteration = 1; // static variable that represents the number of
  // tweets we already did.
  printf ("Tweet %d: ", iteration);

  WordStruct *first_word = get_first_random_word (dictionary);
  char *cur_word = first_word->word;
  WordStruct *cur_word_ptr = first_word; // save the current word in order to
  // rand it's next word.
  printf ("%s ", cur_word);
  int word_in_sen = 1; // variable represents the number of words printed.
  while (1)
    {
      WordStruct *next_word = get_next_random_word (cur_word_ptr);
      cur_word = next_word->word;
      cur_word_ptr = next_word;
      word_in_sen++;
      int n = (int) strlen (next_word->word);
      // if the last word printed is ends with '.' or we already printed 20
      // words, we stop generate the sentence.
      if (cur_word[n - 1] == DOT
          || word_in_sen == MAX_WORDS_IN_SENTENCE_GENERATION)
        {
          printf ("%s", cur_word);
          printf ("\n");
          break;
        }
      else
        printf ("%s ", cur_word);
    }
  iteration += 1;
  return word_in_sen;
}

void deal_with_allocation_error (void *ptr)
{
  if (ptr == NULL)
    {
      fprintf (stdout, ALLOC_ERROR);
      fclose (words_file);
      exit (EXIT_FAILURE);
    }
}
/**
 * Gets 2 WordStructs. If second_word in first_word's prob_list,
 * update the existing probability value.
 * Otherwise, add the second word to the prob_list of the first word.
 * @param first_word
 * @param second_word
 * @return 0 if already in list, 1 otherwise.
 */
int add_word_to_probability_list (WordStruct *first_word,
                                  WordStruct *second_word)
{
  // check if the second word is already in the first word's prod list.
  for (int i = 0; i < first_word->prob_list_size; i++)
    {

      if (strcmp (first_word->prob_list[i].word_struct_ptr->word, \
      second_word->word)
          == 0)
        {
          // if it is, we only add 1 to the second word's num of appears.
          first_word->prob_list[i].appears_after_word += 1;
          return 0;
        }
    }
  // if it is not, we create/resize the prob list.
  int new_size = first_word->prob_list_size + 1;
  // if new size is 1 - the prob was not initialized.
  if (new_size == 1)
    {
      first_word->prob_list = malloc (sizeof (WordProbability));
      deal_with_allocation_error (first_word->prob_list);
    }
    // the prob list is exist, so we re-alloc it.
  else
    {
      WordProbability *temp;
      temp = realloc (first_word->prob_list,
                      new_size * sizeof (WordProbability));
      deal_with_allocation_error (temp);
      first_word->prob_list = temp;
    }

  first_word->prob_list_size = new_size;
  struct WordProbability new_word = (WordProbability) \
                                             {second_word, 1};
  first_word->prob_list[new_size - 1] = new_word;
  return 1;
}

/**
 * Read word from the given file. Add every unique word to the dictionary.
 * Also, at every iteration, update the prob_list of the previous word with
 * the value of the current word.
 * @param fp File pointer
 * @param words_to_read Number of words to read from file.
 *                      If value is bigger than the file's word count,
 *                      or if words_to_read == -1 than read entire file.
 * @param dictionary Empty dictionary to fill
 */

char *allocate_one_word ()
/**
 * this function allocate memory to to the string of the word.
 * @return pointer to the allocated memory
 */
{
  char *one_word = malloc (MAX_WORD_LENGTH * sizeof (char));
  deal_with_allocation_error (one_word);
  return one_word;
}

WordStruct *allocate_word_struct ()
/**
 * this function allocate memory to word struct object.
 * @return pointer to the allocated memory
 */
{
  WordStruct *one_wordstruct = malloc (sizeof (WordStruct));
  deal_with_allocation_error (one_wordstruct);
  return one_wordstruct;
}

WordStruct *check_if_in_dict (char *word_ptr, LinkList *dictionary)
/**
 * this function check if a word is already in the dictionary, or not.
 * @param word_ptr pointer to string we search
 * @param dictionary the dictionary we search in - pointer to LinkList.
 * @return pointer to the word, if it in the dictionary, and NULL if it is not.
 */
{
  Node *cur = dictionary->first;
  while (cur != NULL)
    {
      if (strcmp (word_ptr, cur->data->word) == 0)
        return cur->data;
      cur = cur->next;
    }
  return NULL;
}

WordStruct *
add_word_to_wordstruct (char *word_ptr, LinkList *dictionary, int *flag)
/** this function creates a new words struct if the word is not i the
 * dictionary, and updates the exist one, it the word is in the dictionary.
 * @param word_ptr pointer to the word's string we add to the dict.
 * @param dictionary pointer to the dictionary
 * @param flag pointer to variable we updates according to if the word is
 * already in the dictionary or not.
 * @return pointer to the exist/ new word struct.
 */
{
  WordStruct *cur_word = check_if_in_dict (word_ptr, dictionary);
  if (cur_word != NULL) // means the word is already in the dictionary.
    {
      cur_word->appears_in_txt += 1;
      *flag = 1; // a flag that represents in fill_dictionary if the word was
      // is the dict, or not.
      return cur_word;
    }
  else // the word is not in the dict - we create a new one.
    {
      char *new_word = allocate_one_word ();
      strcpy (new_word, word_ptr); // copy the string to new pointer, in order
      // to keep it outside the function.
      WordStruct *new_word_struct = allocate_word_struct ();
      // initialize the fields:
      new_word_struct->word = new_word;
      new_word_struct->appears_in_txt = 1;
      new_word_struct->prob_list_size = 0;
      new_word_struct->prob_list = NULL;
      return new_word_struct;
    }
}

int add_to_prob (WordStruct *prev_word, WordStruct *cur_word)
/** this function decide if to add the current word to the prev word's prod
 * list according to some conditions. if the prev word is not NULL means the
 * prev word is not an end of a sentence, and if it not ends with '.' - we add
 * the word to it's prob list.
 * @param prev_word pointer to the word we check if to updates it's prod list.
 * @param cur_word pointer to the word we want to add to prob list.
 * @return 1 if we did not add to prod list, and 0 otherwise.
 */
{
  if (prev_word == NULL)
    return 1;
  int n = (int) strlen (prev_word->word);
  if (prev_word->word[n - 1] == DOT)
    return 1;

  add_word_to_probability_list (prev_word, cur_word);
  return 0;
}

void fill_dictionary (FILE *fp, int words_to_read, LinkList *dictionary)
/**
 * this function read words from the given file, and creates words struct
 * nodes, to fill the dictionary with.
 * the function read 'words_to_read' words from the file, unless words_to_read
 * is 0, and the function reads all file.
 * @param fp pointer to the file
 * @param words_to_read number of words to read (if it 0 - read all file)
 * @param dictionary the dictionary to fill
 */
{
  struct WordStruct *prev_word = NULL; // initialize variable of pointer to
  // WordStruct - that will represents the prev word we read.
  int i = 0; // variable represents the number of words we already read.
  char sentence[MAX_SENTENCE_LENGTH];
  while (fgets (sentence, MAX_SENTENCE_LENGTH, fp) != NULL)
    {
      char *word_ptr = strtok (sentence, SPLIT);
      // second condition - if the number of words to read is not 0, we stop
      // when we read the words we asked for.
      while ((word_ptr != NULL)
             && ((words_to_read == 0) || (i < words_to_read)))
        {
          int in_dict = 0; // variable represents if the current word is in
          // the dict or not. updated add_word_to_wordstruct function.

          WordStruct *new_word_struct = add_word_to_wordstruct (word_ptr, \
                                                         dictionary, &in_dict);
          if (in_dict == 0) // if the word not in the dictionary - add it.
            add (dictionary, new_word_struct);
          i++; // add 1 to the number of words we read.

          add_to_prob (prev_word, new_word_struct);

          prev_word = new_word_struct;
          word_ptr = strtok (NULL, SPLIT);
        }

      if (feof(fp) || i == words_to_read)
        break;
      prev_word = NULL; // at the end of a sentence, we updates the prev word
      // to NULL.
    }
  fclose (fp);
}

/**
 * Free the given dictionary and all of it's content from memory.
 * @param dictionary Dictionary to free
 */
void free_dictionary (LinkList *dictionary)
{
  Node *cur_node = dictionary->first;
  while (cur_node != NULL)
    {
      Node *next_node = cur_node->next;
      free (cur_node->data->prob_list);
      free (cur_node->data->word);
      free (cur_node->data);
      free (cur_node);

      cur_node = NULL;
      cur_node = next_node;
    }
}

int argc_validity (int argc, char *argv[], FILE **f_ptr)
/**
 * this function check the validity of the CLI arguments.
 * @param argc num of arguments we got
 * @param argv the arguments from the CLI
 * @param f_ptr pointer to file that we update if the opening succeeded.
 * @return return 1 if something is invalid, 0 otherwise.
 */
{
  if ((argc != VALID_ARGC_5) && (argc != VALID_ARGC_4))
    {
      fprintf (stdout, USAGE_MSG);
      return 1;
    }
  FILE *f = fopen (argv[3], "r");
  if (f == NULL)
    {
      fprintf (stdout, FILE_MSG);
      return 1;
    }
  *f_ptr = f;
  return 0;
}

void initialize_seed (char *num)
/**
 * this function initialized rand function with the seed at the start of the
 *  program.
 * @param num the the seed we get from the user
 */
{
  long seed = strtol (num, NULL, BASE);
  srand (seed);
}

int get_num_words_to_read (int argc, char *argv[])
/**
 * this function get from the argv the number of words to read, if we got this
 * parameter, and initialize it to 0 if we did not get it.
 * @param argc argc num of arguments we got
 * @param argv the arguments from the CLI
 * @return the number of words to read
 */
{
  int n = 0;
  if (argc == VALID_ARGC_5)
    {
      n = (int) strtol (argv[4], NULL, BASE);
      if (n == -1)
        n = 0;
    }
  return n;
}

/**
 * @param argc
 * @param argv 1) Seed
 *             2) Number of sentences to generate
 *             3) Path to file
 *             4) Optional - Number of words to read
 */
int main (int argc, char *argv[])
{
  if (argc_validity (argc, argv, &words_file) == 1)
    return EXIT_FAILURE;

  initialize_seed (argv[1]);
  int num_of_words = get_num_words_to_read (argc, argv);
  LinkList dict = (LinkList) {NULL, NULL, 0};
  fill_dictionary (words_file, num_of_words, &dict);

//  print_dictionary(&dict);

  int num_of_tweets = (int) strtol (argv[2], NULL, BASE);
  for (int i = 0; i < num_of_tweets; i++)
    generate_sentence (&dict);

  free_dictionary (&dict);
  return EXIT_SUCCESS;
}