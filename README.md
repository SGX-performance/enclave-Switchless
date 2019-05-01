```
----------------------------------------
enclave-Switchless
Copyright 2019 Tõnis Lusmägi
Copyright 2019 TalTech

HotCalls
Copyright 2017 The Regents of the University of Michigan
Ofir Weisse, Valeria Bertacco and Todd Austin
https://github.com/oweisse/hot-calls

Switchless
Copyright 2019 Intel
---------------------------------------------
```
# enclave-Switchless

Rewrite of Intel SGX SDK Switchless sample project to measure ecalls and ocalls.
Measurments code Based on ISCA 2017 HotCalls paper, that can be found at (http://www.ofirweisse.com/previous_work.html) and intel SGX SDK Switchless sample.

## Build and run tests
`make SGX_MODE=HW SGX_PRERELEASE=1; ./test_switchless`

`./run.sh` to run the full benchmark.

The main benchmark function is at App/App.cpp.

Measurements of different type of calls are in `measurements/<timestamp>` directory:

- Empty_ecall_switchless.csv
- Empty_ocall_switchless.csv
- Empty_ecall.csv
- Empty_ocall.csv

The number of iterations is defined by `REPEATS` at `App/App.cpp`.

The round trip time of calls is measured in cycles, using RDTSCP insturction. The overhead of the RDTSCP insturction is roughly 30 cylces, which should be substructed from the numbers in the `csv` files. Different machines may have different overheads for RDTSCP.  

NOTE: the file `spinlock.c` is taken from Intel's SGX SDK repository at (https://github.com/01org/linux-sgx)
