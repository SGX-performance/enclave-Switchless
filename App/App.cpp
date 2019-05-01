/*
 * ----------------------------------------
 * enclave-Switchless
 * Copyright 2019 Tõnis Lusmägi
 * Copyright 2019 TalTech
 * 
 * HotCalls
 * Copyright 2017 The Regents of the University of Michigan
 * Ofir Weisse, Valeria Bertacco and Todd Austin
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * ---------------------------------------------
 */

/*
 * Copyright (C) 2011-2018 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>

#include <unistd.h>
#include <pwd.h>
#define MAX_PATH FILENAME_MAX

#include <sgx_urts.h>
#include <sgx_uswitchless.h>
#include "App.h"
#include "Enclave_u.h"
//
#include <iostream>
#include <sstream>
#include <fstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "../include/common.h"
#include "../include/hot_calls.h"

using namespace std;

/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

typedef struct _sgx_errlist_t {
    sgx_status_t err;
    const char *msg;
    const char *sug; /* Suggestion */
} sgx_errlist_t;

#define REPEATS 20000

#define MEASUREMENTS_ROOT_DIR               "measurments"

/* Error code returned by sgx_create_enclave */
static sgx_errlist_t sgx_errlist[] = {
    {
        SGX_ERROR_UNEXPECTED,
        "Unexpected error occurred.",
        NULL
    },
    {
        SGX_ERROR_INVALID_PARAMETER,
        "Invalid parameter.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_MEMORY,
        "Out of memory.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_LOST,
        "Power transition occurred.",
        "Please refer to the sample \"PowerTransition\" for details."
    },
    {
        SGX_ERROR_INVALID_ENCLAVE,
        "Invalid enclave image.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ENCLAVE_ID,
        "Invalid enclave identification.",
        NULL
    },
    {
        SGX_ERROR_INVALID_SIGNATURE,
        "Invalid enclave signature.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_EPC,
        "Out of EPC memory.",
        NULL
    },
    {
        SGX_ERROR_NO_DEVICE,
        "Invalid SGX device.",
        "Please make sure SGX module is enabled in the BIOS, and install SGX driver afterwards."
    },
    {
        SGX_ERROR_MEMORY_MAP_CONFLICT,
        "Memory map conflicted.",
        NULL
    },
    {
        SGX_ERROR_INVALID_METADATA,
        "Invalid enclave metadata.",
        NULL
    },
    {
        SGX_ERROR_DEVICE_BUSY,
        "SGX device was busy.",
        NULL
    },
    {
        SGX_ERROR_INVALID_VERSION,
        "Enclave version was invalid.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ATTRIBUTE,
        "Enclave was not authorized.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_FILE_ACCESS,
        "Can't open enclave file.",
        NULL
    },
};

/* Check error conditions for loading enclave */
void print_error_message(sgx_status_t ret)
{
    size_t idx = 0;
    size_t ttl = sizeof sgx_errlist/sizeof sgx_errlist[0];

    for (idx = 0; idx < ttl; idx++) {
        if(ret == sgx_errlist[idx].err) {
            if(NULL != sgx_errlist[idx].sug)
                printf("Info: %s\n", sgx_errlist[idx].sug);
            printf("Error: %s\n", sgx_errlist[idx].msg);
            break;
        }
    }

    if (idx == ttl)
        printf("Error: Unexpected error occurred.\n");
}

sgx_enclave_id_t m_enclaveID;
int              m_sgxDriver;
string           m_measurementsDir;

void WriteMeasurementsToFile( string fileName, uint64_t* measurementsMatrix, size_t numRows )
{
	string fileFullPath = m_measurementsDir + "/" + fileName;
	cout << "Writing results.. ";
	cout << fileFullPath << " ";
	ofstream measurementsFile;
	measurementsFile.open( fileFullPath, ios::app );
	for( size_t rowIdx = 0; rowIdx < numRows; ++rowIdx ) {
		measurementsFile << measurementsMatrix[ rowIdx ] << " ";
		measurementsFile << "\n";
	}

	measurementsFile.close();

	cout << "Done\n";
}

/* Initialize the enclave:
 *   Step 1: try to retrieve the launch token saved by last transaction
 *   Step 2: call sgx_create_enclave to initialize an enclave instance
 *   Step 3: save the launch token if it is updated
 */

int initialize_enclave(const sgx_uswitchless_config_t* us_config)
{
	sgx_status_t ret = SGX_ERROR_UNEXPECTED;

	/* Call sgx_create_enclave to initialize an enclave instance */
	/* Debug Support: set 2nd parameter to 1 */

	const void* enclave_ex_p[32] = { 0 };

	enclave_ex_p[SGX_CREATE_ENCLAVE_EX_SWITCHLESS_BIT_IDX] = (const void*)us_config;

	ret = sgx_create_enclave_ex(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, NULL, NULL, &global_eid, NULL, SGX_CREATE_ENCLAVE_EX_SWITCHLESS, enclave_ex_p);
	if (ret != SGX_SUCCESS) {
		print_error_message(ret);
		return -1;
	}

	return 0;
}

void GetTimeStamp( char *timestamp, size_t size )
{
  time_t rawtime;
  struct tm * timeinfo;
  time (&rawtime);
  timeinfo = localtime(&rawtime);

  strftime(timestamp,size,"%Y-%m-%d_%H-%M-%S",timeinfo);
}

bool IsDirectoryExists( string path )
{
	struct stat st = {0};

	return ! (stat(path.c_str(), &st) == -1);
}

