package main

import (
  "bufio"
  "fmt"
  "log"
  "os"
  "sort"
)

type LetterCounts = map[rune]int
type Outcome = struct { guess, colors int }

const WORD_LENGTH = 5
const NUM_GUESSES = 12972
const NUM_ANSWERS = 2315
const NUM_COLORS = 683      // 682 == "!!!!!"
const MAX_CANDIDATES = 100

var GUESSES [NUM_GUESSES]string
var ANSWERS [NUM_ANSWERS]string
var ANSWER_LETTER_COUNTS [NUM_ANSWERS]LetterCounts
var COLORS [NUM_GUESSES][NUM_ANSWERS]int

func lookup_guess(guess_str string) int {
  for i, g := range GUESSES {
    if guess_str == g {
      return i
    }
  }
  log.Fatal("No such guess")
  return -1
}

func lookup_answer(answer_str string) int {
  for i, a := range ANSWERS {
    if answer_str == a {
      return i
    }
  }
  log.Fatal("No such answer")
  return -1
}


func read_file(path string, lines []string) {
  file, error := os.Open(path)
  if error != nil {
    log.Fatal(error)
  }
  defer file.Close()

  scanner := bufio.NewScanner(file)
  for i := 0; scanner.Scan(); i++ {
    lines[i] = scanner.Text()
  }
}

func get_letter_counts(word string) LetterCounts {
  counts := make(LetterCounts)
  for _, c := range word {
    counts[c]++
  }
  return counts
}

func get_colors_index(color_string string) int {
  index := 0;
  for i := 0; i < WORD_LENGTH; i++ {
    if i > 0 {
      index <<= 2;
    }
    switch color_string[i] {
    case '-':
      index += 0;
    case '+':
      index += 1;
    case '!':
      index += 2;
    default:
      log.Fatal(color_string[i])
    }
  }
  return index;
}

func lookup_colors(index int) string {
  colors := make([]rune, WORD_LENGTH)
  for i := WORD_LENGTH - 1; i >= 0; i-- {
    switch index & 3 {
    case 0:
      colors[i] = '-'
    case 1:
      colors[i] = '+'
    case 2:
      colors[i] = '!'
    }
    index >>= 2
  }
  return string(colors)
}

func get_colors(guess int, answer int) int {
  guess_str := GUESSES[guess]
  answer_str := ANSWERS[answer]
  answer_letter_counts := ANSWER_LETTER_COUNTS[answer]
  guess_letter_counts := make(LetterCounts)
  colors := make([]rune, WORD_LENGTH)
  for i := 0; i < WORD_LENGTH; i++ {
    g := rune(guess_str[i])
    a := rune(answer_str[i])
    if g == a {
      colors[i] = '!'
      guess_letter_counts[g]++
    } else if guess_letter_counts[g] < answer_letter_counts[g] {
      colors[i] = '+'
      guess_letter_counts[g]++
    } else {
      colors[i] = '-'
    }
  }
  return get_colors_index(string(colors))
}

func possible_answer(word int, outcomes []Outcome) bool {
  for _, outcome := range outcomes {
    if get_colors(outcome.guess, word) != outcome.colors {
      return false
    }
  }
  return true
}

func make_outcome(guess string, colors string) Outcome {
  return Outcome{lookup_guess(guess), get_colors_index(colors)}
}

func filter_answers(answers []int, outcomes []Outcome) []int {
  var filtered []int
  for _, answer := range answers {
    if possible_answer(answer, outcomes) {
      filtered = append(filtered, answer)
    }
  }
  return filtered;
}

func score_guess(guess int, guesses []int, answers []int) float64 {
  var colors_counts [NUM_COLORS]int
  for _, answer := range answers {
    colors_counts[COLORS[guess][answer]]++
  }
  expected_score := 0.0
  num_answers := float64(len(answers))
  for _, count := range colors_counts {
    if count > 0 {
      prob := float64(count) / num_answers
      expected_score += (prob * float64(count))
    }
  }
  return expected_score
}

func score_guess_steps(
  guess int,
  guesses []int,
  answers []int,
  depth int,
  max_depth int) float64 {
  var colors_counts [NUM_COLORS]int
  for _, answer := range answers {
    colors_counts[COLORS[guess][answer]]++
  }
  expected_score := 0.0
  num_answers := float64(len(answers))
  for colors, count := range colors_counts {
    if count > 0 {
      prob := float64(count) / num_answers
      var score float64
      if colors == 682 {  // !!!!!
        score = 0.0
      } else if depth < max_depth {
        outcome := Outcome{guess, colors}
        answers_left := filter_answers(answers, []Outcome{outcome})
        _, best_score := best_guess(guesses, answers_left, depth + 1, max_depth)
        score = best_score + 1
      } else {
        score = float64(5 - depth)  // Assuming all puzzles can be solved within 5.
      }
      expected_score += (prob * score)
    }
  }
  return expected_score
}

