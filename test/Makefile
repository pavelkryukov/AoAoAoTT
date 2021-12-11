test-%: test.cpp ../aoaoaott.hpp Makefile
	$(CXX) $< -o $@ -Wall -Wextra -std=c++17 -O0 $(CXXFLAGS) -DCONTAINER=$(subst test-,,$@) -I$(BOOST_PFR_PATH)

test: test-AoS test-SoA
	./test-AoS && ./test-SoA

clean:
	rm test-AoS test-SoA
