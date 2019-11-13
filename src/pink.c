/*
 * This program uses the PortAudio Portable Audio Library.
 * For more information see: http://www.portaudio.com
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * The text above constitutes the entire PortAudio license; however,
 * the PortAudio community also makes the following non-binding requests:
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * the can be incorporated into the canonical version. It is also
 * requested that these non-binding requests be included along with the
 * license above.
 */

#include <math.h>
#include "include/pink.h"

static unsigned long GenerateRandomNumber(void);

/************************************************************/
/* Calculate pseudo-random 32 bit number based on linear congruential method. */
static unsigned long GenerateRandomNumber(void) {
    /* Change this seed for different random sequences. */
    static unsigned long randSeed = 22222;
    randSeed = (randSeed * 196314165) + 907633515;
    return randSeed;
}

/************************************************************/
/* Setup PinkNoise structure for N rows of generators. */
void InitializePinkNoise(PinkNoise *pink, int numRows) {
    int i;
    long pmax;
    pink->pink_Index = 0;
    pink->pink_IndexMask = (1<<numRows) - 1;
    /* Calculate maximum possible signed random value. Extra 1 for white noise always added. */
    pmax = (numRows + 1) * (1<<(PINK_RANDOM_BITS-1));
    pink->pink_Scalar = 1.0f / pmax;

    /* Initialize rows. */
    for (i = 0; i < numRows; i++) {
        pink->pink_Rows[i] = 0;
    }

    pink->pink_RunningSum = 0;
}

/* Generate Pink noise values between -1.0 and +1.0 */
float GeneratePinkNoise(PinkNoise *pink) {
    long newRandom;
    long sum;
    float output;
    /* Increment and mask index. */
    pink->pink_Index = (pink->pink_Index + 1) & pink->pink_IndexMask;
    /* If index is zero, don't update any random values. */
    if ( pink->pink_Index != 0 ) {
        /* Determine how many trailing zeros in PinkIndex. */
        /* This algorithm will hang if n==0 so test first. */
        int numZeros = 0;
        int n = pink->pink_Index;

        while ((n & 1) == 0) {
            n = n >> 1;
            numZeros++;
        }

        /* Replace the indexed ROWS random value.
         * Subtract and add back to RunningSum instead of adding all the random
         * values together. Only one changes each time.
         */
        pink->pink_RunningSum -= pink->pink_Rows[numZeros];
        newRandom = ((long) GenerateRandomNumber()) >> PINK_RANDOM_SHIFT;
        pink->pink_RunningSum += newRandom;
        pink->pink_Rows[numZeros] = newRandom;
    }

    /* Add extra white noise value. */
    newRandom = ((long) GenerateRandomNumber()) >> PINK_RANDOM_SHIFT;
    sum = pink->pink_RunningSum + newRandom;
    /* Scale to range of -1.0 to 0.9999. */
    output = pink->pink_Scalar * sum;

    return output;
}
