/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

typedef struct {
  ExecContext_execContextVariables_enum_t variableNeeded;
  uint32_t variableSize;
  uint64_t execContextOffset;
  ExecContext_requiredDef_enum_t isRequired;
} ExecContextFieldInfo_t;

typedef struct {
  uint32_t execContextSize;
  uint32_t numExecContextFields;
  ExecContextFieldInfo_t execContextInfo[];
} ExecContextInfo_t;