void CreateMeasurementsDirectory()
{
	char timestamp[ 100 ] = {0};
	GetTimeStamp( timestamp, 100 );
	// printf( "%s\n", timestamp);

	if( ! IsDirectoryExists( MEASUREMENTS_ROOT_DIR )  ) {
		printf( "Creating directory %s\n", MEASUREMENTS_ROOT_DIR );
		mkdir(MEASUREMENTS_ROOT_DIR, 0700);
	}

	m_measurementsDir = string( MEASUREMENTS_ROOT_DIR ) + "/" + timestamp;
	if( ! IsDirectoryExists( m_measurementsDir )  ) {
		printf( "Creating directory %s\n", m_measurementsDir.c_str() );
		mkdir(m_measurementsDir.c_str(), 0700);
	}
}

inline __attribute__((always_inline))  uint64_t rdtscp(void)
{
        unsigned int low, high;

        asm volatile("rdtscp" : "=a" (low), "=d" (high));

        return low | ((uint64_t)high) << 32;
}

/* OCall functions */
void ocall_print_string(const char *str)
{
    /* Proxy/Bridge will check the length and null-terminate
     * the input string to prevent buffer overflow.
     */
    printf("%s", str);
}

void ocall_empty( void* data )
{
    //Because RDTSCP is not allowed inside an enclave in SGX 1.x, we have to issue it here,
    //in the ocall. Therefore, instead of measuring enclave-->ocall-->enclave, we will measure
    //ocall-->enclave-->next_ocall

    static uint64_t startTime     = 0;

    OcallParams* ocallParams = (OcallParams*)data;
    *(ocallParams->cyclesCount)  = rdtscp() - startTime; //startTime was set in previous iteration (except when first called)
    ocallParams->counter++;

    startTime     = rdtscp(); //for next iteration
}

void ocall_empty_switchless( void* data )
{
    //Because RDTSCP is not allowed inside an enclave in SGX 1.x, we have to issue it here,
    //in the ocall. Therefore, instead of measuring enclave-->ocall-->enclave, we will measure
    //ocall-->enclave-->next_ocall

    static uint64_t startTime     = 0;

    OcallParams* ocallParams = (OcallParams*)data;
    *(ocallParams->cyclesCount)  = rdtscp() - startTime; //startTime was set in previous iteration (except when first called)
    ocallParams->counter++;

    startTime     = rdtscp(); //for next iteration
}

void benchmark_empty_ocall(int is_switchless)
{
    uint64_t performaceMeasurements[ REPEATS ]= {0};

    OcallParams ocallParams;
    ocallParams.counter     = 0;

    EcallMeasureSDKOcallsPerformance(global_eid, (uint64_t*)performaceMeasurements, REPEATS, &ocallParams, is_switchless);

    if (is_switchless != 1){
    	printf("Repeating an **ordinary** OCall that does nothing for %d times...\n", REPEATS);
		ostringstream filename;
		filename <<  "Empty_ocall.csv";
		WriteMeasurementsToFile(filename.str(),
								(uint64_t*)performaceMeasurements,
								REPEATS) ;
	} else {
		printf("Repeating an **switchless** OCall that does nothing for %d times...\n", REPEATS);
		ostringstream filename;
		filename <<  "Empty_ocall_switchless.csv";
		WriteMeasurementsToFile(filename.str(),
								(uint64_t*)performaceMeasurements,
								REPEATS) ;
    }
}

void benchmark_empty_ecall(int is_switchless) 
{
	uint64_t performaceMeasurements[REPEATS]= {0};
	uint64_t    startTime       = 0;
	uint64_t    endTime         = 0;

    unsigned long nrepeats = REPEATS;
    printf("Repeating an **%s** ECall that does nothing for %lu times...\n",
            is_switchless ? "switchless" : "ordinary", nrepeats);

    for(uint64_t i=0; i < REPEATS; ++i) {

    startTime = rdtscp();
    sgx_status_t(*ecall_fn)(sgx_enclave_id_t) = is_switchless ? ecall_empty_switchless : ecall_empty;
    ecall_fn(global_eid);
    endTime   = rdtscp();

    performaceMeasurements [ i ] = endTime       - startTime;

    }

    if (is_switchless != 1){
            ostringstream filename;
            filename <<  "Empty_ecall.csv";
            WriteMeasurementsToFile(filename.str(),
            						(uint64_t*)performaceMeasurements,
        							REPEATS) ;
        } else {
            ostringstream filename;
            filename <<  "Empty_ecall_switchless.csv";
            WriteMeasurementsToFile(filename.str(),
            						(uint64_t*)performaceMeasurements,
        							REPEATS) ;
        }
}

/* Application entry */
int SGX_CDECL main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    /* Configuration for Switchless SGX */
    sgx_uswitchless_config_t us_config = SGX_USWITCHLESS_CONFIG_INITIALIZER;
    us_config.num_uworkers = 2;
    us_config.num_tworkers = 2;

    /* Initialize the enclave */
    if(initialize_enclave(&us_config) < 0)
    {
        printf("Error: enclave initialization failed\n");
        return -1;
    }
    CreateMeasurementsDirectory();
    
    printf("Running a benchmark that compares **ordinary** and **switchless** OCalls...\n");
    benchmark_empty_ocall(1);
    benchmark_empty_ocall(0);
    printf("Done.\n");
    

    printf("Running a benchmark that compares **ordinary** and **switchless** ECalls...\n");
    benchmark_empty_ecall(1);
    benchmark_empty_ecall(0);
    printf("Done.\n");

    sgx_destroy_enclave(global_eid);
    return 0;
}
