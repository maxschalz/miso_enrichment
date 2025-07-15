### DO NOT DELETE THIS COMMENT: INSERT_ARCHETYPES_HERE ###
USE_CYCLUS("misoenrichment" "miso_enrich")
USE_CYCLUS("misoenrichment" "python_enrichment")
USE_CYCLUS("misoenrichment" "converters")
# For now, enrichment calculator is only commented out. Depending on the fixes,
# I may delete it altogether (along with the corresponding files).
# USE_CYCLUS("misoenrichment" "enrichment_calculator")
USE_CYCLUS("misoenrichment" "miso_helper")
USE_CYCLUS("misoenrichment" "flexible_input")

USE_CYCLUS("misoenrichment" "var_recipe_source")

USE_CYCLUS("misoenrichment" "gpr_reactor")

INSTALL_CYCLUS_MODULE("misoenrichment" "./")

# install header files
FILE(GLOB h_files "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
