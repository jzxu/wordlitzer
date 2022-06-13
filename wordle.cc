#include <algorithm>
#include <cassert>
#include <cstdio>
#include <functional>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>

using Outcome = std::pair<std::string, std::string>;
using LetterCounts = std::unordered_map<char, int>;

constexpr int WORD_LENGTH = 5;

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

Outcome make_outcome(const std::string& guess, const std::string& colors) {
  return std::make_pair(guess, colors);
}

std::string get_colors(const std::string& guess, const std::string& answer) {
  static std::unordered_map<std::string, std::string> cache;
  auto iter = cache.find(guess + answer);
  if (iter != cache.end()) {
    return iter->second;
  }
  std::string colors(WORD_LENGTH, ' ');
  LetterCounts answer_letter_counts = get_letter_counts(answer);
  LetterCounts guess_letter_counts;
  for (int i = 0; i < WORD_LENGTH; i++) {
    if (guess[i] == answer[i]) {
      colors[i] = '!';
      guess_letter_counts[guess[i]]++;
    } else if (guess_letter_counts[guess[i]] < answer_letter_counts[guess[i]]) {
      colors[i] = '+';
      guess_letter_counts[guess[i]]++;
    } else {
      colors[i] = '-';
    }
  }
  cache[guess + answer] = colors;
  return colors;
}

bool possible_answer(const std::string& word, const std::vector<Outcome>& outcomes) {
  for (const Outcome& outcome : outcomes) {
    if (get_colors(outcome.first, word) != outcome.second) {
      return false;
    }
  }
  return true;
}

std::vector<std::string> filter_answers(const std::vector<std::string>& answers,
					const std::vector<Outcome>& outcomes) {
  std::vector<std::string> filtered;
  for (const std::string& answer : answers) {
    if (possible_answer(answer, outcomes)) {
      filtered.push_back(answer);
    }
  }
  return filtered;
}


std::pair<std::string, double>
best_guess(const std::vector<std::string>& possible_guesses,
	   const std::vector<std::string>& answers, int depth, int max_depth);

double score_guess(const std::string& guess,
		   const std::vector<std::string>& possible_guesses,
		   const std::vector<std::string>& answers, int depth,
		   int max_depth) {
  std::unordered_map<std::string, int> colors_counts;
  for (const std::string& answer : answers) {
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
      const Outcome& outcome = make_outcome(guess, colors_count.first);
      std::vector<std::string> answers_left = filter_answers(answers, {outcome});
      auto result = best_guess(possible_guesses, answers_left, depth + 1, max_depth);
      num_eliminated += result.second;
    }
    score += (prob * num_eliminated);
  }
  return score;
}

std::pair<std::string, double>
best_guess(const std::vector<std::string>& possible_guesses,
	   const std::vector<std::string>& answers,
	   int depth, int max_depth) {
  if (depth == 0) {
    printf("Computing shallow scores.\n");
  }
  std::vector<std::pair<std::string, double>> shallow_scores;
  for (const std::string& guess : possible_guesses) {
    double score = score_guess(guess, possible_guesses, answers, 0, 0);
    if (score > 0) {
      shallow_scores.push_back({guess, score});
    }
  }
  if (depth == 0) {
    printf("Done computing shallow scores. %d candidates.\n", shallow_scores.size());
  }

  if (shallow_scores.empty()) {
    return {possible_guesses[0], 0.0};
  }
  
  std::sort(shallow_scores.begin(), shallow_scores.end(), [](auto &left, auto &right) {
    return left.second > right.second;
  });

  if (depth == max_depth) {
    return shallow_scores[0];
  }
  
  std::string best_guess;
  double best_score = 0;
  int i = 0;
  int max_candidates = 100;
  for (auto iter = shallow_scores.begin();
       iter != shallow_scores.end() && i < max_candidates;
       ++iter, ++i) {
    if (depth == 0) {
      printf("Candidate %d/%d\n", i, max_candidates);
    }
    const std::string& guess = iter->first;
    double score = score_guess(guess, possible_guesses, answers, depth, max_depth);
    if (score > best_score) {
      best_guess = guess;
      best_score = score;
      if (depth == 0) {
	printf("New best: %s - %g\n", best_guess.c_str(), best_score);
      }
    }
  }
  return std::make_pair(best_guess, best_score);
}

void solve(const std::vector<Outcome>& outcomes) {
  std::vector<std::string> answers = load_file("wordle_answers.txt");
  std::vector<std::string> answers_left = filter_answers(answers, outcomes);
  printf("%d\n", answers_left.size());

  if (answers_left.empty()) {
    printf("No POSSIBLE ANSWERS\n");
    return;
  }

  if (answers_left.size() <= 5) {
    printf("POSSIBLE ANSWERS:");
    for (const std::string& answer : answers_left) {
      printf(" '%s'", answer.c_str());
    }
    printf("\n");
  }

  std::vector<std::string> guesses = load_file("wordle_allowed_words.txt");
  auto result = best_guess(guesses, answers_left, 0, 1);
  printf("%s  %g\n", result.first.c_str(), result.second);
}

void test() {
  std::vector<Outcome> outcomes = {
    make_outcome("crane", "--+-!"),
    make_outcome("mauls", "-!!-+")
  };
  assert(possible_answer("pause", outcomes));
  assert(!possible_answer("boron", outcomes));
  
  std::vector<Outcome> outcomes2 = {
    make_outcome("taboo", "-!-!-"),
  };
  assert(!possible_answer("haloo", outcomes2));
  assert(possible_answer("haloc", outcomes2));
}

void test2() {
  std::vector<Outcome> outcomes = {
    make_outcome("toile", "---++"),
    //make_outcome("begar", "-+--!"),
    //make_outcome("unsod", "!----"),
  };
  solve(outcomes);
}

int main(int argc, char** argv) {
  test2();
  return 0;
}
