LLVM_CONFIG=/home/share/llvm_build/bin/llvm-config

CXX=`$(LLVM_CONFIG) --bindir`/clang
CXXFLAGS=`$(LLVM_CONFIG) --cppflags` -fPIC -fno-rtti
LDFLAGS=`$(LLVM_CONFIG) --ldflags`
IRFLAGS=-Xclang -disable-O0-optnone -fno-discard-value-names -S -emit-llvm
OPT=`$(LLVM_CONFIG) --bindir`/opt

.PHONY: all test run clean 
all: hw2.so test

test: foo.ll

hw2.so: hw2.cpp
	$(CXX) -shared -o $@ $< $(CXXFLAGS) $(LDFLAGS)

icpp.ll: icpp.c
	$(CXX) $(IRFLAGS) -o $@ $<

icpp2.ll: icpp2.c
	$(CXX) $(IRFLAGS) -o $@ $<

icpp3.ll: icpp3.c
	$(CXX) $(IRFLAGS) -o $@ $<

foo.ll: foo.c
	$(CXX) $(IRFLAGS) -o $@ $<

run: icpp.ll hw2.so
	$(OPT) -disable-output -load-pass-plugin=./hw2.so -passes=hw2 $<

clean:
	rm -f *.o *.ll *.so