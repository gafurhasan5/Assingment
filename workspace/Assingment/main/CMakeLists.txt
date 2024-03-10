# In CMakeLists.txt
set(PARTITION_TABLE ${CMAKE_SOURCE_DIR}/partitions.csv)
set(EXTRA_COMPONENT_DIRS "path/to/partition_table_directory")
idf_component_register(
   SRCS main.c        
   INCLUDE_DIRS "."       
    
)