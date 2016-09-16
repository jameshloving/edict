cmake_minimum_required (VERSION 2.6)
project (Edict)
 
# add the executable
add_executable(edict edict.c)
#target_link_libraries(edict libipulog)
#target_link_libraries(edict libnetfilter-log)
target_link_libraries(edict netfilter_log_libipulog)
