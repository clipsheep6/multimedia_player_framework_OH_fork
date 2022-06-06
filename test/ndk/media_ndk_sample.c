/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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
 */

#include <stdio.h>
#include <string.h>
#include "avcodec_ndk_sample.h"
#include "ndk_sample_log.h"

void RunAVCodec(void)
{
    (void)printf("Please select a demo scenario number: \n");
    (void)printf("0:video dec \n");
    (void)printf("1:video enc \n");
    (void)printf("2:audio dec \n");
    (void)printf("3:audio enc \n");

    int input = 0;
    scanf("%d", &input);
    (void)printf("input: %d\n", input);

    switch (input) {
        case 0: // video dec
            RunVideoDec();
            break;
        default:
            (void)printf("invalid select\n");
    }
}

int main(int argc, char *argv[])
{
    (void)printf("Please select a demo scenario number: \n");
    (void)printf("0:player \n");
    (void)printf("1:recorder \n");
    (void)printf("2:avcodec \n");

    int input = 0;
    scanf("%d", &input);
    (void)printf("input: %d\n", input);

    switch (input) {
        case 2: // avcodec number
            RunAVCodec();
            break;
        default:
            (void)printf("invalid select\n");
    }

    return 0;
}
