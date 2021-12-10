test-%: test.cpp ../aoaoaott.hpp
	$(CXX) $< -o $@ -Wall -Wextra -std=c++17 -O0 $(CXXFLAGS) -DCONTAINER=$(subst test-,,$@) -I$(BOOST_PFR_PATH) -lboost_unit_test_framework

test: test-AoS test-SoA
	./test-AoS && ./test-SoA

clean:
	rm test-AoS test-SoA
