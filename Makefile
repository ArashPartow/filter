#
# ******************************************************************
# *           Delimiter Seperated Values Filter Library            *
# *                                                                *
# * Author: Arash Partow (2004)                                    *
# * URL: http://www.partow.net/programming/dsvfilter/index.html    *
# *                                                                *
# * Copyright notice:                                              *
# * Free use of the Delimiter Seperated Values Filter Library is   *
# * permitted under the guidelines and in accordance with the most *
# * current version of the Common Public License.                  *
# * http://www.opensource.org/licenses/cpl1.0.php                  *
# *                                                                *
# ******************************************************************
#



COMPILER         = -c++
#COMPILER        = -clang
OPTIMIZATION_OPT = -O3
BASE_OPTIONS     = -ansi -pedantic-errors -Wall -Wextra -Werror -Wno-long-long
OPTIONS          = $(BASE_OPTIONS) $(OPTIMIZATION_OPT) -o
LINKER_OPT       = -L/usr/lib -lstdc++
#LINKER_OPT       = -L/usr/lib -lstdc++ -lboost_iostreams

BUILD_LIST+=dsv_filter_example

all: $(BUILD_LIST)

dsv_filter_example: dsv_filter_example.cpp dsv_filter.hpp exprtk.hpp strtk.hpp
	$(COMPILER) $(OPTIONS) dsv_filter_example dsv_filter_example.cpp $(LINKER_OPT)

pgo: dsv_filter_example.cpp dsv_filter.hpp exprtk.hpp strtk.hpp
	$(COMPILER) $(BASE_OPTIONS) -O3 -march=native -fprofile-generate -o dsv_filter_example dsv_filter_example.cpp $(LINKER_OPT)
	./dsv_filter_example
	$(COMPILER) $(BASE_OPTIONS) -O3 -march=native -fprofile-use -o dsv_filter_example dsv_filter_example.cpp $(LINKER_OPT)

strip_bin:
	strip -s dsv_filter_example

valgrind_check:
	valgrind --leak-check=full --show-reachable=yes --track-origins=yes ./dsv_filter_example

clean:
	rm -f core.* *~ *.o *.bak *stackdump gmon.out *.gcda *.gcno *.gcnor *.gch
