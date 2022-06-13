import sys
from collections import Counter, defaultdict
import itertools
import multiprocessing

GUESSES = [
  'roate',
  'spyal',
]

OUTCOMES = [
  '+-++-',
  '---++',
]

WORD_LENGTH = 5

POSSIBLE_OUTCOMES = list(itertools.product('!-+', repeat=WORD_LENGTH))

def possible_answer(word, guesses, outcomes):
  letter_counts = Counter(word)
  for guess, outcome in zip(guesses, outcomes):
    # Sometimes it's possible for a letter to occur twice in a guess, where one
    # of the instances is a ! or + and the other is a -. In this case I guess
    # the - means that the letter does not occur twice.
    positive_counts = Counter()
    negative_letters = set()
    for g, o, w in zip(guess, outcome, word):
      if o == '!' and g != w:
        return False
      if o != '!' and g == w:
        return False
      if o in '!+':
        positive_counts[g] += 1
      if o == '-':
        negative_letters.add(g)

    for letter, count in positive_counts.most_common():
      if letter_counts[letter] < count:
        return False

    for letter in negative_letters:
      if letter_counts[letter] > positive_counts[letter]:
        return False
  return True

ANSWERS = [l.strip().lower() for l in open('wordle_answers.txt')]
ANSWERS = [a for a in ANSWERS if possible_answer(a, GUESSES, OUTCOMES)]

def get_outcome(guess, correct_answer):
  outcome = []
  letter_counts = Counter(correct_answer)
  guess_counts = Counter()
  for g, c in zip(guess, correct_answer):
    if g == c:
      outcome.append('!')
      guess_counts[g] += 1
    elif guess_counts[g] < letter_counts[g]:
      outcome.append('+')
      guess_counts[g] += 1
    else:
      outcome.append('-')
  return ''.join(outcome)

def score_guess(guess):
  outcome_counts = Counter([get_outcome(guess, answer) for answer in ANSWERS])
  score = 0
  for _, count in outcome_counts.most_common():
    prob = count / len(ANSWERS)
    num_eliminated = len(ANSWERS) - count
    score += prob * num_eliminated
  return score

if __name__ == '__main__':
  print(possible_answer('haloc', ['taboo'], ['-!-!-']))
  print(len(ANSWERS))
  if len(ANSWERS) == 0:
    print('NO POSSIBLE ANSWERS')
    sys.exit(1)
  if len(ANSWERS) <= 5:
    print(f'POSSIBLE ANSWERS: {ANSWERS}')

  guesses = [l.strip().lower() for l in open('wordle_allowed_words.txt')]
  with multiprocessing.Pool(10) as pool:
    scores = pool.map(score_guess, guesses)
  sorted_guesses = list(sorted(zip(scores, guesses), reverse=True))

  print(sorted_guesses[:10])
