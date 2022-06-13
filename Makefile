wordle: wordle.cc
	g++ -O2 -pg wordle.cc -o wordle

wordle2: wordle2.cc
	g++ -O2 wordle2.cc -o wordle2

wordle3: wordle3.cc
	g++ -O2 wordle3.cc -o wordle3

wordle4: wordle4.cc
	g++ -O2 wordle4.cc -o wordle4

run_wordle2: wordle2
	./wordle2

run_wordle3: wordle3
	./wordle3

run_wordle4: wordle4
	./wordle4
