catch.o: catch.cpp catch.hpp
	$(CXX) $< -o $@ -c -Wall -Wextra -std=c++17 -O0

test.o: test.cpp aoaoaott.hpp
	$(CXX) $< -o $@ -c -Wall -Wextra -std=c++17 -O0
    
test: test.o catch.o
	$(CXX) $^ -o $@
	./$@

clean:
	rm test *.o