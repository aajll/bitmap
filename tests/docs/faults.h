/**
 * SPDX-License-Identifier: MIT
 *
 * @file faults.h
 *
 * @brief
 *    Owner interface assumed by the guard example in docs/design.md.
 *
 * @details
 *    Declarations only. The documentation test compiles the example against
 *    this header, so the example must match these prototypes. Nothing links
 *    or runs it.
 */

#ifndef FAULTS_H_
#define FAULTS_H_

#include <stdbool.h>
#include <stddef.h>

/** Saved guard state, such as an interrupt mask, returned by the lock. */
typedef unsigned int faults_key_t;

faults_key_t faults_lock(void);
void faults_unlock(faults_key_t key);
bool fault_raise(size_t position);

#endif /* FAULTS_H_ */
