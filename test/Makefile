catch.o: catch.cpp catch.hpp
	$(CXX) $< -o $@ -c -Wall -Wextra -std=c++17 $(CXXFLAGS) -O0

test-aos.o: test.cpp ../aoaoaott.hpp ../type_list.hpp
	$(CXX) $< -o $@ -c -Wall -Wextra -std=c++17 $(CXXFLAGS) -O0 -DCONTAINER=AoS -I../../pfr/include

test-soa.o: test.cpp ../aoaoaott.hpp ../type_list.hpp
	$(CXX) $< -o $@ -c -Wall -Wextra -std=c++17 $(CXXFLAGS) -O0 -DCONTAINER=SoA -I../../pfr/include

test: test-aos.o test-soa.o catch.o
	$(CXX) $^ -o $@ $(LDFLAGS)

test-aos: test-aos.o catch.o
	$(CXX) $^ -o $@ $(LDFLAGS)
  
clean:
	rm test test-aos *.o
