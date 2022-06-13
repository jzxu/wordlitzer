#include <algorithm>
#include <cassert>
#include <cstdio>
#include <functional>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>

using Outcome = std::pair<int, int>;
using LetterCounts = std::unordered_map<char, int>;

constexpr int WORD_LENGTH = 5;
constexpr int MAX_CANDIDATES = 100;

std::vector<std::string> COLORS;
std::vector<std::string> GUESSES;
std::vector<std::string> ANSWERS;
std::vector<LetterCounts> ANSWER_LETTER_COUNTS;

// Mapping from GUESS_INDEX x ANSWER_INDEX -> COLOR_INDEX.
std::vector<int> COLORS_CACHE;

long long CACHE_HITS = 0;
long long CACHE_MISSES = 0;

std::vector<std::string> load_file(const std::string& path) {
  std::vector<std::string> lines;
  std::ifstream f(path);
  std::string line;
  while (std::getline(f, line)) {
    lines.push_back(line);
  }
  return lines;
}

LetterCounts get_letter_counts(const std::string& word) {
  LetterCounts counts;
  for (char c : word) {
    counts[c]++;
  }
  return counts;
}

void initialize_tables() {
  printf("Initializing tables.\n");
  GUESSES = load_file("wordle_allowed_words.txt");
  ANSWERS = load_file("wordle_answers.txt");
  for (const std::string& answer : ANSWERS) {
    ANSWER_LETTER_COUNTS.push_back(get_letter_counts(answer));
  }
  COLORS_CACHE.resize(ANSWERS.size() * GUESSES.size(), -1);
  printf("Done.\n");
}

int lookup_guess(const std::string& guess_str) {
  for (int i = 0; i < GUESSES.size(); i++) {
    if (GUESSES[i] == guess_str) {
      return i;
    }
  }
  assert(false);
  return -1;
}

int lookup_answer(const std::string& answer_str) {
  for (int i = 0; i < ANSWERS.size(); i++) {
    if (ANSWERS[i] == answer_str) {
      return i;
    }
  }
  assert(false);
  return -1;
}

int get_colors_index(const std::string& color_string) {
  int index = 0;
  for (int i = 0; i < WORD_LENGTH; i++) {
    switch (color_string[i]) {
    case '-':
      index += 0;
      break;
    case '+':
      index += 1;
      break;
    case '!':
      index += 2;
      break;
    default:
      assert(false);
      break;
    }
    index <<= 2;
  }
  return index;
}

Outcome make_outcome(const std::string& guess, const std::string& colors) {
  int colors_index = get_colors_index(colors);
  for (int i = 0; i < GUESSES.size(); i++) {
    if (GUESSES[i] == guess) {
      return {i, colors_index};
    }
  }
  assert(false);
  return {-1, -1};
}

int get_colors(int guess, int answer) {
  const int cache_index = guess * ANSWERS.size() + answer;
  if (COLORS_CACHE[cache_index] >= 0) {
    CACHE_HITS++;
    return COLORS_CACHE[cache_index];
  }
  CACHE_MISSES++;
  const std::string guess_str = GUESSES[guess];
  const std::string answer_str = ANSWERS[answer];
  std::string colors(WORD_LENGTH, ' ');
  LetterCounts answer_letter_counts = ANSWER_LETTER_COUNTS[answer];
  LetterCounts guess_letter_counts;
  for (int i = 0; i < WORD_LENGTH; i++) {
    if (guess_str[i] == answer_str[i]) {
      colors[i] = '!';
      guess_letter_counts[guess_str[i]]++;
    } else if (guess_letter_counts[guess_str[i]] < answer_letter_counts[guess_str[i]]) {
      colors[i] = '+';
      guess_letter_counts[guess_str[i]]++;
    } else {
      colors[i] = '-';
    }
  }
  int colors_index = get_colors_index(colors);
  COLORS_CACHE[cache_index] = colors_index;
  return colors_index;
}

bool possible_answer(int word, const std::vector<Outcome>& outcomes) {
  for (const Outcome& outcome : outcomes) {
    if (get_colors(outcome.first, word) != outcome.second) {
      return false;
    }
  }
  return true;
}

std::vector<int> filter_answers(const std::vector<int>& answers,
				const std::vector<Outcome>& outcomes) {
  std::vector<int> filtered;
  for (int answer : answers) {
    if (possible_answer(answer, outcomes)) {
      filtered.push_back(answer);
    }
  }
  return filtered;
}


std::pair<int, double> best_guess(const std::vector<int>& guesses,
				  const std::vector<int>& answers,
				  int depth, int max_depth);

