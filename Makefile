build:
	gcc cmd/process_generator.c -o bin/process_generator.out
	gcc cmd/clk.c -o bin/clk.out
	gcc cmd/scheduler.c -o bin/scheduler.out
	gcc cmd/process.c -o bin/process.out
	gcc cmd/test_generator.c -o bin/test_generator.out

clean:
	rm -f *.out  processes.txt

run:
	./process_generator.out


