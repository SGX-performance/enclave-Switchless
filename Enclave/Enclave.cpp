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

#include "Enclave_t.h"
#include "../include/common.h"

/*
void ecall_repeat_ocalls(unsigned long nrepeats, int use_switchless) {
    sgx_status_t(*ocall_fn)(void) = use_switchless ? ocall_empty_switchless : ocall_empty;
        ocall_fn();
}
*/

void EcallMeasureSDKOcallsPerformance( uint64_t*     performanceCounters,
                                       uint64_t      numRepeats,
                                       OcallParams*  ocallParams,
									   int 			 use_switchless)
{
		if (use_switchless != 1){
			ocallParams->cyclesCount = &performanceCounters[ 0 ];

			const uint16_t requestedCallID = 0;
			ocall_empty( ocallParams ); //Setup startTime to current rdtscp()
			for( uint64_t i=0; i < numRepeats; ++i ) {
				ocallParams->cyclesCount = &performanceCounters[i];
				ocall_empty(ocallParams);
			}
		} else{
			ocallParams->cyclesCount = &performanceCounters[ 0 ];

			const uint16_t requestedCallID = 0;
			ocall_empty_switchless( ocallParams ); //Setup startTime to current rdtscp()
			for( uint64_t i=0; i < numRepeats; ++i ) {
				ocallParams->cyclesCount = &performanceCounters[i];
				ocall_empty_switchless(ocallParams);
		}
	}
}

void ecall_empty(void) {}
void ecall_empty_switchless(void) {}
