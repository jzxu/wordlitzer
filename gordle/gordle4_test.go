package main

import (
  "fmt"
  "testing"
)

func Test_get_colors(t *testing.T) {
  var test_cases = []struct {
    guess, answer, colors string
  }{
    {"abbey", "abbey", "!!!!!"},
    {"reast", "thorn", "+---+"},
    {"throb", "thorn", "!!++-"},
    {"orate", "thorn", "++-+-"},
    {"roast", "thorn", "++--+"},
    {"court", "thorn", "-+-!+"},
    {"thorn", "thorn", "!!!!!"},
    {"reast", "other", "++--+"},
    {"tutee", "other", "+--!-"},
    {"other", "other", "!!!!!"},
    {"reast", "tacit", "--+-!"},
    {"dough", "tacit", "-----"},
    {"tapis", "tacit", "!!-!-"},
    {"quail", "tacit", "--+!-"},
    {"peony", "tacit", "-----"},
    {"magic", "tacit", "-!-!+"},
    {"tacit", "tacit", "!!!!!"},
  }

  initialize_tables()
  for _, tc := range test_cases {
    test_name := fmt.Sprintf("%s,%s,%s", tc.guess, tc.answer, tc.colors)
    t.Run(test_name, func(t *testing.T) {
      actual_colors := get_colors(lookup_guess(tc.guess), lookup_answer(tc.answer))
      expected_colors := get_colors_index(tc.colors)
      if (actual_colors != expected_colors) {
        actual_colors_str := lookup_colors(actual_colors)
        t.Errorf("got %s, expected %s", actual_colors_str, tc.colors)
      }
    })
  }
}

func Test_possible_answer(t *testing.T) {
  var test_cases = []struct {
    answer string
    outcomes []Outcome
    valid bool
  }{
    {
      "pause",
      []Outcome{make_outcome("crane", "--+-!"), make_outcome("mauls", "-!!-+")},
      true,
    },
    {
      "acorn",
      []Outcome{make_outcome("crane", "--+-!"), make_outcome("mauls", "-!!-+")},
      false,
    },
    {
      "taboo",
      []Outcome{make_outcome("taboo", "-!-!-")},
      false,
    },
    {
      "wagon",
      []Outcome{make_outcome("taboo", "-!-!-")},
      true,
    },
  }

  initialize_tables()
  for _, tc := range test_cases {
    test_name := fmt.Sprintf("%s,%v", tc.answer, tc.outcomes)
    t.Run(test_name, func(t *testing.T) {
      actual := possible_answer(lookup_answer(tc.answer), tc.outcomes)
      if actual != tc.valid {
        t.Errorf("got %t, expected %t", actual, tc.valid)
      }
    })
  }
}

func Test_filter_answers(t *testing.T) {
  initialize_tables()
  t.Run("test_filter_answers", func(t *testing.T) {
    var outcomes = []Outcome{
      make_outcome("reast", "-----"),
      make_outcome("godly", "--+--"),
    }
    answers_left := filter_answers(sequence(len(ANSWERS)), outcomes)
    if len(answers_left) != 2 {
      t.Errorf("Expecting 2 answers left.")
    }
    t.Log(ANSWERS[answers_left[0]])
    t.Log(ANSWERS[answers_left[1]])
  })
}
