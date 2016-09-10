/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_AUDIO_FIXEDFFT_H
#define ANDROID_AUDIO_FIXEDFFT_H

#include <stdint.h>
#include <sys/cdefs.h>

/** \cond */
__BEGIN_DECLS
/** \endcond */

/** See description in fixedfft.cpp */
extern void fixed_fft_real(int n, int32_t *v);

/** \cond */
__END_DECLS
/** \endcond */

#endif  // ANDROID_AUDIO_FIXEDFFT_H
