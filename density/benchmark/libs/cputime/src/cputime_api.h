/*
 * Centaurean CPUTime
 *
 * Copyright (c) 2015, Guillaume Voirin
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     1. Redistributions of source code must retain the above copyright notice, this
 *        list of conditions and the following disclaimer.
 *
 *     2. Redistributions in binary form must reproduce the above copyright notice,
 *        this list of conditions and the following disclaimer in the documentation
 *        and/or other materials provided with the distribution.
 *
 *     3. Neither the name of the copyright holder nor the names of its
 *        contributors may be used to endorse or promote products derived from
 *        this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * 3/05/15 17:52
 */

#ifndef CPUTIME_API_H
#define CPUTIME_API_H

#if defined(_WIN64) || defined(_WIN32)
#include <windows.h>
#include <time.h>
#include <string.h>

#define CPUTIME_WINDOWS_EXPORT  __declspec(dllexport)
#else

#include <sys/resource.h>

#define CPUTIME_WINDOWS_EXPORT
#endif

/*
 * Data structures
 */
typedef enum {
    CPUTIME_CHRONOMETER_STATE_IDLE, CPUTIME_CHRONOMETER_STATE_RUNNING
} CPUTIME_CHRONOMETER_STATE;

typedef struct {
    struct timeval start;
    struct timeval lap;
    struct timeval stop;
    CPUTIME_CHRONOMETER_STATE state;
} cputime_chronometer;

/*
 * Get the current running task's total usertime
 *
 * @return a timeval structure containing the current running task's total usertime
 */
CPUTIME_WINDOWS_EXPORT struct timeval cputime_get_current_total_usertime();

/*
 * Start a chronometer
 *
 * @param chronometer a usertime chronometer structure
 */
CPUTIME_WINDOWS_EXPORT void cputime_chronometer_start(cputime_chronometer *chrono);

/*
 * Get a lap time in seconds (time since the last start or lap function was used)
 *
 * @param chronometer a usertime chronometer structure
 */
CPUTIME_WINDOWS_EXPORT double cputime_chronometer_lap(cputime_chronometer *chrono);

/*
 * Stop the chronometer, and get time since its start in seconds
 *
 * @param chronometer a usertime chronometer structure
 */
CPUTIME_WINDOWS_EXPORT double cputime_chronometer_stop(cputime_chronometer *chrono);

/*
 * Return the current elapsed time since the chronometer's start
 *
 * @param chronometer a usertime chronometer structure
 */
CPUTIME_WINDOWS_EXPORT double cputime_chronometer_elapsed(cputime_chronometer *chrono);

#endif