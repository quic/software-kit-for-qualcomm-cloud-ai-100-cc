# Avoid installing all metadata and networkdesc files with EXCLUDE_FROM_ALL
add_subdirectory(metadata EXCLUDE_FROM_ALL)
add_subdirectory(networkdesc EXCLUDE_FROM_ALL)

set(METADATA_INSTALL_FILES
  metadata/metadata-common/inc/AICMetadataExecCtx.h
  metadata/metadata-common/inc/nnc_err_inf.h
  metadata/metadata-common/inc/nnc_mmap_inf.h
  metadata/metadata-common/inc/nnc_pmu_inf.h
  metadata/metadata-common/inc/nnc_port_inf.h
  metadata/metadata-common/inc/nnc_reprog_mcid_inf.h
  metadata/metadata-common/inc/nnc_rtld_inf.h
  metadata/metadata-common/inc/nnc_udma_inf.h
  metadata/metadata-common/inc/nnc_ulog_inf.h)

add_subdirectory(support)
add_subdirectory(program)
add_subdirectory(toolchain)
add_subdirectory(driver)

install(FILES ${METADATA_INSTALL_FILES} DESTINATION dev/inc/compute)