func best_guess(guesses []int, answers []int, depth int, max_depth int) (int, float64) {
  if len(answers) == 0 {
    log.Fatal("No more answers")
  }
  if len(answers) == 1 {
    return lookup_guess(ANSWERS[answers[0]]), 0.0  // shouldn't this be 1.0?
  }
  if depth == 0 {
    fmt.Println("Computing shallow scores.")
  }
  type Result struct {
    guess int
    score float64
  }
  shallow_scores := make([]Result, 0)
  worthwhile_guesses := make([]int, 0)
  threshold := 0.8 * float64(len(answers))
  for _, guess := range guesses {
    score := score_guess(guess, guesses, answers)
    if (len(shallow_scores) == 0) || (score < threshold) {
      shallow_scores = append(shallow_scores, Result{guess, score})
      worthwhile_guesses = append(worthwhile_guesses, guess)
    }
  }
  if depth == 0 {
    fmt.Printf("Done computing shallow scores. %d candidates.\n", len(shallow_scores))
  }

  sort.Slice(shallow_scores, func(i, j int) bool {
    return shallow_scores[i].score < shallow_scores[j].score
  })

  if depth == max_depth {
    return shallow_scores[0].guess, shallow_scores[0].score
  }

  var best_guess int
  best_score := 1000000.0
  if depth == 0 {
    type Result struct {
      guess int
      score float64
    }
    result_chan := make(chan Result, MAX_CANDIDATES)
    i := 0
    j := 0
    for ; i < MAX_CANDIDATES && j < len(shallow_scores); i++ {
      var guess int
      if len(answers) <= 10 && i < len(answers) {
        // If there's only a few answers left, always try to guess them
        // first.
        guess = lookup_guess(ANSWERS[answers[i]])
      } else {
        guess = shallow_scores[j].guess
        j++
      }
      go func(guess int) {
        score := score_guess_steps(guess, worthwhile_guesses, answers, depth, max_depth)
        result_chan <- Result{guess, score}
      }(guess)
    }
    for k := 0; k < i; k++ {
      result := <- result_chan
      fmt.Printf("Candidate %03d/%03d: %s  %g\n", k, i, GUESSES[result.guess], result.score)
      if result.score < best_score {
        best_guess = result.guess
        best_score = result.score
        if depth == 0 {
          fmt.Printf("  New best: %s - %g\n", GUESSES[best_guess], best_score);
        }
      }
    }
  } else {
    j := 0
    for i:= 0; i < MAX_CANDIDATES && j < len(shallow_scores); i++ {
      var guess int
      if len(answers) <= 10 && i < len(answers) {
        // If there's only a few answers left, always try to guess them
        // first.
        guess = lookup_guess(ANSWERS[answers[i]])
      } else {
        guess = shallow_scores[j].guess
        j++
      }
      score := score_guess_steps(guess, worthwhile_guesses, answers, depth, max_depth)
      if score < best_score {
        best_guess = guess
        best_score = score
        if depth == 0 {
          fmt.Printf("  New best: %s - %g\n", GUESSES[best_guess], best_score);
        }
      }
    }
  }
  return best_guess, best_score
}

func sequence(n int) []int {
  seq := make([]int, n)
  for i, _ := range seq {
    seq[i] = i
  }
  return seq
}

func solve(outcomes []Outcome, max_depth int) int {
  answers_left := filter_answers(sequence(len(ANSWERS)), outcomes)
  fmt.Printf("Num possible answers: %d\n", len(answers_left))
  
  if len(answers_left) == 0 {
    fmt.Printf("NO POSSIBLE ANSWERS\n")
    return 0
  }

  if len(answers_left) <= 5 {
    fmt.Printf("POSSIBLE ANSWERS:")
    for _, answer := range answers_left {
      fmt.Printf(" '%s'", ANSWERS[answer])
    }
    fmt.Println("");
  }

  best_guess, score := best_guess(sequence(len(GUESSES)), answers_left, 0, max_depth)
  fmt.Printf("%s  %g\n", GUESSES[best_guess], score)
  return best_guess
}

func play() {
  var moves = []struct {
    guess, colors string
  }{
    {"reast", "---+-"},
  }
  outcomes := make([]Outcome, len(moves))
  for i, move := range moves {
    outcomes[i] = make_outcome(move.guess, move.colors)
  }
  fmt.Println(outcomes)
  solve(outcomes, 3)
}

func initialize_tables() {
  read_file("guesses.txt", GUESSES[:])
  read_file("answers.txt", ANSWERS[:])

  // Precompute letter counts for all answers.
  for i, answer := range ANSWERS {
    ANSWER_LETTER_COUNTS[i] = get_letter_counts(answer)
  }
}

func initialize_colors_table() {
  fmt.Println("Initializing colors table")
  for g := 0; g < NUM_GUESSES; g++ {
    for a := 0; a < NUM_ANSWERS; a++ {
      COLORS[g][a] = get_colors(g, a)
    }
  }
  fmt.Println("Done initializing colors table")
}

func main() {
  initialize_tables()
  initialize_colors_table()
  
  play()
}
