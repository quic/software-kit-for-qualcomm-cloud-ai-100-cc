/*
 * Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

// Read the flatbuffer execContext (call this after reading metadata) and
// produce an intermediate representation
// Data provided by compiler as part of metadata to construct
// the exec context structure that will be exposed to network.
// QSM will allocat an array of such structures, deserialize
// the info and populate the array. It will pass this info to
// NSP RI as part of activate and RI will create the execution
// context structure.
#include "execContext_reader.h"
#include "execContext_verifier.h"
#include "stub_header.h"

nnc_status_code_t nnc_load_exec_context_info(uint32_t handle,
                                             void *flatbufferExecContext,
                                             size_t execContextInfoSize) {
  nnc_status_code_t nnc_status = NNC_STATUS_SUCCESS;
  uint32_t exec_context_info_size = 0;
  ExecContextInfo_t *exec_ctxt_info_addr = NULL;
  ExecContextFieldInfo_t *exec_ctx_field_info_ptr = NULL;

  /* Verify integrity of execContext buffer */
  int ok = ExecContext_execContext_verify_as_root((void *)flatbufferExecContext,
                                                  execContextInfoSize);
  if (ok) {
    //const char *s = flatcc_verify_error_string(ok);
    return NNC_STATUS_ERROR;
  }

  ExecContext_execContext_table_t execContextFlat =
      ExecContext_execContext_as_root(flatbufferExecContext);

  /* Deserialize the exec context info vector length */
  uint32_t num_execContext_fields = ExecContext_execContextField_vec_len(
      ExecContext_execContext_execContextFields(execContextFlat));
  exec_context_info_size =
      offsetof(ExecContextInfo_t, execContextInfo) +
      num_execContext_fields * sizeof(ExecContextFieldInfo_t);
  exec_ctxt_info_addr = (ExecContextInfo_t *)nnc_nn_allocate_memory(
      exec_context_info_size, handle, nnc_status);

  if (nnc_status != NNC_STATUS_SUCCESS) {
    return NNC_STATUS_MEM_ALLOC_ERROR;
  }

  memset(exec_ctxt_info_addr, 0, exec_context_info_size);

  /* Deserialize the exec context struct size */
  exec_ctxt_info_addr->execContextSize =
      ExecContext_execContext_execContextSize(execContextFlat);
  exec_ctxt_info_addr->numExecContextFields = num_execContext_fields;

  exec_ctx_field_info_ptr = &exec_ctxt_info_addr->execContextInfo[0];

  for (uint32_t i = 0; i < num_execContext_fields;
       i++, exec_ctx_field_info_ptr++) {
    ExecContext_execContextField_table_t execContextField =
        ExecContext_execContextField_vec_at(
            ExecContext_execContext_execContextFields(execContextFlat), i);

    /* Deserialize the info for each variable */
    exec_ctx_field_info_ptr->variableNeeded =
        ExecContext_execContextField_variableNeeded(execContextField);
    exec_ctx_field_info_ptr->variableSize =
        ExecContext_execContextField_variableSize(execContextField);
    exec_ctx_field_info_ptr->execContextOffset =
        ExecContext_execContextField_execContextOffset(execContextField);
    exec_ctx_field_info_ptr->isRequired =
        ExecContext_execContextField_isRequired(execContextField);

    if (!ExecContext_requiredDef_is_known_value(
            exec_ctx_field_info_ptr->isRequired)) {

      /* Shall we add a error code specific to flag exec context structure
       * incompatibility ? */
      return NNC_STATUS_ERROR;
    }

    /* Check if all the required variables are supported by the Platform */
    if (!ExecContext_execContextVariables_is_known_value(
            exec_ctx_field_info_ptr->variableNeeded) &&
        exec_ctx_field_info_ptr->isRequired ==
            ExecContext_requiredDef_required) {

      /* Shall we add a error code specific to flag exec context structure
       * incompatibility ? */
      return NNC_STATUS_ERROR;
    }
  }

  return nnc_status;
}