double score_guess(int guess, const std::vector<int>& guesses, const std::vector<int>& answers, int depth, int max_depth) {
  std::unordered_map<int, int> colors_counts;
  for (int answer : answers) {
    colors_counts[get_colors(guess, answer)]++;
  }
  double score = 0;
  double num_answers = static_cast<double>(answers.size());
  for (const auto& colors_count : colors_counts) {
    double count = static_cast<double>(colors_count.second);
    double prob = count / num_answers;
    double num_eliminated = 0;
    num_eliminated = num_answers - count;
    if (depth < max_depth) {
      const Outcome& outcome = {guess, colors_count.first};
      const std::vector<int> answers_left = filter_answers(answers, {outcome});
      auto result = best_guess(guesses, answers_left, depth + 1, max_depth);
      num_eliminated += result.second;
    }
    score += (prob * num_eliminated);
  }
  return score;
}

// Returns guess index, score.
std::pair<int, double> best_guess(const std::vector<int>& guesses,
				  const std::vector<int>& answers,
				  int depth, int max_depth) {
  if (answers.size() == 1) {
    return {lookup_guess(ANSWERS[answers[0]]), 0.0};
  }
  if (depth == 0) {
    printf("Computing shallow scores.\n");
  }
  std::vector<std::pair<int, double>> shallow_scores;
  std::vector<int> worthwhile_guesses;
  double threshold = 0.2 * answers.size();
  for (int guess : guesses) {
    double score = score_guess(guess, guesses, answers, 0, 0);
    if (score > threshold) {
      shallow_scores.push_back({guess, score});
      worthwhile_guesses.push_back(guess);
    }
  }
  if (depth == 0) {
    printf("Done computing shallow scores. %d candidates.\n", shallow_scores.size());
  }

  if (shallow_scores.empty()) {
    return {0, 0.0};
  }

  std::sort(shallow_scores.begin(), shallow_scores.end(), [](auto &left, auto &right) {
    return left.second > right.second;
  });

  if (depth == max_depth) {
    return shallow_scores[0];
  }

  int best_guess;
  double best_score = 0;
  int i = 0;
  for (auto iter = shallow_scores.begin();
       iter != shallow_scores.end() && i < MAX_CANDIDATES;
       ++iter, ++i) {
    if (depth == 0) {
      printf("Candidate %d/%d\n", i, MAX_CANDIDATES);
    }
    int guess = iter->first;
    double score = score_guess(guess, worthwhile_guesses, answers, depth, max_depth);
    if (score > best_score) {
      best_guess = guess;
      best_score = score;
      if (depth == 0) {
	printf("New best: %s - %g\n", GUESSES[best_guess].c_str(), best_score);
      }
    }
  }
  return {best_guess, best_score};
}

void solve(const std::vector<Outcome>& outcomes, int max_depth) {
  std::vector<int> all_answers;
  for (int i = 0; i < ANSWERS.size(); i++) {
    all_answers.push_back(i);
  }
  std::vector<int> answers_left = filter_answers(all_answers, outcomes);
  printf("Num possible answers: %d\n", answers_left.size());
  
  if (answers_left.empty()) {
    printf("No POSSIBLE ANSWERS\n");
    return;
  }

  if (answers_left.size() <= 5) {
    printf("POSSIBLE ANSWERS:");
    for (int answer : answers_left) {
      printf(" '%s'", ANSWERS[answer].c_str());
    }
    printf("\n");
  }

  std::vector<int> all_guesses;
  for (int i = 0; i < GUESSES.size(); i++) {
    all_guesses.push_back(i);
  }
  auto result = best_guess(all_guesses, answers_left, 0, max_depth);
  printf("%s  %g\n", GUESSES[result.first].c_str(), result.second);
  printf("CACHE_HITS: %ld, MISSES: %ld, HIT_RATE: %g\n",
	 CACHE_HITS, CACHE_MISSES, static_cast<double>(CACHE_HITS) / (CACHE_HITS + CACHE_MISSES));
}

void test() {
  std::vector<Outcome> outcomes = {
    make_outcome("crane", "--+-!"),
    make_outcome("mauls", "-!!-+")
  };
  assert(possible_answer(lookup_answer("pause"), outcomes));
  assert(!possible_answer(lookup_answer("acorn"), outcomes));
  
  std::vector<Outcome> outcomes2 = {
    make_outcome("taboo", "-!-!-"),
  };
  assert(!possible_answer(lookup_answer("taboo"), outcomes2));
  assert(possible_answer(lookup_answer("wagon"), outcomes2));
}

void test2() {
  std::vector<Outcome> outcomes = {
    make_outcome("toile", "----+"),
    make_outcome("denar", "-+-+-"),
    make_outcome("glams", "--+-!"),
  };
  solve(outcomes, 2);
}

int main(int argc, char** argv) {
  initialize_tables();
  test2();
  return 0;
}
